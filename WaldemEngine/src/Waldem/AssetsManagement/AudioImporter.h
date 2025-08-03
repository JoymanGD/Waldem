#pragma once

#include "Waldem/Interfaces/Importer.h"
#include "Waldem/Audio/AudioClip.h"

namespace Waldem
{
    class CAudioImporter : public IImporter<AudioClip>
    {
    public:
        virtual ~CAudioImporter() override = default;

        CAudioImporter();

        virtual AudioClip* Import(const Path& path, bool relative = true) override;
    };
}
