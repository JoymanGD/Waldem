#pragma once
#include "GameScene.h"
#include <string>

namespace Waldem
{
    class WALDEM_API SceneManager
    {
    public:
        static GameScene* GetCurrentScene() { return CurrentScene; }

        static void NewScene()
        {
            UnloadScene();
            CurrentScene = new GameScene();
            CurrentScene->Initialize();
        }

        static void UnloadScene()
        {
            WArray<flecs::entity> to_remove;
            auto q = ECS::World.query_builder<SceneEntity>().build();
            q.each([&](flecs::entity e, SceneEntity)
            {
                to_remove.Add(e);
            });
            for (auto& e : to_remove)
                e.destruct();
            
            if(CurrentScene)
            {
                delete CurrentScene;
                CurrentScene = nullptr;
            }
        }

        static void LoadScene(Path& path)
        {
            PendingPath = path;
            LoadProgress = 0.0f;
            LoadInProgress = true;
            NeedToDeserializeScene = true;
            LoadStage = 0;
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
        inline static bool NeedToDeserializeScene = false;
        inline static bool LoadInProgress = false;
        inline static float LoadProgress = 0.0f;
        inline static int LoadStage = 0;
        inline static Path PendingPath = "";
    };
}
