#include "wdpch.h"
#include "ModelSpawner.h"

#include "Waldem/AssetsManagement/ContentManager.h"
#include "Waldem/ECS/Components/MeshComponent.h"
#include "Waldem/ECS/Components/Transform.h"
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem::ModelSpawner
{
    ECS::Entity InstantiateModel(const Path& modelPath, ECS::Entity parent)
    {
        CModel* model = CContentManager::LoadAsset<CModel>(modelPath);
        if(model == nullptr)
        {
            return {};
        }

        WArray<ECS::Entity> createdNodes;
        createdNodes.Resize(model->Nodes.Num());

        WArray<Matrix4> worldMatrices;
        worldMatrices.Resize(model->Nodes.Num(), Matrix4(1.0f));

        ECS::Entity firstRootEntity = {};

        for (size_t i = 0; i < model->Nodes.Num(); ++i)
        {
            const auto& node = model->Nodes[i];
            WString nodeName = node.Name.IsEmpty() ? WString("ModelNode") : node.Name;

            ECS::Entity nodeEntity = ECS::CreateSceneEntity(nodeName);
            createdNodes[i] = nodeEntity;

            Matrix4 worldMatrix = node.LocalTransform;
            if(node.ParentIndex >= 0 && (size_t)node.ParentIndex < worldMatrices.Num())
            {
                worldMatrix = worldMatrices[(size_t)node.ParentIndex] * node.LocalTransform;
            }
            else if(parent.is_alive() && parent.has<Transform>())
            {
                worldMatrix = parent.get<Transform>().Matrix * node.LocalTransform;
            }

            worldMatrices[i] = worldMatrix;

            if(nodeEntity.has<Transform>())
            {
                auto& transform = nodeEntity.get_mut<Transform>();
                transform.Matrix = worldMatrix;
                transform.DecompileMatrix();
                transform.LastRotation = transform.Rotation;
                nodeEntity.modified<Transform>();
            }

            if(node.ParentIndex < 0 && !firstRootEntity.is_alive())
            {
                firstRootEntity = nodeEntity;
            }
        }

        for (size_t i = 0; i < model->Nodes.Num(); ++i)
        {
            const auto& node = model->Nodes[i];
            ECS::Entity nodeEntity = createdNodes[i];

            if(node.ParentIndex >= 0 && (size_t)node.ParentIndex < createdNodes.Num())
            {
                ECS::SetParent(nodeEntity, createdNodes[(size_t)node.ParentIndex], true);
            }
            else if(parent.is_alive())
            {
                ECS::SetParent(nodeEntity, parent, true);
            }

            if(node.MeshPaths.IsEmpty())
            {
                continue;
            }

            for (size_t meshIdx = 0; meshIdx < node.MeshPaths.Num(); ++meshIdx)
            {
                ECS::Entity targetEntity = nodeEntity;

                if(meshIdx > 0)
                {
                    WString childName = (std::string(nodeEntity.name().c_str()) + "_Mesh_" + std::to_string(meshIdx)).c_str();
                    targetEntity = ECS::CreateSceneEntity(childName);
                    ECS::SetParent(targetEntity, nodeEntity, true);

                    if(targetEntity.has<Transform>())
                    {
                        auto& childTransform = targetEntity.get_mut<Transform>();
                        childTransform.SetMatrix(Matrix4(1.0f));
                        targetEntity.modified<Transform>();
                    }
                }

                MeshComponent meshComponent;
                meshComponent.MeshRef.Reference = node.MeshPaths[meshIdx];
                targetEntity.set<MeshComponent>(meshComponent);
            }
        }

        if(!firstRootEntity.is_alive() && createdNodes.Num() > 0)
        {
            firstRootEntity = createdNodes[0];
        }

        delete model;
        return firstRootEntity;
    }
}
