#include "wdpch.h"
#include "FBXImporter.h"
#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>

#include "stb_image.h"
#include "Waldem/Renderer/Pipeline.h"
#include "Waldem/Renderer/Model/Texture.h"

Waldem::Model* Waldem::FBXImporter::Import(std::string& path, bool relative)
{
    Model* result = new Model();

    BufferLayout bufferLayout = {
        { ShaderDataType::Float3, "Position", false },
        { ShaderDataType::Float3, "Normal", false },
        { ShaderDataType::Float4, "Color", false },
        { ShaderDataType::Float3, "UV", false },
    };
    
    auto assimpModel = ImportInternal(path, ModelImportFlags::CalcTangentSpace | ModelImportFlags::GenUVCoords | ModelImportFlags::Triangulate | ModelImportFlags::PreTransformVertices | ModelImportFlags::FlipWindingOrder | ModelImportFlags::FlipUVs, relative);

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
                vertex.Position = glm::vec3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
                vertex.Normal = glm::vec3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);
                vertex.Color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

                if(assimpMesh->HasTextureCoords(0))
                {
                    vertex.UV = glm::vec3(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y, assimpMesh->mTextureCoords[0][j].z);
                }
                else
                {
                    vertex.UV = glm::vec3(0, 0, 0);
                }
                
                vertexData.push_back(vertex);
            }

            auto material = assimpModel->mMaterials[i];

            uint8_t const* image_data = nullptr;

            int width = 0;
            int height = 0;
            int componentsCount = 0;

            aiString texturePath;

            // determine if this is a PBR material or not

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS)
            {
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
                glm::vec4 fakeData = glm::vec4(1.f, .5f, 0.f, 1.f);
                image_data = (uint8_t*)&fakeData;
            }

            Texture* texture = new Texture(width, height, componentsCount, image_data);
            Material mat(texture);

            Mesh* mesh = new Mesh(vertexData.data(), vertexData.size() * sizeof(Vertex), indices.data(), indices.size(), bufferLayout, mat);

            result->AddMesh(mesh);
        }
    }

    return result;
}
        