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

        WArray<Asset*> ImportTo(const Path& from, const Path& to, bool relative = true) override;
    };
}
