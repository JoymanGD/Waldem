#pragma once

#include "Waldem/Interfaces/Importer.h"

namespace Waldem
{
    class AudioClip;

    class CAudioImporter : public IImporter
    {
    public:
        ~CAudioImporter() override = default;

        CAudioImporter();

        WArray<Asset*> Import(const Path& from, Path& to, bool relative = true) override;
    };
}
