#pragma once

#include "Waldem/Interfaces/Importer.h"
#include "Waldem/Renderer/Texture.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    class Texture2D;

    class CImageImporter : public IImporter<TextureDesc>
    {
    public:
        virtual ~CImageImporter() override = default;

        virtual TextureDesc* Import(const Path& path, bool relative = true) override;
    };
}
