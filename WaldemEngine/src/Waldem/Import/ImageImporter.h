#pragma once

#include "Waldem/Interfaces/Importer.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    class Texture2D;

    class ImageImporter : public IImporter<Texture2D>
    {
    public:
        virtual ~ImageImporter() override = default;

        virtual Texture2D* Import(const Path& path, bool relative = true) override;
    };
}
