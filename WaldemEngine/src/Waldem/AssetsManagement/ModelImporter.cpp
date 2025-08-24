#include "wdpch.h"
#include "ModelImporter.h"
#include "Waldem/Renderer/Texture.h"
#include "Waldem/Renderer/Model/Model.h"
#include <filesystem>
#include <fstream>
#include "assimp/scene.h"
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
    
    Texture2D* CreateDummyTexture(WString name)
    {
        Vector4 dummyColor = Vector4(1.f, 1.f, 1.f, 1.f);
        uint8_t* image_data = (uint8_t*)&dummyColor;

        return Renderer::CreateTexture(name, 1, 1, TextureFormat::R8G8B8A8_UNORM, image_data);
    }

    TextureDesc* CreateTextureDesc(const Path& path, WString name, TextureType textureType, const aiScene* assimpModel, aiMaterial* assimpMaterial)
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
                Path parentPath = path.parent_path();
                auto externalTexturePath = parentPath.append(texturePath.C_Str());
                image_data = stbi_load(externalTexturePath.string().c_str(), &width, &height, &componentsCount, 4);
            }

            return new TextureDesc(name, width, height, TextureFormat::R8G8B8A8_UNORM, image_data);
        }

        return nullptr;
    }

    Material* CreateMaterial(WArray<Asset*>& assets, const Path& from, const Path& to, const aiScene* assimpModel, aiMaterial* assimpMaterial)
    {
        auto diffuse = CreateTextureDesc(from, "DiffuseTexture", DIFFUSE, assimpModel, assimpMaterial);
        auto normal = CreateTextureDesc(from, "NormalTexture", NORMALS, assimpModel, assimpMaterial);
        auto metalRoughness = CreateTextureDesc(from, "MetalRoughnessTexture", DIFFUSE_ROUGHNESS, assimpModel, assimpMaterial);

        auto folder = from.stem();
        Path relativePath = std::filesystem::relative(to, CONTENT_PATH).string();
        relativePath /= folder;

        TextureReference diffuseRef("Empty");
        TextureReference normalRef("Empty");
        TextureReference metalRoughnessRef("Empty");

        if(diffuse)
        {
            assets.Add(diffuse);
            diffuseRef.Reference = relativePath / diffuse->Name.ToString();
        }

        if(normal)
        {
            assets.Add(normal);
            diffuseRef.Reference = relativePath / normal->Name.ToString();
        }

        if(metalRoughness)
        {
            assets.Add(metalRoughness);
            diffuseRef.Reference = relativePath / metalRoughness->Name.ToString();
        }
        
        Vector4 albedo = Vector4(1.0f);
        if (aiGetMaterialColor(assimpMaterial, AI_MATKEY_BASE_COLOR, (aiColor4D*)&albedo) != AI_SUCCESS)
        {
            WD_CORE_INFO("No base color stored in material for mesh: {0}. Using default value.", from.string());
        }

        float roughness = 1.0f;
        if(aiGetMaterialFloat(assimpMaterial, AI_MATKEY_ROUGHNESS_FACTOR, &roughness) != AI_SUCCESS)
        {
            WD_CORE_INFO("No roughness factor stored in material for mesh: {0}. Using default value.", from.string());
        }

        float metallic = 0.0f;
        if(aiGetMaterialFloat(assimpMaterial, AI_MATKEY_METALLIC_FACTOR, &metallic) != AI_SUCCESS)
        {
            WD_CORE_INFO("No metallic factor stored in material for mesh: {0}. Using default value.", from.string());
        }

        return new Material(WString(assimpMaterial->GetName().C_Str()), diffuseRef, normalRef, metalRoughnessRef, albedo, roughness, metallic);
    }

    WArray<Asset*> CModelImporter::Import(const Path& from, Path& to, bool relative)
    {
        WArray<Asset*> assets;
        
        auto assimpModel = ImportInternal(from, ModelImportFlags::CalcTangentSpace | ModelImportFlags::Triangulate | ModelImportFlags::GenBoundingBoxes | ModelImportFlags::FlipUVs | ModelImportFlags::MakeLeftHanded | ModelImportFlags::FlipWindingOrder, relative);

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

                for (uint32_t j = 0; j < assimpMesh->mNumVertices; ++j)
                {
                    Vertex vertex = {};
                    vertex.Position = Vector3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
                    vertex.Normal = Vector3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);
                    vertex.Tangent = Vector3(assimpMesh->mTangents[j].x, assimpMesh->mTangents[j].y, assimpMesh->mTangents[j].z);
                    vertex.Bitangent = Vector3(assimpMesh->mBitangents[j].x, assimpMesh->mBitangents[j].y, assimpMesh->mBitangents[j].z);
                    
                    if(assimpMesh->HasVertexColors(0))
                    {
                        vertex.Color = Vector4(assimpMesh->mColors[0][j].r, assimpMesh->mColors[0][j].g, assimpMesh->mColors[0][j].b, assimpMesh->mColors[0][j].a);
                    }
                    else
                    {
                        vertex.Color = Vector4(1.0f);
                    }

                    if(assimpMesh->HasTextureCoords(0))
                    {
                        vertex.UV = Vector2(assimpMesh->mTextureCoords[0][j].x, assimpMesh->mTextureCoords[0][j].y);
                    }
                    else
                    {
                        vertex.UV = Vector2(0, 0);
                    }

                    vertexBufferData.Add(vertex);
                }

                auto material = assimpModel->mMaterials[assimpMesh->mMaterialIndex];
                Material* mat = CreateMaterial(assets, from, to, assimpModel, material);
                assets.Add(mat);

                Matrix4 modelMatrix;
                if(assimpModel->mRootNode->mNumChildren > 0)
                {
                    modelMatrix = AssimpToMatrix4(assimpModel->mRootNode->mChildren[i]->mTransformation);
                }
                else
                {
                    modelMatrix = AssimpToMatrix4(assimpModel->mRootNode->mTransformation);
                }
                
                AABB bBox { Vector3(assimpMesh->mAABB.mMin.x, assimpMesh->mAABB.mMin.y, assimpMesh->mAABB.mMin.z), Vector3(assimpMesh->mAABB.mMax.x, assimpMesh->mAABB.mMax.y, assimpMesh->mAABB.mMax.z)};
                WString dirtyName = WString(assimpMesh->mName.C_Str());
                WString name = SanitizeString(dirtyName);
                
                auto folder = from.stem();
                Path relativePath = std::filesystem::relative(to, CONTENT_PATH).string();
                relativePath /= folder;
                relativePath.append(mat->Name.ToString());
                MaterialReference materialRef(relativePath);
                
                CMesh* mesh = new CMesh(vertexBufferData, indexBufferData, materialRef, bBox, name, modelMatrix);
                assets.Add(mesh);
            }
        }
        else
        {
		    WD_CORE_ERROR("Failed to load FBX model from path: {0}", from);
        }

        return assets;
    }

    const aiScene* CModelImporter::ImportInternal(const Path& path, ModelImportFlags importFlags, bool relative)
    {
        return AssimpImporter.ReadFile(path.string(), (unsigned)importFlags);
    }
}
