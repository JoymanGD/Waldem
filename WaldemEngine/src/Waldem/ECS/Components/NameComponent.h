#pragma once

#define MAX_ENTITY_NAME_SIZE 128

namespace Waldem
{
    struct WALDEM_API NameComponent
    {
        WString Name;
        NameComponent() : Name("DefaultEntity") {}
        NameComponent(WString name) : Name(name) {}
    };
}
