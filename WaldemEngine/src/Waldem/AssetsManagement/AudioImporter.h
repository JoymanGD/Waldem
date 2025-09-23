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

        WArray<Asset*> ImportTo(const Path& from, const Path& to, bool relative = true) override;
    };
}
