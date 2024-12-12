#include "wdpch.h"
#include "ModelImporter.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Waldem/Renderer/Texture.h"
#include <filesystem>
#include <fstream>

#include "Waldem/Application.h"

namespace Waldem
{
    Model* ModelImporter::Import(String path, bool relative)
    {
        Model* result = new Model();

        BufferLayout bufferLayout = {
            { ShaderDataType::Float3, "Position", false },
            { ShaderDataType::Float3, "Normal", false },
            { ShaderDataType::Float2, "UV", true },
            { ShaderDataType::Int, "MeshId", true },
        };
        
        auto assimpModel = ImportInternal(path, ModelImportFlags::CalcTangentSpace | ModelImportFlags::Triangulate | ModelImportFlags::GenBoundingBoxes | ModelImportFlags::PreTransformVertices | ModelImportFlags::FlipUVs | ModelImportFlags::MakeLeftHanded | ModelImportFlags::FlipWindingOrder, relative);

        if(assimpModel)
        {
            for (int i = 0; i < assimpModel->mNumMeshes; ++i)
            {
                auto assimpMesh = assimpModel->mMeshes[i];

                std::vector<uint32_t> indices;
                
                for (int j = 0; j < assimpMesh->mNumFaces; ++j)
                {
                    for (int k = 0; k < assimpMesh->mFaces[j].mNumIndices; ++k)
                    {
                        indices.push_back(assimpMesh->mFaces[j].mIndices[k]);
                    }
                }

                std::vector<Vertex> vertexData;

                for (uint32_t j = 0; j < assimpMesh->mNumVertices; ++j)
                {
                    Vertex vertex = {};
                    vertex.Position = Vector3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
                    vertex.Normal = Vector3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);
                    vertex.MeshId = i;

                    if(assimpMesh->HasTextureCoords(0))
                    {
                        vertex.UV = Vector2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y);
                    }
                    else
                    {
                        vertex.UV = Vector2(0, 0);
                    }
                    
                    vertexData.push_back(vertex);
                }

                auto material = assimpModel->mMaterials[i];

                uint8_t* image_data = nullptr;

                int width = 0;
                int height = 0;
                int componentsCount = 0;

                aiString texturePath;

                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
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
                    Vector4 fakeData = Vector4(1.f, .5f, 0.f, 1.f);
                    image_data = (uint8_t*)&fakeData;
                }

                TextureFormat format;
                
                if(componentsCount == 3)
                {
                    format = TextureFormat::TEXTURE_FORMAT_R32G32B32_FLOAT; //TODO: check if its workable
                }
                else
                {
                    format = TextureFormat::TEXTURE_FORMAT_R8G8B8A8_UNORM;
                }

                Texture2D* texture = Application::GetRenderer().CreateTexture("DiffuseTexture", width, height, format, image_data);
                Material mat(texture);

                uint32_t vertexBufferSize = vertexData.size() * sizeof(Vertex);
                BoundingBox bBox { Vector3(assimpMesh->mAABB.mMin.x, assimpMesh->mAABB.mMin.y, assimpMesh->mAABB.mMin.z), Vector3(assimpMesh->mAABB.mMax.x, assimpMesh->mAABB.mMax.y, assimpMesh->mAABB.mMax.z)};

                Mesh* mesh = new Mesh(vertexData.data(), vertexBufferSize, indices.data(), indices.size(), mat, bBox);

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
