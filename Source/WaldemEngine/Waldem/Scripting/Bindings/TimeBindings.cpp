#include "wdpch.h"
#include "TimeBindings.h"
#include "ScriptBindings.h"
#include "Waldem/Scripting/Mono.h"
#include "Waldem/Time.h"

namespace Waldem::Bindings
{
    namespace
    {
        float Time_GetDeltaTime()      { return Time::DeltaTime; }
        float Time_GetFixedDeltaTime() { return Time::FixedDeltaTime; }
        float Time_GetElapsedTime()    { return Time::ElapsedTime; }
    }

    void RegisterTimeCalls(Mono* runtime)
    {
        BIND(runtime, Time_GetDeltaTime);
        BIND(runtime, Time_GetFixedDeltaTime);
        BIND(runtime, Time_GetElapsedTime);
    }
}
