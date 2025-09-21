#include "wdpch.h"
#include "GameScene.h"

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/istreamwrapper.h"
#include "Waldem/ECS/Components/Selected.h"

Waldem::GameScene::~GameScene()
{
}

void Waldem::GameScene::Initialize()
{
}

void Waldem::GameScene::Draw(float deltaTime)
{
}

void Waldem::GameScene::FixedUpdate(float fixedDeltaTime)
{
}

void Waldem::GameScene::DrawUI(float deltaTime)
{
}

void Waldem::GameScene::Serialize(Path& outPath)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer writer(buffer);

    writer.StartArray();

    auto query = ECS::World.query_builder<SceneEntity>().build();
    query.each([&](flecs::entity entity, SceneEntity)
    {
        std::string jsonStr = entity.to_json().c_str();

        rapidjson::Document tmpDoc;
        tmpDoc.Parse(jsonStr.c_str());

        if (!tmpDoc.HasParseError() && tmpDoc.IsObject())
        {
            tmpDoc.Accept(writer);
        }
    });

    writer.EndArray();

    if (!outPath.has_filename())
        outPath /= "Scene.scene";

    std::ofstream outFile(outPath.c_str(), std::ios::out | std::ios::trunc);
    if (outFile.is_open())
    {
        outFile << buffer.GetString();
    }
}

void Waldem::GameScene::Deserialize(Path& inPath)
{
    // Open file
    std::ifstream ifs(inPath.c_str());
    if (!ifs.is_open())
        return;

    rapidjson::IStreamWrapper isw(ifs);
    rapidjson::Document doc;
    doc.ParseStream(isw);

    if (doc.HasParseError() || !doc.IsArray())
        return;

    // Iterate over each entity object in the array
    for (auto& entVal : doc.GetArray())
    {
        if (!entVal.IsObject())
            continue;

        // Convert entity JSON back into a string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        entVal.Accept(writer);

        // Create a new entity from JSON
        auto entity = ECS::World.entity();
        entity.from_json(buffer.GetString());

        if (entity.has<Transform>())
        {
            auto transform = entity.get_mut<Transform>();
            transform->LastRotation = transform->Rotation;
            transform->Update();
            // transform->LastPosition = transform->Position;
            // transform->LastScale = transform->LocalScale;
            entity.modified<Transform>();
        }
    }
}
