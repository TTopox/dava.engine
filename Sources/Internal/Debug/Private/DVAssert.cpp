#include "Debug/DVAssert.h"
#include "Debug/Backtrace.h"

#include "Concurrency/LockGuard.h"
#include "Concurrency/Mutex.h"

#include <csignal>

namespace DAVA
{
namespace Assert
{
static Vector<Handler> registredHandlers;

static Mutex registredHandlersMutex;

void AddHandler(const Handler handler)
{
    LockGuard<Mutex> lock(registredHandlersMutex);

    const Vector<Handler>::iterator position = std::find(registredHandlers.begin(), registredHandlers.end(), handler);
    if (position != registredHandlers.end())
    {
        return;
    }

    registredHandlers.push_back(handler);
}

void RemoveHandler(const Handler handler)
{
    LockGuard<Mutex> lock(registredHandlersMutex);

    const Vector<Handler>::iterator position = std::find(registredHandlers.begin(), registredHandlers.end(), handler);
    if (position != registredHandlers.end())
    {
        registredHandlers.erase(position);
    }
}
}
}

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_MACOS__) || defined(__DAVAENGINE_ANDROID__)
void RaiseSigTrap()
{
    raise(SIGTRAP);
}
#endif

static int GetFailBehaviourPriority(const DAVA::Assert::FailBehaviour behaviour)
{
    using namespace DAVA::Assert;

    switch (behaviour)
    {
    case FailBehaviour::Default:
        return 0;

    case FailBehaviour::Continue:
        return 1;

    case FailBehaviour::Halt:
        return 2;

    default:
        return -1;
    }
}

DAVA::Assert::FailBehaviour HandleAssert(const char* const expr,
                                         const char* const fileName,
                                         const int lineNumber,
                                         const char* const message)
{
    using namespace DAVA;
    using namespace DAVA::Assert;

    // Copy handlers list to avoid data race in case some handler uses AddHandler or RemoveHandler functions
    Vector<Handler> handlersCopy;
    {
        LockGuard<Mutex> lock(registredHandlersMutex);
        handlersCopy = registredHandlers;
    }

    if (handlersCopy.empty())
    {
        return FailBehaviour::Default;
    }

    // Invoke all the handlers with according assert info and return result behaviour
    // Each behaviour is more prioritized than the previous one, in this order:
    // Default -> Continue -> Halt

    Vector<void*> backtrace = Debug::GetBacktrace();
    const AssertInfo assertInfo(expr, fileName, lineNumber, message, backtrace);

    FailBehaviour resultBehaviour = FailBehaviour::Default;
    for (const Handler& handler : handlersCopy)
    {
        const FailBehaviour requestedBehaviour = handler(assertInfo);

        if (GetFailBehaviourPriority(requestedBehaviour) > GetFailBehaviourPriority(resultBehaviour))
        {
            resultBehaviour = requestedBehaviour;
        }
    }

    return resultBehaviour;
}