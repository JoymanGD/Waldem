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

    Texture2D* CreateTexture(String path, String name, TextureType textureType, const aiScene* assimpModel, aiMaterial* assimpMaterial)
    {
        uint8_t* image_data;

        int width = 0;
        int height = 0;
        int componentsCount = 0;

        aiString texturePath;
        
        if (assimpMaterial->GetTexture((aiTextureType)textureType, 0, &texturePath) == aiReturn_SUCCESS)
        {
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
        }
        else
        {
            width = 1;
            height = 1;

            // Fake orange texture data for the case when texture is not found
            Vector4 fakeData = normalize(Vector4(1.f, .2f, 0.f, 1.f));
            image_data = (uint8_t*)&fakeData;
        }

        TextureFormat format;
        
        if(componentsCount == 3)
        {
            format = TextureFormat::R32G32B32_FLOAT; //TODO: check if its workable
        }
        else
        {
            format = TextureFormat::R8G8B8A8_UNORM;
        }

        auto texture = Renderer::CreateTexture(name, width, height, format, image_data);

        return texture;
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
    
    Model* ModelImporter::Import(String path, bool relative)
    {
        Model* result = new Model();
        
        auto assimpModel = ImportInternal(path, ModelImportFlags::CalcTangentSpace | ModelImportFlags::Triangulate | ModelImportFlags::GenBoundingBoxes | ModelImportFlags::FlipUVs | ModelImportFlags::MakeLeftHanded | ModelImportFlags::FlipWindingOrder, relative);

        if(assimpModel)
        {
            for (int i = 0; i < assimpModel->mNumMeshes; ++i)
            {
                auto assimpMesh = assimpModel->mMeshes[i];

                std::vector<uint32_t> indexBufferData;
                
                for (int j = 0; j < assimpMesh->mNumFaces; ++j)
                {
                    for (int k = 0; k < assimpMesh->mFaces[j].mNumIndices; ++k)
                    {
                        indexBufferData.push_back(assimpMesh->mFaces[j].mIndices[k]);
                    }
                }

                std::vector<Vertex> vertexBufferData;

                for (uint32_t j = 0; j < assimpMesh->mNumVertices; ++j)
                {
                    Vertex vertex = {};
                    vertex.Position = Vector3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
                    vertex.Normal = Vector3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);
                    vertex.Tangent = Vector3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z);
                    vertex.MeshId = i;

                    if(assimpMesh->HasTextureCoords(0))
                    {
                        vertex.UV = Vector2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y);
                    }
                    else
                    {
                        vertex.UV = Vector2(0, 0);
                    }
                    
                    vertexBufferData.push_back(vertex);
                }

                auto material = assimpModel->mMaterials[i];
                auto mat = CreateMaterial(path, assimpModel, material);

                uint32_t vertexBufferSize = vertexBufferData.size() * sizeof(Vertex);
                uint32_t indexBufferSize = indexBufferData.size() * sizeof(uint32_t);
                BoundingBox bBox { Vector3(assimpMesh->mAABB.mMin.x, assimpMesh->mAABB.mMin.y, assimpMesh->mAABB.mMin.z), Vector3(assimpMesh->mAABB.mMax.x, assimpMesh->mAABB.mMax.y, assimpMesh->mAABB.mMax.z)};
                Mesh* mesh = new Mesh(vertexBufferData.data(), vertexBufferSize, indexBufferData.data(), indexBufferSize, mat, bBox);
                if(assimpModel->mRootNode->mNumChildren > 0)
                {
                    mesh->ObjectMatrix = AssimpToMatrix4(assimpModel->mRootNode->mChildren[i]->mTransformation);
                }
                else
                {
                    mesh->ObjectMatrix = AssimpToMatrix4(assimpModel->mRootNode->mTransformation);
                }
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
