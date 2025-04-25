#pragma once
#include <mono/metadata/image.h>
#include <mono/utils/mono-forward.h>

#include "Waldem/Types/String.h"

namespace Waldem
{
    class WALDEM_API Mono
    {
    private:
        MonoDomain* MonoRootDomain = nullptr;
        MonoDomain* MonoAppDomain = nullptr;
        void PrintAssemblyTypes(MonoAssembly* assembly);
    public:
        void Initialize();
        MonoAssembly* LoadCSharpAssembly(const Path& assemblyPath);
        MonoClass* GetClass(MonoAssembly* assembly, const WString& className, const WString& namespaceName = "");
        MonoObject* CreateObject(MonoClass* klass);
    };
}
