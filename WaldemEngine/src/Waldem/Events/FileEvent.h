#pragma once

#include "Event.h"

namespace Waldem
{
    class WALDEM_API FileDroppedEvent : public Event
    {
    public:
        FileDroppedEvent(Path path) : Path(path) {}
        
        inline Path& GetPath() { return Path; }

        WString ToString() const override
        {
            std::stringstream ss;
            ss << "FileDroppedEvent: " << Path;
            return ss.str();
        }

        EVENT_CLASS_TYPE(FileDropped)
        EVENT_CLASS_CATEGORY(EventCategoryFile)
    private:
        Path Path;
    };
}
