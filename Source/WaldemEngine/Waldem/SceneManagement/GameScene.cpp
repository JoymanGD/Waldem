#include "wdpch.h"
#include "GameScene.h"

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include <unordered_map>
#include <vector>

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
    std::vector<flecs::entity_t> sceneEntityIds;
    std::unordered_map<flecs::entity_t, int> sceneIdByEntityId;

    auto query = ECS::World.query_builder<SceneEntity>().build();
    query.each([&](flecs::entity entity, SceneEntity)
    {
        const int sceneId = static_cast<int>(sceneEntityIds.size());
        sceneEntityIds.push_back(entity.id());
        sceneIdByEntityId[entity.id()] = sceneId;
    });

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter writer(buffer);

    writer.SetIndent(' ', 2);
    writer.StartArray();

    for(const auto entityId : sceneEntityIds)
    {
        auto entity = ECS::World.entity(entityId);
        std::string jsonStr = entity.to_json().c_str();

        rapidjson::Document tmpDoc;
        tmpDoc.Parse(jsonStr.c_str());

        if (!tmpDoc.HasParseError() && tmpDoc.IsObject())
        {
            int parentSceneId = -1;
            auto parent = ECS::GetParent(entity);
            if(parent.is_alive())
            {
                auto it = sceneIdByEntityId.find(parent.id());
                if(it != sceneIdByEntityId.end())
                {
                    parentSceneId = it->second;
                }
            }

            writer.StartObject();
            writer.Key("scene_id");
            writer.Int(sceneIdByEntityId[entityId]);
            writer.Key("parent_scene_id");
            writer.Int(parentSceneId);
            writer.Key("entity");
            tmpDoc.Accept(writer);
            writer.EndObject();
        }
    }

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

    std::unordered_map<int, flecs::entity_t> entityBySceneId;
    std::vector<std::pair<flecs::entity_t, int>> pendingParentLinks;

    // Iterate over each entity object in the array
    for (auto& entVal : doc.GetArray())
    {
        if (!entVal.IsObject())
            continue;
        
        rapidjson::Value* entityJsonValue = &entVal;
        int sceneId = -1;
        int parentSceneId = -1;

        if (entVal.HasMember("entity") && entVal["entity"].IsObject())
        {
            entityJsonValue = &entVal["entity"];

            if (entVal.HasMember("scene_id") && entVal["scene_id"].IsInt())
            {
                sceneId = entVal["scene_id"].GetInt();
            }

            if (entVal.HasMember("parent_scene_id") && entVal["parent_scene_id"].IsInt())
            {
                parentSceneId = entVal["parent_scene_id"].GetInt();
            }
        }

        // Convert entity JSON back into a string
        rapidjson::StringBuffer buffer;
        rapidjson::Writer writer(buffer);
        entityJsonValue->Accept(writer);

        // Create a new entity from JSON
        auto entity = ECS::World.entity();
        entity.from_json(buffer.GetString());
        
        if(sceneId >= 0)
        {
            entityBySceneId[sceneId] = entity.id();
            pendingParentLinks.emplace_back(entity.id(), parentSceneId);
        }

        if (entity.has<Transform>())
        {
            auto& transform = entity.get_mut<Transform>();
            transform.LastRotation = transform.Rotation;
            transform.Update();
            // transform->LastPosition = transform->Position;
            // transform->LastScale = transform->LocalScale;
            entity.modified<Transform>();
        }
    }
    
    for(const auto& [childId, parentSceneId] : pendingParentLinks)
    {
        auto child = ECS::World.entity(childId);
        if(!child.is_alive() || !child.has<SceneEntity>())
        {
            continue;
        }

        uint64 resolvedParentId = 0;
        if(parentSceneId >= 0)
        {
            auto it = entityBySceneId.find(parentSceneId);
            if(it != entityBySceneId.end())
            {
                resolvedParentId = static_cast<uint64>(it->second);
            }
        }

        auto& sceneEntity = child.get_mut<SceneEntity>();
        sceneEntity.ParentId = resolvedParentId;
        child.modified<SceneEntity>();
    }

    ECS::RebuildParentRelations();
}
