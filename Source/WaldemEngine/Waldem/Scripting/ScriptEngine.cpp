#include "wdpch.h"
#include "ScriptEngine.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/class.h>
#include <mono/metadata/object.h>

#include "Mono.h"
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <regex>
#include <vector>
#include "Waldem/ECS/Components/ScriptComponent.h"
#include "Waldem/Utils/FileUtils.h"
#include "Bindings/AnimatorBindings.h"
#include "Bindings/CameraBindings.h"
#include "Bindings/DebugBindings.h"
#include "Bindings/EntityBindings.h"
#include "Bindings/InputBindings.h"
#include "Bindings/LightBindings.h"
#include "Bindings/QuaternionBindings.h"
#include "Bindings/RigidBodyBindings.h"
#include "Bindings/TimeBindings.h"
#include "Bindings/TransformBindings.h"

namespace Waldem
{
    namespace
    {
        struct ScriptVector3
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        struct ScriptQuaternion
        {
            float w = 1.0f;
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };

        using ScriptFieldType = ScriptEngine::ScriptFieldType;
        using ScriptFieldValue = ScriptEngine::ScriptFieldValue;
        using ScriptFieldDescriptor = ScriptEngine::ScriptFieldDescriptor;
        Path CurrentRuntimeAssemblyPath;
        uint64_t RuntimeAssemblyVersion = 0;

        std::string ToUtf8Path(const Path& path)
        {
            return path.string();
        }

        Path ResolveProjectRootPath()
        {
            Path currentFolder = GetCurrentFolder();
            return currentFolder.parent_path().parent_path().parent_path();
        }

        ScriptFieldType GetScriptFieldType(MonoType* monoType)
        {
            if(monoType == nullptr)
            {
                return ScriptFieldType::None;
            }

            switch(mono_type_get_type(monoType))
            {
            case MONO_TYPE_R4:
                return ScriptFieldType::Float;
            case MONO_TYPE_I4:
                return ScriptFieldType::Int;
            case MONO_TYPE_BOOLEAN:
                return ScriptFieldType::Bool;
            case MONO_TYPE_VALUETYPE:
            {
                MonoClass* typeClass = mono_class_from_mono_type(monoType);
                if(typeClass == nullptr)
                {
                    return ScriptFieldType::None;
                }

                const char* classNamespace = mono_class_get_namespace(typeClass);
                const char* className = mono_class_get_name(typeClass);
                if(classNamespace != nullptr && className != nullptr &&
                    strcmp(classNamespace, "Waldem") == 0 &&
                    strcmp(className, "Vector3") == 0)
                {
                    return ScriptFieldType::Vector3;
                }

                if(classNamespace != nullptr && className != nullptr &&
                    strcmp(classNamespace, "Waldem") == 0 &&
                    strcmp(className, "Quaternion") == 0)
                {
                    return ScriptFieldType::Quaternion;
                }

                return ScriptFieldType::None;
            }
            default:
                return ScriptFieldType::None;
            }
        }

        bool IsEditableScriptField(MonoClassField* field, ScriptFieldType& outType)
        {
            if(field == nullptr)
            {
                return false;
            }

            const uint32_t flags = mono_field_get_flags(field);
            if((flags & MONO_FIELD_ATTR_PUBLIC) == 0 || (flags & MONO_FIELD_ATTR_STATIC) != 0)
            {
                return false;
            }

            const char* fieldName = mono_field_get_name(field);
            if(fieldName == nullptr || fieldName[0] == '<')
            {
                return false;
            }

            outType = GetScriptFieldType(mono_field_get_type(field));
            return outType != ScriptFieldType::None;
        }

        ScriptFieldValue ReadFieldValue(MonoObject* instanceObject, MonoClassField* field, ScriptFieldType type)
        {
            ScriptFieldValue value;
            value.Type = type;

            switch(type)
            {
            case ScriptFieldType::Float:
                mono_field_get_value(instanceObject, field, &value.FloatValue);
                break;
            case ScriptFieldType::Int:
                mono_field_get_value(instanceObject, field, &value.IntValue);
                break;
            case ScriptFieldType::Bool:
                mono_field_get_value(instanceObject, field, &value.BoolValue);
                break;
            case ScriptFieldType::Vector3:
            {
                ScriptVector3 vectorValue;
                mono_field_get_value(instanceObject, field, &vectorValue);
                value.Vector3Value = Vector3(vectorValue.x, vectorValue.y, vectorValue.z);
                break;
            }
            case ScriptFieldType::Quaternion:
            {
                ScriptQuaternion quaternionValue;
                mono_field_get_value(instanceObject, field, &quaternionValue);
                value.QuaternionValue = Quaternion(quaternionValue.w, quaternionValue.x, quaternionValue.y, quaternionValue.z);
                break;
            }
            default:
                break;
            }

            return value;
        }

        void WriteFieldValue(MonoObject* instanceObject, MonoClassField* field, const ScriptFieldValue& value)
        {
            switch(value.Type)
            {
            case ScriptFieldType::Float:
                mono_field_set_value(instanceObject, field, (void*)&value.FloatValue);
                break;
            case ScriptFieldType::Int:
                mono_field_set_value(instanceObject, field, (void*)&value.IntValue);
                break;
            case ScriptFieldType::Bool:
                mono_field_set_value(instanceObject, field, (void*)&value.BoolValue);
                break;
            case ScriptFieldType::Vector3:
            {
                ScriptVector3 vectorValue;
                vectorValue.x = value.Vector3Value.x;
                vectorValue.y = value.Vector3Value.y;
                vectorValue.z = value.Vector3Value.z;
                mono_field_set_value(instanceObject, field, &vectorValue);
                break;
            }
            case ScriptFieldType::Quaternion:
            {
                ScriptQuaternion quaternionValue;
                quaternionValue.w = value.QuaternionValue.w;
                quaternionValue.x = value.QuaternionValue.x;
                quaternionValue.y = value.QuaternionValue.y;
                quaternionValue.z = value.QuaternionValue.z;
                mono_field_set_value(instanceObject, field, &quaternionValue);
                break;
            }
            default:
                break;
            }
        }

        bool TryParseFieldOverrides(const WString& overrides, std::unordered_map<std::string, ScriptFieldValue>& outValues)
        {
            outValues.clear();
            if(overrides.IsEmpty())
            {
                return true;
            }

            rapidjson::Document document;
            document.Parse(overrides.C_Str());
            if(document.HasParseError() || !document.IsArray())
            {
                return false;
            }

            for(auto& entry : document.GetArray())
            {
                if(!entry.IsObject() || !entry.HasMember("name") || !entry["name"].IsString() || !entry.HasMember("type") || !entry["type"].IsInt())
                {
                    continue;
                }

                ScriptFieldValue value;
                value.Type = (ScriptFieldType)entry["type"].GetInt();
                const std::string fieldName = entry["name"].GetString();

                switch(value.Type)
                {
                case ScriptFieldType::Float:
                    if(entry.HasMember("float") && entry["float"].IsNumber())
                    {
                        value.FloatValue = entry["float"].GetFloat();
                        outValues[fieldName] = value;
                    }
                    break;
                case ScriptFieldType::Int:
                    if(entry.HasMember("int") && entry["int"].IsInt())
                    {
                        value.IntValue = entry["int"].GetInt();
                        outValues[fieldName] = value;
                    }
                    break;
                case ScriptFieldType::Bool:
                    if(entry.HasMember("bool") && entry["bool"].IsBool())
                    {
                        value.BoolValue = entry["bool"].GetBool();
                        outValues[fieldName] = value;
                    }
                    break;
                case ScriptFieldType::Vector3:
                    if(entry.HasMember("vector3") && entry["vector3"].IsObject())
                    {
                        const auto& vectorObject = entry["vector3"];
                        if(vectorObject.HasMember("x") && vectorObject["x"].IsNumber() &&
                            vectorObject.HasMember("y") && vectorObject["y"].IsNumber() &&
                            vectorObject.HasMember("z") && vectorObject["z"].IsNumber())
                        {
                            value.Vector3Value = Vector3(vectorObject["x"].GetFloat(), vectorObject["y"].GetFloat(), vectorObject["z"].GetFloat());
                            outValues[fieldName] = value;
                        }
                    }
                    break;
                case ScriptFieldType::Quaternion:
                    if(entry.HasMember("quaternion") && entry["quaternion"].IsObject())
                    {
                        const auto& quaternionObject = entry["quaternion"];
                        if(quaternionObject.HasMember("w") && quaternionObject["w"].IsNumber() &&
                            quaternionObject.HasMember("x") && quaternionObject["x"].IsNumber() &&
                            quaternionObject.HasMember("y") && quaternionObject["y"].IsNumber() &&
                            quaternionObject.HasMember("z") && quaternionObject["z"].IsNumber())
                        {
                            value.QuaternionValue = Quaternion(
                                quaternionObject["w"].GetFloat(),
                                quaternionObject["x"].GetFloat(),
                                quaternionObject["y"].GetFloat(),
                                quaternionObject["z"].GetFloat());
                            outValues[fieldName] = value;
                        }
                    }
                    break;
                default:
                    break;
                }
            }

            return true;
        }

        WString SerializeFieldOverrides(const std::unordered_map<std::string, ScriptFieldValue>& values)
        {
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            writer.StartArray();

            for(const auto& [name, value] : values)
            {
                writer.StartObject();
                writer.Key("name");
                writer.String(name.c_str());
                writer.Key("type");
                writer.Int((int)value.Type);

                switch(value.Type)
                {
                case ScriptFieldType::Float:
                    writer.Key("float");
                    writer.Double(value.FloatValue);
                    break;
                case ScriptFieldType::Int:
                    writer.Key("int");
                    writer.Int(value.IntValue);
                    break;
                case ScriptFieldType::Bool:
                    writer.Key("bool");
                    writer.Bool(value.BoolValue);
                    break;
                case ScriptFieldType::Vector3:
                    writer.Key("vector3");
                    writer.StartObject();
                    writer.Key("x");
                    writer.Double(value.Vector3Value.x);
                    writer.Key("y");
                    writer.Double(value.Vector3Value.y);
                    writer.Key("z");
                    writer.Double(value.Vector3Value.z);
                    writer.EndObject();
                    break;
                case ScriptFieldType::Quaternion:
                    writer.Key("quaternion");
                    writer.StartObject();
                    writer.Key("w");
                    writer.Double(value.QuaternionValue.w);
                    writer.Key("x");
                    writer.Double(value.QuaternionValue.x);
                    writer.Key("y");
                    writer.Double(value.QuaternionValue.y);
                    writer.Key("z");
                    writer.Double(value.QuaternionValue.z);
                    writer.EndObject();
                    break;
                default:
                    break;
                }

                writer.EndObject();
            }

            writer.EndArray();
            return buffer.GetString();
        }

        bool CollectScriptFieldDescriptors(MonoClass* klass, MonoObject* defaultInstance, std::vector<ScriptFieldDescriptor>& outFields)
        {
            outFields.clear();
            if(klass == nullptr || defaultInstance == nullptr)
            {
                return false;
            }

            std::vector<ScriptFieldDescriptor> reversedFields;

            for(MonoClass* currentClass = klass; currentClass != nullptr; currentClass = mono_class_get_parent(currentClass))
            {
                void* iterator = nullptr;
                while(MonoClassField* field = mono_class_get_fields(currentClass, &iterator))
                {
                    ScriptFieldType fieldType = ScriptFieldType::None;
                    if(!IsEditableScriptField(field, fieldType))
                    {
                        continue;
                    }

                    ScriptFieldDescriptor descriptor;
                    descriptor.Name = mono_field_get_name(field);
                    descriptor.DefaultValue = ReadFieldValue(defaultInstance, field, fieldType);
                    reversedFields.push_back(descriptor);
                }
            }

            outFields.assign(reversedFields.rbegin(), reversedFields.rend());
            return true;
        }

        MonoClassField* FindScriptField(MonoClass* klass, const std::string& fieldName, ScriptFieldType* outType = nullptr)
        {
            for(MonoClass* currentClass = klass; currentClass != nullptr; currentClass = mono_class_get_parent(currentClass))
            {
                if(MonoClassField* field = mono_class_get_field_from_name(currentClass, fieldName.c_str()))
                {
                    ScriptFieldType fieldType = ScriptFieldType::None;
                    if(IsEditableScriptField(field, fieldType))
                    {
                        if(outType != nullptr)
                        {
                            *outType = fieldType;
                        }
                        return field;
                    }
                }
            }

            return nullptr;
        }

        Path ResolveScriptAssemblyPath()
        {
            if(!CurrentRuntimeAssemblyPath.empty() && exists(CurrentRuntimeAssemblyPath))
            {
                return CurrentRuntimeAssemblyPath;
            }

            const Path currentFolder = GetCurrentFolder();
            const Path localAssemblyPath = currentFolder / "ScriptEngine.dll";
            const Path siblingAssemblyPath = currentFolder.parent_path() / "ScriptEngine" / "ScriptEngine.dll";

            if(exists(localAssemblyPath))
            {
                return localAssemblyPath;
            }

            return siblingAssemblyPath;
        }

        std::string ReadCommandOutput(const std::string& command)
        {
            std::string output;
            FILE* pipe = _popen(command.c_str(), "r");
            if(pipe == nullptr)
            {
                return output;
            }

            char buffer[512];
            while(fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                output += buffer;
            }

            _pclose(pipe);

            while(!output.empty() && (output.back() == '\n' || output.back() == '\r'))
            {
                output.pop_back();
            }

            return output;
        }

        bool RunProcessBlocking(const std::wstring& commandLine, const Path& workingDirectory)
        {
            STARTUPINFOW startupInfo = {};
            startupInfo.cb = sizeof(startupInfo);

            PROCESS_INFORMATION processInfo = {};
            std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
            mutableCommandLine.push_back(L'\0');

            const std::wstring workingDirectoryString = workingDirectory.wstring();
            if(!CreateProcessW(
                nullptr,
                mutableCommandLine.data(),
                nullptr,
                nullptr,
                FALSE,
                CREATE_NO_WINDOW,
                nullptr,
                workingDirectoryString.c_str(),
                &startupInfo,
                &processInfo))
            {
                return false;
            }

            WaitForSingleObject(processInfo.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(processInfo.hProcess, &exitCode);

            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);

            return exitCode == 0;
        }

        Path ResolveScriptPath(const ScriptReference& scriptReference)
        {
            Path scriptPath = scriptReference.Reference;
            if(scriptPath.is_relative())
            {
                scriptPath = Path(PROJECT_CONTENT_PATH) / scriptPath;
            }

            return scriptPath;
        }

        bool ReadTextFile(const Path& path, std::string& outText)
        {
            std::ifstream stream(path, std::ios::in);
            if(!stream.is_open())
            {
                return false;
            }

            outText.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
            return true;
        }

        bool ExtractScriptMetadata(const ScriptReference& scriptReference, Path& outAssemblyPath, WString& outNamespaceName, WString& outClassName)
        {
            const Path scriptPath = ResolveScriptPath(scriptReference);
            if(scriptPath.empty() || !exists(scriptPath))
            {
                return false;
            }

            std::string source;
            if(!ReadTextFile(scriptPath, source))
            {
                return false;
            }

            std::smatch namespaceMatch;
            if(std::regex_search(source, namespaceMatch, std::regex(R"(\bnamespace\s+([A-Za-z_][A-Za-z0-9_\.]*))")))
            {
                outNamespaceName = namespaceMatch[1].str().c_str();
            }
            else
            {
                outNamespaceName = "";
            }

            std::smatch classMatch;
            if(std::regex_search(source, classMatch, std::regex(R"(\bclass\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*[A-Za-z_][A-Za-z0-9_\.<>]*)")))
            {
                outClassName = classMatch[1].str().c_str();
            }
            else
            {
                outClassName = scriptPath.stem().string().c_str();
            }

            outAssemblyPath = ResolveScriptAssemblyPath();
            return !outClassName.IsEmpty();
        }

    }

    void ScriptEngine::Initialize(Mono* runtime)
    {
        Runtime = runtime;
        AssemblyCache.clear();
        EntityInstances.clear();
        CurrentRuntimeAssemblyPath.clear();

        if(Runtime == nullptr)
        {
            WD_CORE_ERROR("ScriptEngine initialization failed: Mono runtime is null");
            return;
        }

        RegisterInternalCalls();
        LoadCoreAssembly();
    }

    void ScriptEngine::Shutdown()
    {
        for(auto& [entityId, instance] : EntityInstances)
        {
            InvokeNoArgs(instance, instance.OnDestroyMethod);
            if(instance.GCHandle != 0)
            {
                mono_gchandle_free(instance.GCHandle);
            }
        }

        EntityInstances.clear();
        AssemblyCache.clear();
        CoreAssembly = nullptr;
        CurrentRuntimeAssemblyPath.clear();
        Runtime = nullptr;
    }

    bool ScriptEngine::RebuildScriptAssembly()
    {
        const Path projectRoot = ResolveProjectRootPath();
        const Path vsWherePath = Path("C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe");
        if(!exists(vsWherePath))
        {
            LastReloadStatus = "vswhere.exe was not found";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        const std::string vsWhereCommand = "\"" + vsWherePath.string() + "\" -latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe";
        const std::string msbuildPathString = ReadCommandOutput(vsWhereCommand);
        if(msbuildPathString.empty())
        {
            LastReloadStatus = "Failed to locate MSBuild";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        const std::wstring configuration = GetCurrentFolder().parent_path().filename().wstring();
        const Path csprojPath = projectRoot / "Source" / "ScriptEngine" / "ScriptEngine.csproj";

        const std::wstring commandLine =
            L"\"" + Path(msbuildPathString).wstring() + L"\" \"" + csprojPath.wstring() +
            L"\" /t:Build /p:Configuration=" + configuration + L" /p:Platform=x64 /p:PostBuildEvent= /nologo";

        if(!RunProcessBlocking(commandLine, projectRoot))
        {
            LastReloadStatus = "ScriptEngine build failed";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        return true;
    }

    bool ScriptEngine::CopyAssemblyToRuntimeFolder()
    {
        const Path configuration = GetCurrentFolder().parent_path().filename();
        const Path sourceAssemblyPath = ResolveProjectRootPath() / "Build" / configuration / "ScriptEngine" / "ScriptEngine.dll";
        const Path sourcePdbPath = sourceAssemblyPath.parent_path() / "ScriptEngine.pdb";
        const Path runtimeFolder = GetCurrentFolder();
        const std::string versionSuffix = ".reload_" + std::to_string(++RuntimeAssemblyVersion);
        const Path targetAssemblyPath = runtimeFolder / ("ScriptEngine" + versionSuffix + ".dll");
        const Path targetPdbPath = runtimeFolder / ("ScriptEngine" + versionSuffix + ".pdb");

        std::error_code errorCode;
        copy_file(sourceAssemblyPath, targetAssemblyPath, std::filesystem::copy_options::overwrite_existing, errorCode);
        if(errorCode)
        {
            LastReloadStatus = "Failed to copy ScriptEngine.dll to runtime folder";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        if(exists(sourcePdbPath))
        {
            errorCode.clear();
            copy_file(sourcePdbPath, targetPdbPath, std::filesystem::copy_options::overwrite_existing, errorCode);
        }

        CurrentRuntimeAssemblyPath = targetAssemblyPath;

        return true;
    }

    bool ScriptEngine::ReloadScripts(bool rebuildAssembly)
    {
        if(Runtime == nullptr)
        {
            LastReloadStatus = "Script runtime is not initialized";
            return false;
        }

        std::vector<ECS::EntityT> scriptedEntities;
        auto query = ECS::World.query_builder<ScriptComponent>().build();
        query.each([&](ECS::Entity entity, ScriptComponent&)
        {
            scriptedEntities.push_back(entity.id());
        });

        if(rebuildAssembly && !RebuildScriptAssembly())
        {
            return false;
        }

        if(!CopyAssemblyToRuntimeFolder())
        {
            return false;
        }

        for(auto& [entityId, instance] : EntityInstances)
        {
            InvokeNoArgs(instance, instance.OnDestroyMethod);
            if(instance.GCHandle != 0)
            {
                mono_gchandle_free(instance.GCHandle);
            }
        }

        EntityInstances.clear();
        AssemblyCache.clear();
        CoreAssembly = nullptr;

        if(!Runtime->ReloadAppDomain())
        {
            LastReloadStatus = "Failed to reload Mono app domain";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        RegisterInternalCalls();
        if(!LoadCoreAssembly())
        {
            LastReloadStatus = "Failed to load rebuilt script assembly";
            WD_CORE_ERROR("{0}", LastReloadStatus);
            return false;
        }

        int recreatedCount = 0;
        for(ECS::EntityT entityId : scriptedEntities)
        {
            ECS::Entity entity = ECS::World.entity(entityId);
            if(!entity.is_alive() || !entity.has<ScriptComponent>())
            {
                continue;
            }

            const ScriptComponent& scriptComponent = entity.get<ScriptComponent>();
            if(CreateEntityInstance(entity, scriptComponent))
            {
                recreatedCount++;
            }
        }

        LastReloadStatus = "Reloaded " + std::to_string(recreatedCount) + " script instance(s)";
        WD_CORE_INFO("{0}", LastReloadStatus);
        return true;
    }

    bool ScriptEngine::GetScriptFieldDescriptors(const ScriptComponent& scriptComponent, std::vector<ScriptFieldDescriptor>& outFields)
    {
        outFields.clear();
        if(Runtime == nullptr || !scriptComponent.Script.IsValid())
        {
            return false;
        }

        MonoAssembly* assembly = ResolveAssembly(scriptComponent);
        MonoClass* klass = ResolveClass(assembly, scriptComponent);
        if(klass == nullptr)
        {
            return false;
        }

        MonoObject* defaultInstance = Runtime->CreateObject(klass);
        if(defaultInstance == nullptr)
        {
            return false;
        }

        return CollectScriptFieldDescriptors(klass, defaultInstance, outFields);
    }

    bool ScriptEngine::GetScriptFieldValue(const ScriptComponent& scriptComponent, const std::string& fieldName, ScriptFieldValue& outValue)
    {
        std::vector<ScriptFieldDescriptor> fields;
        if(!GetScriptFieldDescriptors(scriptComponent, fields))
        {
            return false;
        }

        for(const ScriptFieldDescriptor& field : fields)
        {
            if(field.Name == fieldName)
            {
                outValue = field.DefaultValue;
                break;
            }
        }

        std::unordered_map<std::string, ScriptFieldValue> overrides;
        TryParseFieldOverrides(scriptComponent.FieldOverrides, overrides);
        auto overrideIt = overrides.find(fieldName);
        if(overrideIt != overrides.end())
        {
            outValue = overrideIt->second;
            return true;
        }

        return outValue.Type != ScriptFieldType::None;
    }

    void ScriptEngine::SetScriptFieldValue(ScriptComponent& scriptComponent, const std::string& fieldName, const ScriptFieldValue& value)
    {
        std::unordered_map<std::string, ScriptFieldValue> overrides;
        TryParseFieldOverrides(scriptComponent.FieldOverrides, overrides);
        overrides[fieldName] = value;
        scriptComponent.FieldOverrides = SerializeFieldOverrides(overrides);
    }

    bool ScriptEngine::LoadCoreAssembly()
    {
        if(Runtime == nullptr)
        {
            return false;
        }

        Path assemblyPath = ResolveScriptAssemblyPath();

        CoreAssembly = Runtime->LoadCSharpAssembly(assemblyPath);
        if(CoreAssembly == nullptr)
        {
            WD_CORE_ERROR("Failed to load core script assembly from {0}", assemblyPath.string());
            return false;
        }

        WD_CORE_INFO("Loaded script assembly from {0}", assemblyPath.string());
        AssemblyCache[ToUtf8Path(assemblyPath)] = CoreAssembly;
        return true;
    }

    MonoAssembly* ScriptEngine::ResolveAssembly(const ScriptComponent& scriptComponent)
    {
        if(Runtime == nullptr)
        {
            return nullptr;
        }

        Path assemblyPath;
        WString namespaceName;
        WString className;
        if(!ExtractScriptMetadata(scriptComponent.Script, assemblyPath, namespaceName, className))
        {
            WD_CORE_ERROR("Failed to resolve script metadata from '{0}'", scriptComponent.Script.Reference.string());
            return nullptr;
        }

        if(assemblyPath.is_relative())
        {
            assemblyPath = GetCurrentFolder() / assemblyPath;
        }

        const std::string assemblyKey = ToUtf8Path(assemblyPath);
        auto cached = AssemblyCache.find(assemblyKey);
        if(cached != AssemblyCache.end())
        {
            return cached->second;
        }

        MonoAssembly* assembly = Runtime->LoadCSharpAssembly(assemblyPath);
        if(assembly != nullptr)
        {
            AssemblyCache[assemblyKey] = assembly;
        }

        return assembly;
    }

    MonoClass* ScriptEngine::ResolveClass(MonoAssembly* assembly, const ScriptComponent& scriptComponent)
    {
        if(Runtime == nullptr || assembly == nullptr)
        {
            return nullptr;
        }

        Path assemblyPath;
        WString namespaceName;
        WString className;
        if(!ExtractScriptMetadata(scriptComponent.Script, assemblyPath, namespaceName, className))
        {
            return nullptr;
        }

        return Runtime->GetClass(
            assembly,
            className,
            namespaceName
        );
    }

    ScriptEngine::ScriptInstance* ScriptEngine::GetInstance(ECS::EntityT entityId)
    {
        auto it = EntityInstances.find(entityId);
        if(it == EntityInstances.end())
        {
            return nullptr;
        }

        return &it->second;
    }

    void ScriptEngine::RegisterInternalCalls()
    {
        Bindings::RegisterAnimatorCalls(Runtime);
        Bindings::RegisterDebugCalls(Runtime);
        Bindings::RegisterEntityCalls(Runtime);
        Bindings::RegisterInputCalls(Runtime);
        Bindings::RegisterTimeCalls(Runtime);
        Bindings::RegisterQuaternionCalls(Runtime);
        Bindings::RegisterTransformCalls(Runtime);
        Bindings::RegisterRigidBodyCalls(Runtime);
        Bindings::RegisterCameraCalls(Runtime);
        Bindings::RegisterLightCalls(Runtime);
    }

    MonoObject* ScriptEngine::GetManagedInstance(const ScriptInstance& instance)
    {
        if(instance.GCHandle == 0)
        {
            return nullptr;
        }

        return mono_gchandle_get_target(instance.GCHandle);
    }

    void ScriptEngine::InvokeNoArgs(const ScriptInstance& instance, MonoMethod* method)
    {
        if(method == nullptr)
        {
            return;
        }

        MonoObject* managedInstance = GetManagedInstance(instance);
        if(managedInstance == nullptr)
        {
            return;
        }

        Runtime->InvokeMethod(managedInstance, method, nullptr);
    }

    void ScriptEngine::InvokeSingleFloat(const ScriptInstance& instance, MonoMethod* method, float value)
    {
        if(method == nullptr)
        {
            return;
        }

        MonoObject* managedInstance = GetManagedInstance(instance);
        if(managedInstance == nullptr)
        {
            return;
        }

        void* params[1] = { &value };
        Runtime->InvokeMethod(managedInstance, method, params);
    }

    bool ScriptEngine::CreateEntityInstance(ECS::Entity entity, const ScriptComponent& scriptComponent)
    {
        if(!IsInitialized() || !entity.is_alive() || !scriptComponent.Enabled || !scriptComponent.Script.IsValid())
        {
            return false;
        }

        DestroyEntityInstance(entity.id());

        MonoAssembly* assembly = ResolveAssembly(scriptComponent);
        if(assembly == nullptr)
        {
            WD_CORE_ERROR("Failed to resolve script assembly for entity {0}", (uint64_t)entity.id());
            return false;
        }

        MonoClass* klass = ResolveClass(assembly, scriptComponent);
        if(klass == nullptr)
        {
            Path assemblyPath;
            WString namespaceName;
            WString className;
            ExtractScriptMetadata(scriptComponent.Script, assemblyPath, namespaceName, className);

            WD_CORE_ERROR(
                "Failed to resolve script class {0}.{1}",
                namespaceName.C_Str(),
                className.C_Str()
            );
            return false;
        }

        MonoObject* instanceObject = Runtime->CreateObject(klass);
        if(instanceObject == nullptr)
        {
            return false;
        }

        ApplyFieldOverrides(scriptComponent, klass, instanceObject);

        ScriptInstance instance;
        instance.Class = klass;
        instance.SetEntityIdMethod = Runtime->GetMethod(klass, "__SetEntityId", 1);
        instance.OnCreateMethod = Runtime->GetMethod(klass, "OnCreate", 0);
        instance.OnUpdateMethod = Runtime->GetMethod(klass, "OnUpdate", 1);
        instance.OnFixedUpdateMethod = Runtime->GetMethod(klass, "OnFixedUpdate", 1);
        instance.OnDestroyMethod = Runtime->GetMethod(klass, "OnDestroy", 0);
        instance.OnCollisionEnterMethod = Runtime->GetMethod(klass, "OnCollisionEnterInternal", 1);
        instance.OnCollisionStayMethod = Runtime->GetMethod(klass, "OnCollisionStayInternal", 1);
        instance.OnCollisionExitMethod = Runtime->GetMethod(klass, "OnCollisionExitInternal", 1);
        instance.OnTriggerEnterMethod = Runtime->GetMethod(klass, "OnTriggerEnterInternal", 1);
        instance.OnTriggerStayMethod = Runtime->GetMethod(klass, "OnTriggerStayInternal", 1);
        instance.OnTriggerExitMethod = Runtime->GetMethod(klass, "OnTriggerExitInternal", 1);
        instance.GCHandle = mono_gchandle_new(instanceObject, true);

        EntityInstances[entity.id()] = instance;
        ScriptInstance& storedInstance = EntityInstances[entity.id()];

        if(storedInstance.SetEntityIdMethod != nullptr)
        {
            uint64_t entityId = static_cast<uint64_t>(entity.id());
            void* params[1] = { &entityId };
            Runtime->InvokeMethod(instanceObject, storedInstance.SetEntityIdMethod, params);
        }
        else
        {
            WD_CORE_ERROR("Failed to bind managed entity id for script instance on entity {0}", (uint64_t)entity.id());
        }

        InvokeNoArgs(storedInstance, storedInstance.OnCreateMethod);
        return true;
    }

    void ScriptEngine::ApplyFieldOverrides(const ScriptComponent& scriptComponent, MonoClass* klass, MonoObject* instanceObject)
    {
        if(klass == nullptr || instanceObject == nullptr)
        {
            return;
        }

        std::unordered_map<std::string, ScriptFieldValue> overrides;
        if(!TryParseFieldOverrides(scriptComponent.FieldOverrides, overrides) || overrides.empty())
        {
            return;
        }

        for(const auto& [fieldName, value] : overrides)
        {
            ScriptFieldType actualType = ScriptFieldType::None;
            MonoClassField* field = FindScriptField(klass, fieldName, &actualType);
            if(field == nullptr || actualType != value.Type)
            {
                continue;
            }

            WriteFieldValue(instanceObject, field, value);
        }
    }

    void ScriptEngine::DestroyEntityInstance(ECS::EntityT entityId)
    {
        ScriptInstance* instance = GetInstance(entityId);
        if(instance == nullptr)
        {
            return;
        }

        InvokeNoArgs(*instance, instance->OnDestroyMethod);

        if(instance->GCHandle != 0)
        {
            mono_gchandle_free(instance->GCHandle);
        }

        EntityInstances.erase(entityId);
    }

    void ScriptEngine::OnUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float deltaTime)
    {
        if(!entity.is_alive() || !scriptComponent.Enabled || !scriptComponent.Script.IsValid())
        {
            return;
        }

        ScriptInstance* instance = GetInstance(entity.id());
        if(instance == nullptr)
        {
            if(!CreateEntityInstance(entity, scriptComponent))
            {
                return;
            }

            instance = GetInstance(entity.id());
            if(instance == nullptr)
            {
                return;
            }
        }

        InvokeSingleFloat(*instance, instance->OnUpdateMethod, deltaTime);
    }

    void ScriptEngine::OnFixedUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float fixedDeltaTime)
    {
        if(!entity.is_alive() || !scriptComponent.Enabled || !scriptComponent.Script.IsValid())
        {
            return;
        }

        ScriptInstance* instance = GetInstance(entity.id());
        if(instance == nullptr)
        {
            if(!CreateEntityInstance(entity, scriptComponent))
            {
                return;
            }

            instance = GetInstance(entity.id());
            if(instance == nullptr)
            {
                return;
            }
        }

        InvokeSingleFloat(*instance, instance->OnFixedUpdateMethod, fixedDeltaTime);
    }

    void ScriptEngine::OnCollisionEvent(CollisionEventType eventType, ECS::Entity entity, ECS::Entity other, const ScriptComponent& scriptComponent, const ContactsManifold& contacts)
    {
        ScriptInstance* instance = GetInstance(entity.id());
        if(instance == nullptr)
        {
            if(!CreateEntityInstance(entity, scriptComponent))
            {
                return;
            }

            instance = GetInstance(entity.id());
            if(instance == nullptr)
            {
                return;
            }
        }

        MonoMethod* method = nullptr;

        switch (eventType)
        {
        case CollisionEventType::Enter:
            method = instance->OnCollisionEnterMethod;
            break;
        case CollisionEventType::Stay:
            method = instance->OnCollisionStayMethod;
            break;
        case CollisionEventType::Exit:
            method = instance->OnCollisionExitMethod;
            break;
        default: ;
        }
        
        if(method == nullptr)
        {
            return;
        }

        MonoObject* managedInstance = GetManagedInstance(*instance);
        if(managedInstance == nullptr)
        {
            return;
        }

        if(contacts.Points.Num() <= 0)
        {
            return;
        }

        const ContactPoint& firstContactPoint = contacts.Points.First();
        
        ContactPointSimplified contactPoint;
        contactPoint.Position = firstContactPoint.Position;
        contactPoint.PositionA = firstContactPoint.PositionA;
        contactPoint.PositionB = firstContactPoint.PositionB;
        contactPoint.Normal = firstContactPoint.Normal;
        contactPoint.Penetration = firstContactPoint.Penetration;
        contactPoint.OtherEntityId = other.id();

        void* params[1] = { &contactPoint };
        Runtime->InvokeMethod(managedInstance, method, params);
    }

    void ScriptEngine::OnTriggerEvent(CollisionEventType eventType, ECS::Entity entity, ECS::Entity other, const ScriptComponent& scriptComponent)
    {
        ScriptInstance* instance = GetInstance(entity.id());
        if(instance == nullptr)
        {
            if(!CreateEntityInstance(entity, scriptComponent))
            {
                return;
            }

            instance = GetInstance(entity.id());
            if(instance == nullptr)
            {
                return;
            }
        }

        MonoMethod* method = nullptr;

        switch (eventType)
        {
        case CollisionEventType::Enter:
            method = instance->OnTriggerEnterMethod;
            break;
        case CollisionEventType::Stay:
            method = instance->OnTriggerStayMethod;
            break;
        case CollisionEventType::Exit:
            method = instance->OnTriggerExitMethod;
            break;
        default: ;
        }
        
        if(method == nullptr)
        {
            return;
        }

        MonoObject* managedInstance = GetManagedInstance(*instance);
        if(managedInstance == nullptr)
        {
            return;
        }

        auto id = other.id();
        void* params[1] = { &id };
        Runtime->InvokeMethod(managedInstance, method, params);
    }
}
