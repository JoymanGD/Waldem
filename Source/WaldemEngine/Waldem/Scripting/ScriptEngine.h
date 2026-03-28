#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>

#include "Waldem/ECS/ECS.h"

namespace Waldem
{
    struct ScriptComponent;
    class Mono;

    class WALDEM_API ScriptEngine
    {
    public:
        static void Initialize(Mono* runtime);
        static void Shutdown();
        static bool ReloadScripts(bool rebuildAssembly = true);
        static const char* GetLastReloadStatus() { return LastReloadStatus.c_str(); }

        static bool IsInitialized() { return Runtime != nullptr; }
        static bool CreateEntityInstance(ECS::Entity entity, const ScriptComponent& scriptComponent);
        static void DestroyEntityInstance(ECS::EntityT entityId);
        static void OnUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float deltaTime);
        static void OnFixedUpdate(ECS::Entity entity, const ScriptComponent& scriptComponent, float fixedDeltaTime);

    private:
        struct ScriptInstance
        {
            MonoClass* Class = nullptr;
            MonoMethod* SetEntityIdMethod = nullptr;
            MonoMethod* OnCreateMethod = nullptr;
            MonoMethod* OnUpdateMethod = nullptr;
            MonoMethod* OnFixedUpdateMethod = nullptr;
            MonoMethod* OnDestroyMethod = nullptr;
            uint32_t GCHandle = 0;
        };

        static bool RebuildScriptAssembly();
        static bool CopyAssemblyToRuntimeFolder();
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
