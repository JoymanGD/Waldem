#pragma once
#include "GameScene.h"

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
            Path = path;
            NeedToDeserializeScene = true;
        }

        static void CheckRequests()
        {
            if(NeedToDeserializeScene)
            {
                UnloadScene();
                CurrentScene = new GameScene();
                CurrentScene->Initialize();
                CurrentScene->Deserialize(Path);
                NeedToDeserializeScene = false;
            }
        }
    private:
        inline static GameScene* CurrentScene = nullptr;
        inline static bool NeedToDeserializeScene = false;
        inline static Path Path = "";
    };
}
