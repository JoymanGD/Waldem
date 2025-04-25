#include "wdpch.h"
#include "GameScene.h"

Waldem::GameScene::~GameScene()
{
    for (auto entity : Entities)
    {
        entity->Destroy();
    }
}

void Waldem::GameScene::Initialize(SceneData* sceneData, InputManager* inputManager, ECSManager* ecsManager, ResourceManager* resourceManager)
{
    EcsManager = ecsManager;
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

void Waldem::GameScene::Serialize(WDataBuffer& outData)
{
    Entities.Serialize(outData);
}

void Waldem::GameScene::Deserialize(WDataBuffer& inData)
{
    Entities.Deserialize(inData);

    for (auto& entity : Entities)
    {
        EcsManager->RegisterEntity(entity);
    }

    EcsManager->Refresh();
}
