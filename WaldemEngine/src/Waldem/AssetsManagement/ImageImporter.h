#pragma once

#include "Waldem/Interfaces/Importer.h"
#include "Waldem/Types/String.h"

namespace Waldem
{
    class Texture2D;

    class CImageImporter : public IImporter<Texture2D>
    {
    public:
        virtual ~CImageImporter() override = default;

        virtual Texture2D* Import(const Path& path, bool relative = true) override;
    };
}
