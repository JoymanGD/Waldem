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
    enum class SimulationState
    {
        Edit = 0,
        Play = 1,
        Pause = 2
    };

    class WALDEM_API Simulation
    {
    public:
        static SimulationState GetState() { return State; }
        static bool IsEditing() { return State == SimulationState::Edit; }
        static bool IsPlaying() { return State == SimulationState::Play || State == SimulationState::Pause; }
        static bool IsPaused() { return State == SimulationState::Pause; }
        static bool ShouldRunRuntimeSystems() { return State == SimulationState::Play; }
        static void SetState(SimulationState state)
        {
            State = state;
            if(state != SimulationState::Play)
            {
                SnapshotPath.clear();
            }
        }

        static bool Play()
        {
            if(State == SimulationState::Pause)
            {
                State = SimulationState::Play;
                Input::SetCursor(false);
                Input::SetEditorCursorReleased(false);
                ViewportManager::FocusViewport(ViewportManager::GetGameViewport(), true);
                return true;
            }

            if(State != SimulationState::Edit || SceneManager::GetCurrentScene() == nullptr)
            {
                return false;
            }

            OriginalScenePath = SceneManager::GetCurrentScenePath();
            SnapshotPath = GetCurrentFolder() / "__EditorPlayMode.scene";
            SceneManager::GetCurrentScene()->Serialize(SnapshotPath);

            State = SimulationState::Play;
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
            if(State != SimulationState::Play)
            {
                return false;
            }

            Input::SetCursor(true);
            ViewportManager::FocusViewport(ViewportManager::GetEditorViewport(), true);

            State = SimulationState::Pause;
            return true;
        }

        static bool Stop()
        {
            if(State == SimulationState::Edit)
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

            State = SimulationState::Edit;

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
            case SimulationState::Play: return "Play";
            case SimulationState::Pause: return "Pause";
            default: return "Edit";
            }
        }

    private:
        inline static SimulationState State = SimulationState::Edit;
        inline static Path SnapshotPath;
        inline static Path OriginalScenePath;
    };
}
