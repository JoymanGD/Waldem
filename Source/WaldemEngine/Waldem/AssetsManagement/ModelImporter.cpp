#include "wdpch.h"
#include "ModelImporter.h"
#include "Waldem/Renderer/Model/Material.h"
#include "Waldem/Renderer/Model/Mesh.h"
#include "Waldem/Renderer/Model/Model.h"
#include "Waldem/Renderer/Texture.h"
#include "assimp/material.h"
#include "assimp/scene.h"
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include "Waldem/Engine.h"
#include "Waldem/Renderer/TextureType.h"
#include "Waldem/Utils/StringUtils.h"

#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
namespace Waldem
{
    Matrix4 AssimpToMatrix4(aiMatrix4x4 matrix)
    {
        return Matrix4(
            matrix.a1, matrix.b1, matrix.c1, matrix.d1,
            matrix.a2, matrix.b2, matrix.c2, matrix.d2,
            matrix.a3, matrix.b3, matrix.c3, matrix.d3,
            matrix.a4, matrix.b4, matrix.c4, matrix.d4
        );
    }
    
    namespace
    {
        struct ImportContext
        {
            const aiScene* Scene = nullptr;
            Path SourcePath;
            Path DestinationPath;
            Path RelativeModelDir;
            float UniformScale = 1.0f;
            WArray<Asset*> Assets;

            std::unordered_map<std::string, Path> TextureRefByKey;
            std::unordered_map<unsigned int, Path> MaterialRefByIndex;
            std::unordered_map<unsigned int, Path> MeshPathByIndex;
            std::unordered_map<std::string, int> NameUseCount;
        };

        WString SanitizeName(const WString& inName, const WString& fallback)
        {
            WString mutableName = inName;
            WString cleaned = SanitizeString(mutableName);
            if(cleaned.IsEmpty())
            {
                return fallback;
            }

            return cleaned;
        }

        WString MakeUniqueName(ImportContext& context, const WString& baseName)
        {
            const std::string base = baseName.C_Str();
            int& count = context.NameUseCount[base];
            WString outName = baseName;
            if(count > 0)
            {
                outName += "_";
                outName += std::to_string(count).c_str();
            }
            count++;
            return outName;
        }

        Path GetTextureRef(
            ImportContext& context,
            aiMaterial* assimpMaterial,
            aiTextureType textureType,
            const WString& materialName,
            const WString& usageSuffix)
        {
            aiString texturePath;
            if(assimpMaterial == nullptr || assimpMaterial->GetTexture(textureType, 0, &texturePath) != aiReturn_SUCCESS)
            {
                return "Empty";
            }

            const char* texturePathCStr = texturePath.C_Str();
            if(texturePathCStr == nullptr || texturePathCStr[0] == '\0')
            {
                return "Empty";
            }

            std::string textureKey;
            int width = 0;
            int height = 0;
            int componentsCount = 0;
            uint8_t* imageData = nullptr;

            if(const aiTexture* embeddedTexture = context.Scene->GetEmbeddedTexture(texturePathCStr))
            {
                textureKey = std::string("embedded:") + texturePathCStr;
                auto existingTextureRef = context.TextureRefByKey.find(textureKey);
                if(existingTextureRef != context.TextureRefByKey.end())
                {
                    return existingTextureRef->second;
                }

                if(embeddedTexture->mHeight == 0)
                {
                    imageData = stbi_load_from_memory((const stbi_uc*)embeddedTexture->pcData, embeddedTexture->mWidth, &width, &height, &componentsCount, 4);
                }
                else
                {
                    width = (int)embeddedTexture->mWidth;
                    height = (int)embeddedTexture->mHeight;
                    componentsCount = 4;
                    size_t dataSize = (size_t)width * (size_t)height * 4;
                    imageData = (uint8_t*)malloc(dataSize);
                    memcpy(imageData, embeddedTexture->pcData, dataSize);
                }
            }
            else
            {
                Path externalTexturePath = context.SourcePath.parent_path() / Path(texturePathCStr);
                if(!std::filesystem::exists(externalTexturePath))
                {
                    externalTexturePath = Path(texturePathCStr);
                }

                std::error_code ec;
                Path canonicalPath = std::filesystem::weakly_canonical(externalTexturePath, ec);
                textureKey = ec ? externalTexturePath.lexically_normal().string() : canonicalPath.string();

                auto existingTextureRef = context.TextureRefByKey.find(textureKey);
                if(existingTextureRef != context.TextureRefByKey.end())
                {
                    return existingTextureRef->second;
                }

                imageData = stbi_load(externalTexturePath.string().c_str(), &width, &height, &componentsCount, 4);
            }

            if(imageData == nullptr || width <= 0 || height <= 0)
            {
                WD_CORE_WARN("Failed to load texture while importing model: {0}", texturePathCStr);
                return "Empty";
            }

            WString textureAssetName = materialName + "_" + usageSuffix;
            textureAssetName = MakeUniqueName(context, SanitizeName(textureAssetName, "Texture"));

            TextureDesc* textureDesc = new TextureDesc(textureAssetName, width, height, 1, TextureFormat::R8G8B8A8_UNORM, imageData);
            context.Assets.Add(textureDesc);

            Path textureRef = context.RelativeModelDir / textureAssetName.ToString();
            context.TextureRefByKey[textureKey] = textureRef;
            return textureRef;
        }

        void BuildMaterials(ImportContext& context)
        {
            for (unsigned int materialIndex = 0; materialIndex < context.Scene->mNumMaterials; ++materialIndex)
            {
                aiMaterial* assimpMaterial = context.Scene->mMaterials[materialIndex];

                aiString assimpMaterialName;
                assimpMaterial->Get(AI_MATKEY_NAME, assimpMaterialName);
                WString materialName = SanitizeName(assimpMaterialName.C_Str(), ("Material_" + std::to_string(materialIndex)).c_str());
                materialName = MakeUniqueName(context, materialName);

                Path diffuseRef = GetTextureRef(context, assimpMaterial, aiTextureType_DIFFUSE, materialName, "Diffuse");
                Path normalRef = GetTextureRef(context, assimpMaterial, aiTextureType_NORMALS, materialName, "Normal");

                Path ormRef = GetTextureRef(context, assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS, materialName, "ORM");
                if(ormRef == "Empty")
                {
                    ormRef = GetTextureRef(context, assimpMaterial, aiTextureType_METALNESS, materialName, "Metallic");
                }

                Vector4 albedo(1.0f);
                aiColor4D albedoColor(1.f, 1.f, 1.f, 1.f);
                if(aiGetMaterialColor(assimpMaterial, AI_MATKEY_BASE_COLOR, &albedoColor) != AI_SUCCESS)
                {
                    if(aiGetMaterialColor(assimpMaterial, AI_MATKEY_COLOR_DIFFUSE, &albedoColor) != AI_SUCCESS)
                    {
                        albedoColor = aiColor4D(1.f, 1.f, 1.f, 1.f);
                    }
                }
                albedo = Vector4(albedoColor.r, albedoColor.g, albedoColor.b, albedoColor.a);

                float roughness = 1.0f;
                if(aiGetMaterialFloat(assimpMaterial, AI_MATKEY_ROUGHNESS_FACTOR, &roughness) != AI_SUCCESS)
                {
                    aiGetMaterialFloat(assimpMaterial, AI_MATKEY_SHININESS, &roughness);
                }

                float metallic = 0.0f;
                aiGetMaterialFloat(assimpMaterial, AI_MATKEY_METALLIC_FACTOR, &metallic);

                Material* material = new Material(
                    materialName,
                    TextureReference(diffuseRef),
                    TextureReference(normalRef),
                    TextureReference(ormRef),
                    albedo,
                    roughness,
                    metallic
                );

                context.Assets.Add(material);
                context.MaterialRefByIndex[materialIndex] = context.RelativeModelDir / materialName.ToString();
            }
        }

        AABB BuildMeshAABB(const aiMesh* assimpMesh)
        {
            return AABB
            {
                Vector3(assimpMesh->mAABB.mMin.x, assimpMesh->mAABB.mMin.y, assimpMesh->mAABB.mMin.z),
                Vector3(assimpMesh->mAABB.mMax.x, assimpMesh->mAABB.mMax.y, assimpMesh->mAABB.mMax.z)
            };
        }

        void ApplyUniformScaleToAABB(AABB& aabb, float uniformScale)
        {
            if(uniformScale == 1.0f)
            {
                return;
            }

            aabb.Min *= uniformScale;
            aabb.Max *= uniformScale;
        }

        void BuildMeshes(ImportContext& context)
        {
            for (unsigned int meshIndex = 0; meshIndex < context.Scene->mNumMeshes; ++meshIndex)
            {
                const aiMesh* assimpMesh = context.Scene->mMeshes[meshIndex];
                if(assimpMesh == nullptr)
                {
                    continue;
                }

                WArray<uint32_t> indexBufferData;
                for (unsigned int faceIdx = 0; faceIdx < assimpMesh->mNumFaces; ++faceIdx)
                {
                    const aiFace& face = assimpMesh->mFaces[faceIdx];
                    for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
                    {
                        indexBufferData.Add(face.mIndices[idx]);
                    }
                }

                WArray<Vertex> vertexBufferData;
                vertexBufferData.Reserve(assimpMesh->mNumVertices);

                for (unsigned int vertexIdx = 0; vertexIdx < assimpMesh->mNumVertices; ++vertexIdx)
                {
                    Vertex vertex{};
                    vertex.Position = Vector4(assimpMesh->mVertices[vertexIdx].x, assimpMesh->mVertices[vertexIdx].y, assimpMesh->mVertices[vertexIdx].z, 1.0f);
                    if(context.UniformScale != 1.0f)
                    {
                        vertex.Position.x *= context.UniformScale;
                        vertex.Position.y *= context.UniformScale;
                        vertex.Position.z *= context.UniformScale;
                    }

                    if(assimpMesh->HasNormals())
                    {
                        vertex.Normal = Vector4(assimpMesh->mNormals[vertexIdx].x, assimpMesh->mNormals[vertexIdx].y, assimpMesh->mNormals[vertexIdx].z, 0.0f);
                    }
                    else
                    {
                        vertex.Normal = Vector4(0, 1, 0, 0);
                    }

                    if(assimpMesh->HasTangentsAndBitangents())
                    {
                        vertex.Tangent = Vector4(assimpMesh->mTangents[vertexIdx].x, assimpMesh->mTangents[vertexIdx].y, assimpMesh->mTangents[vertexIdx].z, 0.0f);
                        vertex.Bitangent = Vector4(assimpMesh->mBitangents[vertexIdx].x, assimpMesh->mBitangents[vertexIdx].y, assimpMesh->mBitangents[vertexIdx].z, 0.0f);
                    }
                    else
                    {
                        vertex.Tangent = Vector4(1, 0, 0, 0);
                        vertex.Bitangent = Vector4(0, 0, 1, 0);
                    }

                    if(assimpMesh->HasVertexColors(0))
                    {
                        vertex.Color = Vector4(
                            assimpMesh->mColors[0][vertexIdx].r,
                            assimpMesh->mColors[0][vertexIdx].g,
                            assimpMesh->mColors[0][vertexIdx].b,
                            assimpMesh->mColors[0][vertexIdx].a
                        );
                    }
                    else
                    {
                        vertex.Color = Vector4(1.0f);
                    }

                    if(assimpMesh->HasTextureCoords(0))
                    {
                        vertex.UV = Vector2(assimpMesh->mTextureCoords[0][vertexIdx].x, assimpMesh->mTextureCoords[0][vertexIdx].y);
                    }
                    else
                    {
                        vertex.UV = Vector2(0, 0);
                    }

                    vertexBufferData.Add(vertex);
                }

                WString meshName = SanitizeName(assimpMesh->mName.C_Str(), ("Mesh_" + std::to_string(meshIndex)).c_str());
                meshName = MakeUniqueName(context, meshName);

                Path materialRef = "Empty";
                auto materialIt = context.MaterialRefByIndex.find(assimpMesh->mMaterialIndex);
                if(materialIt != context.MaterialRefByIndex.end())
                {
                    materialRef = materialIt->second;
                }

                AABB meshAABB = BuildMeshAABB(assimpMesh);
                ApplyUniformScaleToAABB(meshAABB, context.UniformScale);

                CMesh* mesh = new CMesh(meshName);
                mesh->VertexData = vertexBufferData;
                mesh->IndexData = indexBufferData;
                mesh->MaterialRef = MaterialReference(materialRef);
                mesh->BBox = meshAABB;
                mesh->ObjectMatrix = Matrix4(1.0f);
                mesh->Positions.Resize(mesh->VertexData.Num());
                for (size_t positionIndex = 0; positionIndex < mesh->VertexData.Num(); ++positionIndex)
                {
                    mesh->Positions[positionIndex] = Vector3(mesh->VertexData[positionIndex].Position);
                }

                context.Assets.Add(mesh);
                context.MeshPathByIndex[meshIndex] = context.RelativeModelDir / (meshName.ToString() + ".mesh");
            }
        }

        void BuildModelNodeRecursive(ImportContext& context, CModel* model, aiNode* assimpNode, int parentIndex)
        {
            if(assimpNode == nullptr)
            {
                return;
            }

            CModelNode node;
            node.Name = SanitizeName(assimpNode->mName.C_Str(), "Node");
            node.ParentIndex = parentIndex;
            node.LocalTransform = AssimpToMatrix4(assimpNode->mTransformation);
            if(context.UniformScale != 1.0f)
            {
                node.LocalTransform[3].x *= context.UniformScale;
                node.LocalTransform[3].y *= context.UniformScale;
                node.LocalTransform[3].z *= context.UniformScale;
            }

            for (unsigned int i = 0; i < assimpNode->mNumMeshes; ++i)
            {
                const unsigned int meshIndex = assimpNode->mMeshes[i];
                auto meshPathIt = context.MeshPathByIndex.find(meshIndex);
                if(meshPathIt != context.MeshPathByIndex.end())
                {
                    node.MeshPaths.Add(meshPathIt->second);
                }
            }

            const int nodeIndex = (int)model->Nodes.Add(node);

            for (unsigned int childIdx = 0; childIdx < assimpNode->mNumChildren; ++childIdx)
            {
                BuildModelNodeRecursive(context, model, assimpNode->mChildren[childIdx], nodeIndex);
            }
        }

        CModel* BuildModelAsset(ImportContext& context)
        {
            WString modelName = SanitizeName(context.SourcePath.stem().string().c_str(), "ImportedModel");
            CModel* model = new CModel(modelName);

            BuildModelNodeRecursive(context, model, context.Scene->mRootNode, -1);
            return model;
        }
    }

    WArray<Asset*> CModelImporter::ImportTo(const Path& from, const Path& to, bool relative)
    {
        ImportContext context{};
        context.Scene = ImportInternal(
            from,
            ModelImportFlags::CalcTangentSpace |
            ModelImportFlags::Triangulate |
            ModelImportFlags::GenBoundingBoxes |
            ModelImportFlags::FlipUVs |
            ModelImportFlags::MakeLeftHanded |
            ModelImportFlags::FlipWindingOrder |
            ModelImportFlags::JoinIdenticalVertices |
            ModelImportFlags::ImproveCacheLocality,
            relative
        );
        context.SourcePath = from;
        context.DestinationPath = to;
        context.UniformScale = Settings.UniformScale;

        context.RelativeModelDir = std::filesystem::relative(to, CONTENT_PATH).string();
        context.RelativeModelDir /= from.stem();

        if(context.Scene == nullptr)
        {
            WD_CORE_ERROR("Failed to load model from path: {0}", from.string());
            return {};
        }

        BuildMaterials(context);
        BuildMeshes(context);
        context.Assets.Add(BuildModelAsset(context));
        return context.Assets;
    }

    const aiScene* CModelImporter::ImportInternal(const Path& path, ModelImportFlags importFlags, bool relative)
    {
        return AssimpImporter.ReadFile(path.string(), (unsigned)importFlags);
    }
}
