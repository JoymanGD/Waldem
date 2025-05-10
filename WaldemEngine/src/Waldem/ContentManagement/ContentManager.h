#pragma once
#include "AudioImporter.h"
#include "ImageImporter.h"
#include "ModelImporter.h"

namespace Waldem
{
    class WALDEM_API CContentManager
    {
    public:
        bool ImportAsset(const Path& path);
    private:
        CModelImporter ModelImporter;
        CImageImporter ImageImporter;
        CAudioImporter AudioImporter;
    };
}
