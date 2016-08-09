#if defined(__DAVAENGINE_COREV2__)

#include "Base/BaseTypes.h"
#include "Engine/Private/CommandArgs.h"
#include "Engine/Private/EngineStartup.h"

// clang-format off

#if defined(__DAVAENGINE_MACOS__) || \
    defined(__DAVAENGINE_IPHONE__) || \
    (defined(__DAVAENGINE_WIN32__) && defined(CONSOLE))

int main(int argc, char* argv[])
{
    using namespace DAVA;
    Vector<String> cmdargs = Private::GetCommandArgs(argc, argv);
    return Private::EngineStart(cmdargs);
}

#elif defined(__DAVAENGINE_WIN32__) || defined(__DAVAENGINE_WIN_UAP__)

#include <windows.h>

// Win32
// To use WinMain in static lib with unicode support set entry point to wWinMainCRTStartup:
//  1. through linker commandline option /ENTRY:wWinMainCRTStartup
//  2. property panel Linker -> Advanced -> Entry Point
//  3. cmake script - set_target_properties(target PROPERTIES LINK_FLAGS "/ENTRY:wWinMainCRTStartup")
// https://msdn.microsoft.com/en-us/library/dybsewaf.aspx
// https://support.microsoft.com/en-us/kb/125750

#if defined(__DAVAENGINE_WIN_UAP__)
// Windows Universal Application
// WinMain should have attribute which specifies threading model
[Platform::MTAThread]
#endif
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    using namespace DAVA;
    Vector<String> cmdargs = Private::GetCommandArgs();
    return Private::EngineStart(cmdargs);
}

#elif defined(__DAVAENGINE_ANDROID__)

#include <jni.h>
#include <android/log.h>

#include "Engine/Private/Android/AndroidBridge.h"

DAVA::Private::AndroidBridge* androidBridge = nullptr;

jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        __android_log_print(ANDROID_LOG_ERROR, "DAVA", "JNI_OnLoad: failed to get environment");
        return -1;
    }

    __android_log_print(ANDROID_LOG_INFO, "DAVA", "JNI_OnLoad: androidBridge=%p", androidBridge);
    androidBridge = new DAVA::Private::AndroidBridge(vm);
    return JNI_VERSION_1_6;
}

#endif

// clang-format on

#endif // __DAVAENGINE_COREV2__
