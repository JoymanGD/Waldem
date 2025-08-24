#pragma once

namespace Waldem
{
    class CMesh;
    class Texture2D;
    
    struct WALDEM_API CModel
    {
        CModel() = default;
        CModel(std::vector<CMesh*> meshes) : Meshes(meshes) {}

        std::vector<CMesh*> GetMeshes() { return Meshes; }
        void AddMesh(CMesh* mesh) { Meshes.push_back(mesh); }

        std::vector<Texture2D*> GetTextures();
    private:
        std::vector<CMesh*> Meshes;
    };
}
