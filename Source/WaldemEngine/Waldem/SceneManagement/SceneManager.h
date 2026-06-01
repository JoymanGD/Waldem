#pragma once
#include "GameScene.h"
#include "Waldem/ECS/IdManager.h"
#include "Waldem/ECS/Systems/CoreSystems/HybridRenderingSystem.h"
#include "Waldem/Renderer/Renderer.h"
#include <string>

namespace Waldem
{
    class WALDEM_API SceneManager
    {
    public:
        static GameScene* GetCurrentScene() { return CurrentScene; }
        static const Path& GetCurrentScenePath() { return CurrentScenePath; }
        static void SetCurrentScenePath(const Path& path) { CurrentScenePath = path; }

        static void NewScene()
        {
            UnloadScene();
            CurrentScene = new GameScene();
            CurrentScene->Initialize();
            CurrentScenePath.clear();
        }

        static void UnloadScene()
        {
            Renderer::Wait();

            WArray<flecs::entity> to_remove;
            auto q = ECS::World.query_builder<SceneEntity>().build();
            q.each([&](flecs::entity e, SceneEntity)
            {
                to_remove.Add(e);
            });
            for (auto& e : to_remove)
                e.destruct();

            IdManager::Reset();
            HybridRenderingSystem::ResetSceneRuntimeData();
            Renderer::Wait();
            
            if(CurrentScene)
            {
                delete CurrentScene;
                CurrentScene = nullptr;
            }
        }

        static void LoadScene(const Path& path)
        {
            CurrentScenePath = path;
            PendingPath = path;
            LoadProgress = 0.0f;
            LoadInProgress = true;
            NeedToDeserializeScene = true;
            LoadStage = 0;
        }

        static void LoadSceneImmediate(const Path& path)
        {
            CurrentScenePath = path;
            PendingPath = path;
            NeedToDeserializeScene = false;
            LoadInProgress = false;
            LoadProgress = 0.0f;
            LoadStage = 0;

            UnloadScene();
            CurrentScene = new GameScene();
            CurrentScene->Initialize();
            CurrentScene->Deserialize(PendingPath);
        }

        static bool GetLoadStatus(float& outProgress, std::string& outLabel)
        {
            outProgress = LoadProgress;
            outLabel = PendingPath.empty() ? "Scene" : PendingPath.filename().string();
            return LoadInProgress;
        }

        static void CheckRequests()
        {
            if(NeedToDeserializeScene)
            {
                if(LoadStage == 0)
                {
                    UnloadScene();
                    LoadProgress = 0.25f;
                    LoadStage = 1;
                    return;
                }

                if(LoadStage == 1)
                {
                    CurrentScene = new GameScene();
                    CurrentScene->Initialize();
                    LoadProgress = 0.5f;
                    LoadStage = 2;
                    return;
                }

                if(LoadStage == 2)
                {
                    CurrentScene->Deserialize(PendingPath);
                    LoadProgress = 1.0f;
                    NeedToDeserializeScene = false;
                    LoadInProgress = false;
                    LoadStage = 0;
                }
            }
        }
    private:
        inline static GameScene* CurrentScene = nullptr;
        inline static Path CurrentScenePath = "";
        inline static bool NeedToDeserializeScene = false;
        inline static bool LoadInProgress = false;
        inline static float LoadProgress = 0.0f;
        inline static int LoadStage = 0;
        inline static Path PendingPath = "";
    };
}
