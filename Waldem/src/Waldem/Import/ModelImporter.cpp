#include "wdpch.h"
#include "ModelImporter.h"
#ifndef STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
#include "Waldem/Renderer/Texture.h"
#include <filesystem>
#include <fstream>
#include "assimp/scene.h"
#include "Waldem/Application.h"
#include "Waldem/Renderer/TextureType.h"

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
    
    Texture2D* CreateDummyTexture(String name)
    {
        Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
        uint8_t* image_data = (uint8_t*)&dummyColor;

        int width = 1;
        int height = 1;

        TextureFormat format = TextureFormat::R8G8B8A8_UNORM;

        return Renderer::CreateTexture(name, width, height, format, image_data);
    }

    Texture2D* CreateTexture(String path, String name, TextureType textureType, const aiScene* assimpModel, aiMaterial* assimpMaterial)
    {
        aiString texturePath;
        
        if (assimpMaterial && assimpMaterial->GetTexture((aiTextureType)textureType, 0, &texturePath) == aiReturn_SUCCESS)
        {
            int width = 1;
            int height = 1;
            int componentsCount = 4;
            
            uint8_t* image_data = nullptr;
            
            //texture is embedded
            if (const aiTexture* assimpTexture = assimpModel->GetEmbeddedTexture(texturePath.C_Str()))
            {
                image_data = stbi_load_from_memory((uint8_t const*)assimpTexture->pcData, assimpTexture->mWidth, &width, &height, &componentsCount, 4);
            }
            //texture is external
            else
            {
                std::filesystem::path pathObj(path);
                std::filesystem::path parentPath = pathObj.parent_path();
                auto externalTexturePath = parentPath.append(texturePath.C_Str());
                image_data = stbi_load(externalTexturePath.string().c_str(), &width, &height, &componentsCount, 4);
            }

            TextureFormat format = TextureFormat::R8G8B8A8_UNORM;
        
            // if(componentsCount == 3)
            // {
            //     format = TextureFormat::R32G32B32_FLOAT; //TODO: check if its workable
            // }

            return Renderer::CreateTexture(name, width, height, format, image_data);;
        }

        return nullptr;
    }

    Material* CreateMaterial(String path, const aiScene* assimpModel, aiMaterial* assimpMaterial)
    {
        auto diffuse = CreateTexture(path, "DiffuseTexture", DIFFUSE, assimpModel, assimpMaterial);
        auto normal = CreateTexture(path, "NormalTexture", NORMALS, assimpModel, assimpMaterial);
        // auto metal = CreateTexture(path, "MetalTexture", METALNESS, assimpModel, assimpMaterial);
        // auto roughness = CreateTexture(path, "RoughnessTexture", DIFFUSE_ROUGHNESS, assimpModel, assimpMaterial);
        auto metalRoughness = CreateTexture(path, "MetalRoughnessTexture", DIFFUSE_ROUGHNESS, assimpModel, assimpMaterial);
        return new Material(diffuse, normal, metalRoughness);
    }

    Material* CreateDummyMaterial(Texture2D* dummyTexture)
    {
        return new Material(dummyTexture, dummyTexture, dummyTexture);
    }

    ModelImporter::ModelImporter()
    {
        DummyTexture = CreateDummyTexture("DummyTexture");
    }

    CModel* ModelImporter::Import(String path, bool relative)
    {
        CModel* result = new CModel();
        
        auto assimpModel = ImportInternal(path, ModelImportFlags::CalcTangentSpace | ModelImportFlags::Triangulate | ModelImportFlags::GenBoundingBoxes | ModelImportFlags::FlipUVs | ModelImportFlags::MakeLeftHanded | ModelImportFlags::FlipWindingOrder, relative);

        if(assimpModel)
        {
            for (int i = 0; i < assimpModel->mNumMeshes; ++i)
            {
                auto assimpMesh = assimpModel->mMeshes[i];
                
                WArray<uint32_t> indexBufferData;
                
                for (int j = 0; j < assimpMesh->mNumFaces; ++j)
                {
                    for (int k = 0; k < assimpMesh->mFaces[j].mNumIndices; ++k)
                    {
                        indexBufferData.Add(assimpMesh->mFaces[j].mIndices[k]);
                    }
                }

                WArray<Vertex> vertexBufferData;
                WArray<Vector3> positions;

                for (uint32_t j = 0; j < assimpMesh->mNumVertices; ++j)
                {
                    Vertex vertex = {};
                    vertex.Position = Vector3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
                    vertex.Normal = Vector3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);
                    vertex.Tangent = Vector3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z);
                    vertex.Bitangent = Vector3(assimpMesh->mBitangents[j].x, assimpMesh->mBitangents[j].y, assimpMesh->mBitangents[j].z);
                    vertex.MeshId = i;

                    if(assimpMesh->HasTextureCoords(0))
                    {
                        vertex.UV = Vector2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y);
                    }
                    else
                    {
                        vertex.UV = Vector2(0, 0);
                    }

                    positions.Add(vertex.Position);
                    vertexBufferData.Add(vertex);
                }

                auto material = assimpModel->mMaterials[assimpMesh->mMaterialIndex];
                Material* mat = CreateMaterial(path, assimpModel, material);

                Matrix4 modelMatrix;
                if(assimpModel->mRootNode->mNumChildren > 0)
                {
                    modelMatrix = AssimpToMatrix4(assimpModel->mRootNode->mChildren[i]->mTransformation);
                }
                else
                {
                    modelMatrix = AssimpToMatrix4(assimpModel->mRootNode->mTransformation);
                }
                
                uint32_t vertexBufferSize = vertexBufferData.Num() * sizeof(Vertex);
                uint32_t indexBufferSize = indexBufferData.Num() * sizeof(uint32_t);
                AABB bBox { Vector3(assimpMesh->mAABB.mMin.x, assimpMesh->mAABB.mMin.y, assimpMesh->mAABB.mMin.z), Vector3(assimpMesh->mAABB.mMax.x, assimpMesh->mAABB.mMax.y, assimpMesh->mAABB.mMax.z)};
                CMesh* mesh = new CMesh(vertexBufferData.GetData(), vertexBufferSize, indexBufferData.GetData(), indexBufferSize, positions, indexBufferData, mat, bBox, assimpMesh->mName.C_Str(), modelMatrix);
                result->AddMesh(mesh);
            }
        }
        else
        {
		    WD_CORE_ERROR("Failed to load FBX model from path: {0}", path);
        }

        return result;
    }

    const aiScene* ModelImporter::ImportInternal(String& path, ModelImportFlags importFlags, bool relative)
    {
        if(relative)
        {
            path = std::filesystem::current_path().string() + "/" + path;
        }

        return AssimpImporter.ReadFile(path.c_str(), (unsigned)importFlags);
    }
}
