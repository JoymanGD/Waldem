#include "wdpch.h"
#include "GameScene.h"

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/EditorComponent.h"
#include "Waldem/ECS/Components/NameComponent.h"

Waldem::GameScene::~GameScene()
{
}

void Waldem::GameScene::Initialize(InputManager* inputManager, ResourceManager* resourceManager)
{
}

void Waldem::GameScene::Draw(float deltaTime)
{
    for (ISystem* system : DrawSystems)
    {
        system->Update(deltaTime);
    }
}

void Waldem::GameScene::Update(float deltaTime)
{
    for (ISystem* system : UpdateSystems)
    {
        system->Update(deltaTime);
    }
}

void Waldem::GameScene::FixedUpdate(float fixedDeltaTime)
{
    for (ISystem* system : PhysicsSystems)
    {
        system->Update(fixedDeltaTime);
    }
}

void Waldem::GameScene::DrawUI(float deltaTime)
{
    for (ISystem* system : Widgets)
    {
        system->Update(deltaTime);
    }
}

void Waldem::GameScene::Serialize(Path& outPath)
{
    flecs::query<NameComponent> q = ECS::World.query_builder<SceneEntity>().without<EditorComponent>().cached().build();
    flecs::string json = q.to_json();

    //TODO: use some custom name
    if(!outPath.has_filename())
    {
        outPath /= "Scene.scene"; 
    }

    std::ofstream outFile(outPath.c_str());
    if (outFile.is_open())
    {
        outFile << json.c_str();
        outFile.close();
    }
}

void Waldem::GameScene::Deserialize(Path& inPath)
{
    std::ifstream inFile(inPath.c_str());
    if (inFile.is_open())
    {
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        inFile.close();

        ECS::World.from_json(buffer.str().c_str(), nullptr);
    }
}
