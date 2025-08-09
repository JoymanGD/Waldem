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
    auto json = ECS::World.to_json();

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
    ECS::World.from_json_file(inPath.string().c_str(), nullptr);
    ECS::World.system("Test").kind(flecs::OnUpdate).each([]
    {
       WD_CORE_INFO("Test system executed"); 
    });
}
