#pragma once

#include "Waldem/Audio/Audio.h"
#include "Waldem/ECS/Components/AudioSource.h"
#include "Waldem/Input/Input.h"
#include "Waldem/Renderer/Viewport/ViewportManager.h"
#include "Waldem/SceneManagement/SceneManager.h"
#include "Waldem/Scripting/ScriptEngine.h"
#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    enum class EditorSimulationState
    {
        Edit = 0,
        Play = 1,
        Pause = 2
    };

    class WALDEM_API EditorSimulation
    {
    public:
        static EditorSimulationState GetState() { return State; }
        static bool IsEditing() { return State == EditorSimulationState::Edit; }
        static bool IsPlaying() { return State == EditorSimulationState::Play || State == EditorSimulationState::Pause; }
        static bool IsPaused() { return State == EditorSimulationState::Pause; }
        static bool ShouldRunRuntimeSystems() { return State == EditorSimulationState::Play; }
        static void SetState(EditorSimulationState state)
        {
            State = state;
            if(state != EditorSimulationState::Play)
            {
                SnapshotPath.clear();
            }
        }

        static bool Play()
        {
            if(State == EditorSimulationState::Pause)
            {
                State = EditorSimulationState::Play;
                Input::SetCursor(false);
                Input::SetEditorCursorReleased(false);
                ViewportManager::FocusViewport(ViewportManager::GetGameViewport(), true);
                return true;
            }

            if(State != EditorSimulationState::Edit || SceneManager::GetCurrentScene() == nullptr)
            {
                return false;
            }

            OriginalScenePath = SceneManager::GetCurrentScenePath();
            SnapshotPath = GetCurrentFolder() / "__EditorPlayMode.scene";
            SceneManager::GetCurrentScene()->Serialize(SnapshotPath);

            State = EditorSimulationState::Play;
            SceneManager::LoadSceneImmediate(SnapshotPath);
            SceneManager::SetCurrentScenePath(OriginalScenePath);
            if(ScriptEngine::IsInitialized())
            {
                ScriptEngine::RecreateEntityInstances();
            }

            ViewportManager::FocusViewport(ViewportManager::GetGameViewport(), true);
            return true;
        }

        static bool Pause()
        {
            if(State != EditorSimulationState::Play)
            {
                return false;
            }

            Input::SetCursor(true);
            ViewportManager::FocusViewport(ViewportManager::GetEditorViewport(), true);

            State = EditorSimulationState::Pause;
            return true;
        }

        static bool Stop()
        {
            if(State == EditorSimulationState::Edit)
            {
                return false;
            }

            Input::SetCursor(true);
            ViewportManager::FocusViewport(ViewportManager::GetEditorViewport(), true);

            ECS::World.query<AudioSource>().each([&](AudioSource& audioSource)
            {
                if(audioSource.ClipRef.IsValid() && audioSource.ClipRef.Clip && audioSource.ClipRef.Clip->CurrentChannel)
                {
                    Audio::Stop(audioSource.ClipRef.Clip);
                }
            });

            State = EditorSimulationState::Edit;

            if(!SnapshotPath.empty() && exists(SnapshotPath))
            {
                SceneManager::LoadSceneImmediate(SnapshotPath);
                SceneManager::SetCurrentScenePath(OriginalScenePath);
            }

            OriginalScenePath.clear();

            return true;
        }

        static const char* GetStateName()
        {
            switch(State)
            {
            case EditorSimulationState::Play: return "Play";
            case EditorSimulationState::Pause: return "Pause";
            default: return "Edit";
            }
        }

    private:
        inline static EditorSimulationState State = EditorSimulationState::Edit;
        inline static Path SnapshotPath;
        inline static Path OriginalScenePath;
    };
}
