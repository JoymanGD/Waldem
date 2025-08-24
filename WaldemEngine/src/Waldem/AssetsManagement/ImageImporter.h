#pragma once

#include "Waldem/Interfaces/Importer.h"

namespace Waldem
{
    class TextureDesc;
    class Texture2D;

    class CImageImporter : public IImporter
    {
    public:
        virtual ~CImageImporter() override = default;

        WArray<Asset*> Import(const Path& from, Path& to, bool relative = true) override;
    };
}
