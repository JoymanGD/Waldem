#pragma once
#include <mono/metadata/image.h>
#include <mono/utils/mono-forward.h>

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
        MonoAssembly* LoadCSharpAssembly(String assemblyPath);
        MonoClass* GetClass(MonoAssembly* assembly, const String& className, const String& namespaceName = "");
        MonoObject* CreateObject(MonoClass* klass);
    };
}
