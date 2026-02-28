#include "wdpch.h"
#include "Prefab.h"

#include "Waldem/ECS/Components/SceneEntity.h"
#include "Waldem/ECS/Components/Transform.h"
#include "glm/gtc/type_ptr.hpp"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace Waldem::Prefab
{
    namespace
    {
        bool IsDescendantOf(ECS::Entity entity, ECS::Entity possibleAncestor)
        {
            if(!entity.is_alive() || !possibleAncestor.is_alive())
            {
                return false;
            }

            ECS::Entity currentParent = ECS::GetParent(entity);
            while(currentParent.is_alive())
            {
                if(currentParent.id() == possibleAncestor.id())
                {
                    return true;
                }

                currentParent = ECS::GetParent(currentParent);
            }

            return false;
        }
    }

    bool SaveEntityAsPrefab(ECS::Entity rootEntity, const Path& outPath)
    {
        if(!rootEntity.is_alive() || !rootEntity.has<SceneEntity>())
        {
            return false;
        }

        std::vector<ECS::Entity> subtreeEntities;
        auto query = ECS::World.query_builder<SceneEntity>().build();
        query.each([&](ECS::Entity entity, const SceneEntity&)
        {
            if(entity.id() == rootEntity.id() || IsDescendantOf(entity, rootEntity))
            {
                subtreeEntities.push_back(entity);
            }
        });

        std::sort(subtreeEntities.begin(), subtreeEntities.end(), [](ECS::Entity lhs, ECS::Entity rhs)
        {
            const SceneEntity& lhsSceneEntity = lhs.get<SceneEntity>();
            const SceneEntity& rhsSceneEntity = rhs.get<SceneEntity>();

            if(lhsSceneEntity.HierarchyDepth != rhsSceneEntity.HierarchyDepth)
            {
                return lhsSceneEntity.HierarchyDepth < rhsSceneEntity.HierarchyDepth;
            }

            return lhsSceneEntity.HierarchySlot < rhsSceneEntity.HierarchySlot;
        });

        std::unordered_map<flecs::entity_t, int> prefabIdByEntityId;
        prefabIdByEntityId.reserve(subtreeEntities.size());
        for (int i = 0; i < (int)subtreeEntities.size(); ++i)
        {
            prefabIdByEntityId[subtreeEntities[(size_t)i].id()] = i;
        }

        rapidjson::StringBuffer outBuffer;
        rapidjson::PrettyWriter writer(outBuffer);
        writer.SetIndent(' ', 2);

        writer.StartObject();
        writer.Key("version");
        writer.Int(1);
        writer.Key("nodes");
        writer.StartArray();

        for (int i = 0; i < (int)subtreeEntities.size(); ++i)
        {
            auto entity = subtreeEntities[(size_t)i];
            auto parent = ECS::GetParent(entity);

            int prefabParentId = -1;
            if(parent.is_alive())
            {
                auto it = prefabIdByEntityId.find(parent.id());
                if(it != prefabIdByEntityId.end())
                {
                    prefabParentId = it->second;
                }
            }

            std::string entityJson = entity.to_json().c_str();
            rapidjson::Document entityDoc;
            entityDoc.Parse(entityJson.c_str());
            if(entityDoc.HasParseError() || !entityDoc.IsObject())
            {
                continue;
            }

            Matrix4 localMatrix(1.0f);
            if(entity.has<Transform>())
            {
                const auto& entityTransform = entity.get<Transform>();
                localMatrix = entityTransform.Matrix;

                if(parent.is_alive() && parent.has<Transform>())
                {
                    auto parentIt = prefabIdByEntityId.find(parent.id());
                    if(parentIt != prefabIdByEntityId.end())
                    {
                        localMatrix = inverse(parent.get<Transform>().Matrix) * entityTransform.Matrix;
                    }
                }
            }

            writer.StartObject();
            writer.Key("id");
            writer.Int(i);
            writer.Key("parent");
            writer.Int(prefabParentId);
            writer.Key("local_matrix");
            writer.StartArray();
            const float* localMatrixPtr = glm::value_ptr(localMatrix);
            for (int matrixIndex = 0; matrixIndex < 16; ++matrixIndex)
            {
                writer.Double(localMatrixPtr[matrixIndex]);
            }
            writer.EndArray();
            writer.Key("entity");
            entityDoc.Accept(writer);
            writer.EndObject();
        }

        writer.EndArray();
        writer.EndObject();

        std::error_code ec;
        std::filesystem::create_directories(outPath.parent_path(), ec);

        std::ofstream outFile(outPath.c_str(), std::ios::out | std::ios::trunc);
        if(!outFile.is_open())
        {
            return false;
        }

        outFile << outBuffer.GetString();
        return true;
    }

    ECS::Entity InstantiatePrefab(const Path& prefabPath, ECS::Entity parent)
    {
        std::ifstream inFile(prefabPath.c_str());
        if(!inFile.is_open())
        {
            return {};
        }

        rapidjson::IStreamWrapper isw(inFile);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if(doc.HasParseError() || !doc.IsObject() || !doc.HasMember("nodes") || !doc["nodes"].IsArray())
        {
            return {};
        }

        const auto& nodes = doc["nodes"].GetArray();

        std::unordered_map<int, ECS::Entity> entityByPrefabId;
        entityByPrefabId.reserve(nodes.Size());
        std::vector<std::pair<int, int>> parentLinks;
        parentLinks.reserve(nodes.Size());
        std::vector<int> rootPrefabIds;
        rootPrefabIds.reserve(nodes.Size());
        std::unordered_map<int, Matrix4> localMatrixByPrefabId;
        localMatrixByPrefabId.reserve(nodes.Size());
        std::unordered_map<int, int> parentByPrefabId;
        parentByPrefabId.reserve(nodes.Size());

        for (const auto& node : nodes)
        {
            if(!node.IsObject() || !node.HasMember("id") || !node["id"].IsInt() ||
               !node.HasMember("parent") || !node["parent"].IsInt() ||
               !node.HasMember("entity") || !node["entity"].IsObject())
            {
                continue;
            }

            rapidjson::StringBuffer entityBuffer;
            rapidjson::Writer entityWriter(entityBuffer);
            node["entity"].Accept(entityWriter);

            ECS::Entity entity = ECS::World.entity();
            entity.from_json(entityBuffer.GetString());

            if(entity.has<SceneEntity>())
            {
                auto& sceneEntity = entity.get_mut<SceneEntity>();
                sceneEntity.ParentId = 0;
                sceneEntity.HierarchySlot = -1.0f;
                sceneEntity.HierarchyDepth = 0.0f;
                entity.modified<SceneEntity>();
            }

            if(entity.has<Transform>())
            {
                auto& transform = entity.get_mut<Transform>();
                transform.LastRotation = transform.Rotation;
                // Keep serialized world matrix intact; parenting will derive local from it.
                entity.modified<Transform>();
            }

            const int prefabId = node["id"].GetInt();
            const int prefabParentId = node["parent"].GetInt();
            entityByPrefabId[prefabId] = entity;
            parentLinks.emplace_back(prefabId, prefabParentId);
            parentByPrefabId[prefabId] = prefabParentId;

            if(prefabParentId < 0)
            {
                rootPrefabIds.push_back(prefabId);
            }

            Matrix4 localMatrix(1.0f);
            if(node.HasMember("local_matrix") && node["local_matrix"].IsArray() && node["local_matrix"].Size() == 16)
            {
                float* localMatrixPtr = glm::value_ptr(localMatrix);
                const auto& localMatrixArray = node["local_matrix"].GetArray();
                for (int matrixIndex = 0; matrixIndex < 16; ++matrixIndex)
                {
                    localMatrixPtr[matrixIndex] = localMatrixArray[(rapidjson::SizeType)matrixIndex].GetFloat();
                }
            }
            else if(entity.has<Transform>())
            {
                localMatrix = entity.get<Transform>().Matrix;
            }

            localMatrixByPrefabId[prefabId] = localMatrix;
        }

        for (const auto& [prefabId, prefabParentId] : parentLinks)
        {
            if(prefabParentId < 0)
            {
                continue;
            }

            auto childIt = entityByPrefabId.find(prefabId);
            auto parentIt = entityByPrefabId.find(prefabParentId);
            if(childIt == entityByPrefabId.end() || parentIt == entityByPrefabId.end())
            {
                continue;
            }

            ECS::SetParent(childIt->second, parentIt->second, true);
        }

        std::unordered_map<int, Matrix4> worldMatrixByPrefabId;
        worldMatrixByPrefabId.reserve(entityByPrefabId.size());
        std::vector<int> orderedPrefabIds;
        orderedPrefabIds.reserve(entityByPrefabId.size());
        for (const auto& [prefabId, _] : entityByPrefabId)
        {
            orderedPrefabIds.push_back(prefabId);
        }

        std::sort(orderedPrefabIds.begin(), orderedPrefabIds.end(), [&](int lhs, int rhs)
        {
            int lhsDepth = 0;
            int rhsDepth = 0;

            int current = lhs;
            while(parentByPrefabId.contains(current) && parentByPrefabId[current] >= 0)
            {
                lhsDepth++;
                current = parentByPrefabId[current];
            }

            current = rhs;
            while(parentByPrefabId.contains(current) && parentByPrefabId[current] >= 0)
            {
                rhsDepth++;
                current = parentByPrefabId[current];
            }

            if(lhsDepth != rhsDepth)
            {
                return lhsDepth < rhsDepth;
            }

            return lhs < rhs;
        });

        for (int prefabId : orderedPrefabIds)
        {
            auto entityIt = entityByPrefabId.find(prefabId);
            if(entityIt == entityByPrefabId.end())
            {
                continue;
            }

            ECS::Entity entity = entityIt->second;
            if(!entity.has<Transform>())
            {
                continue;
            }

            Matrix4 worldMatrix = localMatrixByPrefabId[prefabId];
            const int prefabParentId = parentByPrefabId[prefabId];
            if(prefabParentId >= 0 && worldMatrixByPrefabId.contains(prefabParentId))
            {
                worldMatrix = worldMatrixByPrefabId[prefabParentId] * localMatrixByPrefabId[prefabId];
            }

            auto& transform = entity.get_mut<Transform>();
            transform.Matrix = worldMatrix;
            transform.DecompileMatrix();
            transform.LastRotation = transform.Rotation;
            entity.modified<Transform>();

            worldMatrixByPrefabId[prefabId] = worldMatrix;
        }

        if(parent.is_alive())
        {
            for (int rootPrefabId : rootPrefabIds)
            {
                auto rootIt = entityByPrefabId.find(rootPrefabId);
                if(rootIt == entityByPrefabId.end())
                {
                    continue;
                }

                ECS::SetParent(rootIt->second, parent, true);
            }
        }

        if(rootPrefabIds.empty())
        {
            return {};
        }

        auto rootIt = entityByPrefabId.find(rootPrefabIds.front());
        if(rootIt == entityByPrefabId.end())
        {
            return {};
        }

        return rootIt->second;
    }
}
