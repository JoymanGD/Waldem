#pragma once
#include "ModelImporter.h"
#include "Waldem/Renderer/Model/Model.h"

namespace Waldem
{
    class FBXImporter : public ModelImporter
    {
    public:
        FBXImporter() = default;
        ~FBXImporter() override = default;
        
        Model* Import(std::string& path, bool relative = true) override; 
    };
}
