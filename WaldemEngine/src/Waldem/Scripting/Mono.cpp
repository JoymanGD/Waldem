#include "wdpch.h"
#include "Mono.h"
#include <filesystem>
#include <fstream>
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include "Waldem/Utils/FileUtils.h"

namespace Waldem
{
    void Mono::Initialize()
    {
        auto currentPath = GetCurrentFolder().append("/");
        
        //Init mono
        mono_set_assemblies_path(currentPath.append("/mono/lib/4.5").string().c_str());

        MonoRootDomain = mono_jit_init("WaldemScriptRuntime");
        
        if (MonoRootDomain == nullptr)
        {
            WD_CORE_ERROR("Failed to initialize Mono runtime");
            return;
        }

        // Create an App Domain
        MonoAppDomain = mono_domain_create_appdomain((char*)"WaldemAppDomain", nullptr);
        mono_domain_set(MonoAppDomain, true);

        auto testAssembly = LoadCSharpAssembly(currentPath.append("ScriptEngine.dll"));
        auto monoClass = GetClass(testAssembly, "CSharpTesting");
        auto monoObject = CreateObject(monoClass);
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
        uint32_t fileSize = 0;
        char* fileData = ReadBytes(assemblyPath, &fileSize);

        // NOTE: We can't use this image for anything other than loading the assembly because this image doesn't have a reference to the assembly
        MonoImageOpenStatus status;
        MonoImage* image = mono_image_open_from_data_full(fileData, fileSize, 1, &status, 0);

        if (status != MONO_IMAGE_OK)
        {
            const char* errorMessage = mono_image_strerror(status);
            WD_CORE_ERROR("Failed to load assembly: {0}", errorMessage);
            return nullptr;
        }

        MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath.string().c_str(), &status, 0);
        mono_image_close(image);
    
        // Don't forget to free the file data
        delete[] fileData;

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
        MonoObject* instance = mono_object_new(MonoAppDomain, klass);

        if (instance == nullptr)
        {
            WD_CORE_ERROR("Failed to create instance of class");
        }
        
        mono_runtime_object_init(instance);
        return instance;
    }
}
