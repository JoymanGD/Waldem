#include "wdpch.h"
#include "TimeBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/Time.h"
#include "Waldem/ECS/Components/AnimatorComponent.h"
#include <mono/jit/jit.h>

namespace Waldem::Bindings
{
    namespace
    {
        static WString MonoStringToWString(MonoString* monoStr)
        {
            if (!monoStr) return WString("null");
            char* utf8 = mono_string_to_utf8(monoStr);
            WString result(utf8 ? utf8 : "null");
            mono_free(utf8);
            return result;
        }

        void LogInfo(MonoString* message)
        {
            WD_INFO(MonoStringToWString(message));
        }

        void LogWarning(MonoString* message)
        {
            WD_WARN(MonoStringToWString(message));
        }

        void LogError(MonoString* message)
        {
            WD_ERROR(MonoStringToWString(message));
        }
    }

    void RegisterDebugCalls(Mono* runtime)
    {
        BIND(runtime, LogInfo);
        BIND(runtime, LogWarning);
        BIND(runtime, LogError);
    }
}
