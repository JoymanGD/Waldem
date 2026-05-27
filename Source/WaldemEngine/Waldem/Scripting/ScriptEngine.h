#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>

#include "Waldem/ECS/ECS.h"
#include "Waldem/ECS/Components/ColliderComponent.h"

namespace Waldem
{
    struct ScriptComponent;
    class Mono;

    class WALDEM_API ScriptEngine
    {
    public:
        enum class ScriptFieldType
        {
            None = 0,
            Float,
            Int,
            Bool,
            Vector3
        };

        struct ScriptFieldValue
        {
            ScriptFieldType Type = ScriptFieldType::None;
            float FloatValue = 0.0f;
            int32_t IntValue = 0;
            bool BoolValue = false;
            Vector3 Vector3Value = { 0, 0, 0 };
        };

        struct ScriptFieldDescriptor
        {
            std::string Name;
            ScriptFieldValue DefaultValue;
        };

        static void Initialize(Mono* runtime);
        static void Shutdown();
        static bool ReloadScripts(bool rebuildAssembly = true);
        static const char* GetLastReloadStatus() { return LastReloadStatus.c_str(); }
        static bool GetScriptFieldDescriptors(const ScriptComponent& scriptComponent, std::vector<ScriptFieldDescriptor>& outFields);
        static bool GetScriptFieldValue(const ScriptComponent& scriptComponent, const std::string& fieldName, ScriptFieldValue& outValue);
        static void SetScriptFieldValue(ScriptComponent& scriptComponent, const std::string& fieldName, const ScriptFieldValue& value);

        static bool IsInitialized() { return Runtime != nullptr; }
        static bool CreateEntityInstance(ECS::Entity entity, const ScriptComponent& scriptComponent);
        static void DestroyEntityInstance(ECS::EntityT entityId);
        static void OnUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float deltaTime);
        static void OnFixedUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float fixedDeltaTime);
        static void OnCollisionEnter(ECS::Entity entity, ECS::Entity other, const ScriptComponent& scriptComponent, const ContactsManifold& contacts);
        static void OnCollisionStay(ECS::Entity entity, ECS::Entity other, const ScriptComponent& scriptComponent, const ContactsManifold& contacts);
        static void OnCollisionExit(ECS::Entity entity, ECS::Entity other, const ScriptComponent& scriptComponent, const ContactsManifold& contacts);

    private:
        struct ScriptInstance
        {
            MonoClass* Class = nullptr;
            MonoMethod* SetEntityIdMethod = nullptr;
            MonoMethod* OnCreateMethod = nullptr;
            MonoMethod* OnUpdateMethod = nullptr;
            MonoMethod* OnFixedUpdateMethod = nullptr;
            MonoMethod* OnDestroyMethod = nullptr;
            MonoMethod* OnCollisionEnterMethod = nullptr;
            MonoMethod* OnCollisionStayMethod = nullptr;
            MonoMethod* OnCollisionExitMethod = nullptr;
            uint32_t GCHandle = 0;
        };

        static bool RebuildScriptAssembly();
        static bool CopyAssemblyToRuntimeFolder();
        static void ApplyFieldOverrides(const ScriptComponent& scriptComponent, MonoClass* klass, MonoObject* instanceObject);
        static bool LoadCoreAssembly();
        static MonoAssembly* ResolveAssembly(const ScriptComponent& scriptComponent);
        static MonoClass* ResolveClass(MonoAssembly* assembly, const ScriptComponent& scriptComponent);
        static ScriptInstance* GetInstance(ECS::EntityT entityId);
        static void RegisterInternalCalls();
        static MonoObject* GetManagedInstance(const ScriptInstance& instance);
        static void InvokeNoArgs(const ScriptInstance& instance, MonoMethod* method);
        static void InvokeSingleFloat(const ScriptInstance& instance, MonoMethod* method, float value);
    private:
        inline static Mono* Runtime = nullptr;
        inline static MonoAssembly* CoreAssembly = nullptr;
        inline static std::unordered_map<std::string, MonoAssembly*> AssemblyCache;
        inline static std::unordered_map<ECS::EntityT, ScriptInstance> EntityInstances;
        inline static std::string LastReloadStatus;
    };
}
