#pragma once

#include "Waldem/Interfaces/Importer.h"
#include "Waldem/Renderer/Texture.h"

namespace Waldem
{
    class ImageImporter : public IImporter<Texture2D>
    {
    public:
        virtual ~ImageImporter() override = default;

        virtual Texture2D* Import(String path, bool relative = true) override;
    };
}
