#include "wdpch.h"
#include "Mono.h"
#include <filesystem>
#include <fstream>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/class.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>

#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    namespace
    {
        bool ShouldSuspendMonoDebugger()
        {
#if WD_DEBUG
            size_t valueLength = 0;
            if(getenv_s(&valueLength, nullptr, 0, "WD_MONO_SUSPEND") != 0 || valueLength == 0)
            {
                return false;
            }

            char* envValue = nullptr;
            if(_dupenv_s(&envValue, &valueLength, "WD_MONO_SUSPEND") != 0 || envValue == nullptr)
            {
                return false;
            }

            const bool shouldSuspend = envValue[0] == '1' || envValue[0] == 't' || envValue[0] == 'T' || envValue[0] == 'y' || envValue[0] == 'Y';
            free(envValue);
            return shouldSuspend;
#else
            return false;
#endif
        }

        void CreateDebugDomain(MonoDomain* domain)
        {
#if WD_DEBUG
            if(domain != nullptr)
            {
                mono_debug_domain_create(domain);
            }
#else
            (void)domain;
#endif
        }

        void UnloadDebugDomain(MonoDomain* domain)
        {
#if WD_DEBUG
            if(domain != nullptr)
            {
                mono_debug_domain_unload(domain);
            }
#else
            (void)domain;
#endif
        }

    }

    void Mono::Initialize()
    {
#if WD_DEBUG
        const bool suspendForDebugger = ShouldSuspendMonoDebugger();
        const char* options[] = {
            "--soft-breakpoints",
            suspendForDebugger
                ? "--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=y"
                : "--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=n"
        };

        mono_jit_parse_options(2, (char**)options);
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
        WD_CORE_INFO("Mono debugger agent listening on 127.0.0.1:55555 (suspend={0})", suspendForDebugger ? "y" : "n");
#endif

        const Path currentPath = GetCurrentFolder();
        const Path monoLibPath = currentPath / "mono" / "lib" / "4.5";
        
        //Init mono
        mono_set_assemblies_path(monoLibPath.string().c_str());

        MonoRootDomain = mono_jit_init("WaldemScriptRuntime");
        
        if (MonoRootDomain == nullptr)
        {
            WD_CORE_ERROR("Failed to initialize Mono runtime");
            return;
        }

        CreateDebugDomain(MonoRootDomain);

        // Create an App Domain
        MonoAppDomain = mono_domain_create_appdomain((char*)"WaldemAppDomain", nullptr);
        CreateDebugDomain(MonoAppDomain);
        mono_domain_set(MonoAppDomain, true);
    }

    void Mono::Shutdown()
    {
        if(MonoAppDomain != nullptr)
        {
            mono_domain_set(MonoRootDomain, false);
            MonoAppDomain = nullptr;
        }

        if(MonoRootDomain != nullptr)
        {
            mono_jit_cleanup(MonoRootDomain);
            MonoRootDomain = nullptr;
        }

#if WD_DEBUG
        mono_debug_cleanup();
#endif
    }

    bool Mono::ReloadAppDomain()
    {
        if(MonoRootDomain == nullptr)
        {
            return false;
        }

        if(MonoAppDomain != nullptr)
        {
            mono_thread_attach(MonoRootDomain);
            mono_domain_set(MonoRootDomain, false);
            MonoAppDomain = nullptr;
        }

        static int domainCounter = 0;
        std::string domainName = "WaldemAppDomain_" + std::to_string(++domainCounter);
        MonoAppDomain = mono_domain_create_appdomain((char*)domainName.c_str(), nullptr);
        if(MonoAppDomain == nullptr)
        {
            WD_CORE_ERROR("Failed to recreate Mono app domain");
            return false;
        }

        CreateDebugDomain(MonoAppDomain);
        mono_domain_set(MonoAppDomain, true);
        WD_CORE_WARN("Reloaded scripts by creating a fresh Mono app domain without unloading the previous one");
        return true;
    }

    char* ReadBytes(const Path& filepath, uint32_t* outSize)
    {
        std::ifstream stream(filepath, std::ios::binary | std::ios::ate);
    
        if (!stream)
        {
            // Failed to open the file
            return nullptr;
        }

        std::streampos end = stream.tellg();
        stream.seekg(0, std::ios::beg);
        uint32_t size = end - stream.tellg();
    
        if (size == 0)
        {
            // File is empty
            return nullptr;
        }

        char* buffer = new char[size];
        stream.read((char*)buffer, size);
        stream.close();

        *outSize = size;
        return buffer;
    }
    
    MonoAssembly* Mono::LoadCSharpAssembly(const Path& assemblyPath)
    {
        MonoImageOpenStatus status;
        MonoAssembly* assembly = mono_assembly_open_full(assemblyPath.string().c_str(), &status, 0);
        if(assembly == nullptr || status != MONO_IMAGE_OK)
        {
            const char* errorMessage = mono_image_strerror(status);
            WD_CORE_ERROR("Failed to load assembly from file {0}: {1}",
                assemblyPath.string(),
                errorMessage != nullptr ? errorMessage : "unknown");
            return nullptr;
        }

#if WD_DEBUG
        WD_CORE_INFO("Loaded managed assembly for debugging: {0}", assemblyPath.string());

        Path pdbPath = assemblyPath;
        pdbPath.replace_extension(".pdb");
        if(exists(pdbPath))
        {
            uint32_t pdbSize = 0;
            char* pdbData = ReadBytes(pdbPath, &pdbSize);
            if(pdbData != nullptr && pdbSize > 0)
            {
                MonoImage* assemblyImage = mono_assembly_get_image(assembly);
                if(assemblyImage != nullptr)
                {
                    mono_debug_open_image_from_memory(assemblyImage, (const mono_byte*)pdbData, (int)pdbSize);
                    WD_CORE_INFO("Loaded managed debug symbols: {0}", pdbPath.string());
                }
                delete[] pdbData;
            }
            else
            {
                WD_CORE_WARN("Failed to read debug symbols from {0}", pdbPath.string());
            }
        }
        else
        {
            WD_CORE_WARN("No debug symbols found for managed assembly: {0}", pdbPath.string());
        }
#endif

        return assembly;
    }

    void Mono::PrintAssemblyTypes(MonoAssembly* assembly)
    {
        MonoImage* image = mono_assembly_get_image(assembly);
        const MonoTableInfo* typeDefinitionsTable = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
        int32_t numTypes = mono_table_info_get_rows(typeDefinitionsTable);

        for (int32_t i = 0; i < numTypes; i++)
        {
            uint32_t cols[MONO_TYPEDEF_SIZE];
            mono_metadata_decode_row(typeDefinitionsTable, i, cols, MONO_TYPEDEF_SIZE);

            const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
            const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

            WD_CORE_INFO("{0}.{1}", nameSpace, name);
        }
    }

    MonoClass* Mono::GetClass(MonoAssembly* assembly, const WString& className, const WString& namespaceName)
    {
        if(assembly == nullptr)
        {
            return nullptr;
        }

        MonoImage* image = mono_assembly_get_image(assembly);
        MonoClass* klass = mono_class_from_name(image, namespaceName.C_Str(), className.C_Str());

        if (klass == nullptr)
        {
            // Log error here
            return nullptr;
        }

        return klass;
    }

    MonoObject* Mono::CreateObject(MonoClass* klass)
    {
        if(klass == nullptr || MonoAppDomain == nullptr)
        {
            return nullptr;
        }

        MonoObject* instance = mono_object_new(MonoAppDomain, klass);

        if (instance == nullptr)
        {
            WD_CORE_ERROR("Failed to create instance of class");
        }
        
        mono_runtime_object_init(instance);
        return instance;
    }

    MonoMethod* Mono::GetMethod(MonoClass* klass, const char* methodName, int parameterCount)
    {
        if(klass == nullptr)
        {
            return nullptr;
        }

        for(MonoClass* currentClass = klass; currentClass != nullptr; currentClass = mono_class_get_parent(currentClass))
        {
            if(MonoMethod* method = mono_class_get_method_from_name(currentClass, methodName, parameterCount))
            {
                return method;
            }
        }

        return nullptr;
    }

    MonoObject* Mono::InvokeMethod(MonoObject* instance, MonoMethod* method, void** params)
    {
        if(method == nullptr || instance == nullptr)
        {
            return nullptr;
        }

        MonoObject* exception = nullptr;
        MonoObject* result = mono_runtime_invoke(method, instance, params, &exception);
        if(exception != nullptr)
        {
            MonoString* exceptionString = mono_object_to_string(exception, nullptr);
            char* exceptionChars = mono_string_to_utf8(exceptionString);
            WD_CORE_ERROR("Managed exception: {0}", exceptionChars != nullptr ? exceptionChars : "unknown");
            mono_free(exceptionChars);
        }

        return result;
    }

    void Mono::RegisterInternalCall(const char* name, const void* method)
    {
        mono_add_internal_call(name, method);
    }
}
