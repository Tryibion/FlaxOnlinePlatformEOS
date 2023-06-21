#include "OnlinePlatformEOS.h"

#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Utilities/StringConverter.h"
#if USE_EDITOR
#include "Engine/Platform/FileSystem.h"
#include "Engine/Platform/File.h"
#endif
#include <EOSSDK/Include/eos_sdk.h>
#include "Engine/Engine/Time.h"
#include "Engine/Platform/Windows/WindowsWindow.h"
#include "EOSSDK/Include/eos_logging.h"
#include "EOSSDK/Include/eos_types.h"

IMPLEMENT_GAME_SETTINGS_GETTER(EOSSettings, "EOS");

extern "C" void EOS_CALL EOSSDKLogCallback(const EOS_LogMessage* Message)
{
    switch (Message->Level)
    {
    case EOS_ELogLevel::EOS_LOG_Fatal:
        LOG(Fatal, "[EOS] {0}: {1}", String(Message->Category), String(Message->Message));
        break;
    case EOS_ELogLevel::EOS_LOG_Error:
        LOG(Error, "[EOS] {0}: {1}", String(Message->Category), String(Message->Message));
        break;
    case EOS_ELogLevel::EOS_LOG_Warning:
        LOG(Warning, "[EOS] {0}: {1}", String(Message->Category), String(Message->Message));
        break;
    case EOS_ELogLevel::EOS_LOG_Info:
    case EOS_ELogLevel::EOS_LOG_Verbose: 
    case EOS_ELogLevel::EOS_LOG_VeryVerbose:
        LOG(Info, "[EOS] {0}: {1}", String(Message->Category), String(Message->Message));
        break;
    default: break;
    }
}

OnlinePlatformEOS::OnlinePlatformEOS(const SpawnParams& params)
    : ScriptingObject(params)
{
}

bool OnlinePlatformEOS::Initialize()
{
    const auto settings = EOSSettings::Get();
    const auto gameSettings = GameSettings::Get();
    
    // Initialize EOS
    EOS_InitializeOptions initOptions = {};
    initOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
    initOptions.Reserved = nullptr;
    initOptions.ProductName = settings->ProductName.IsEmpty() ? Globals::ProductName.ToStringAnsi().GetText() : settings->ProductName.ToStringAnsi().GetText();
    initOptions.ProductVersion = settings->ProductVersion.ToStringAnsi().GetText();
    initOptions.AllocateMemoryFunction = nullptr;
    initOptions.ReallocateMemoryFunction = nullptr;
    initOptions.ReleaseMemoryFunction = nullptr;
    initOptions.SystemInitializeOptions = nullptr;
    initOptions.OverrideThreadAffinity = nullptr;

    EOS_EResult initResult = EOS_Initialize(&initOptions);
    if (initResult != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS init failed. Init result: {0}", String(EOS_EResult_ToString(initResult)));
        return true;
    }
    
    // Set Logging callback
    EOS_Logging_SetCallback(&EOSSDKLogCallback);
    
    // TODO: put these options in settings in editor
    EOS_Platform_Options platformOptions = {};
    platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    platformOptions.Reserved = nullptr;
    platformOptions.ProductId = "id";
    platformOptions.SandboxId = "id";
    platformOptions.ClientCredentials.ClientId = "id";
    platformOptions.ClientCredentials.ClientSecret = "secret";
    
    if (Engine::IsHeadless())
        platformOptions.bIsServer = EOS_TRUE;
    else
        platformOptions.bIsServer = EOS_FALSE;

    platformOptions.EncryptionKey = nullptr;
    platformOptions.OverrideCountryCode = nullptr;
    platformOptions.OverrideLocaleCode = nullptr;
    platformOptions.DeploymentId = "ID";

    platformOptions.Flags |= EOS_PF_WINDOWS_ENABLE_OVERLAY_OPENGL;
#if PLATFORM_WINDOWS
    platformOptions.Flags |= EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D10 | EOS_PF_WINDOWS_ENABLE_OVERLAY_D3D9 | EOS_PF_WINDOWS_ENABLE_OVERLAY_OPENGL;
#endif

#if USE_EDITOR
    platformOptions.Flags |= EOS_PF_LOADING_IN_EDITOR;
#endif
    
    platformOptions.CacheDirectory = Globals::TemporaryFolder.ToStringAnsi().GetText();
    platformOptions.TickBudgetInMilliseconds = 0;
    platformOptions.RTCOptions = nullptr;
    platformOptions.IntegratedPlatformOptionsContainerHandle = nullptr;

    _hPlatform = EOS_Platform_Create(&platformOptions);

    // Restart with Epic Launcher if not already launched
    auto checkResult = EOS_Platform_CheckForLauncherAndRestart(_hPlatform);
    if (checkResult != EOS_EResult::EOS_NoChange)
    {
        LOG(Info, "Restarting game via Epic Games launcher.");
        Engine::RequestExit(0);
        return true;
    }
    
    //Engine::MainWindow.
    //TODO: hook into changing EOS network status on game network status change
    Engine::LateUpdate.Bind<OnlinePlatformEOS, &OnlinePlatformEOS::OnUpdate>(this);
    return false;
}

void OnlinePlatformEOS::Deinitialize()
{
    EOS_Platform_Release(_hPlatform);
    EOS_Shutdown();
    Engine::LateUpdate.Unbind<OnlinePlatformEOS, &OnlinePlatformEOS::OnUpdate>(this);
}

bool OnlinePlatformEOS::UserLogin(User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::UserLogout(User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetUserLoggedIn(User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetUser(OnlineUser& user, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetFriends(Array<OnlineUser, HeapAllocation>& friends, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetAchievements(Array<OnlineAchievement, HeapAllocation>& achievements, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::UnlockAchievement(const StringView& name, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::UnlockAchievementProgress(const StringView& name, float progress, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::ResetAchievements(User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetStat(const StringView& name, float& value, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::SetStat(const StringView& name, float value, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::GetSaveGame(const StringView& name, Array<byte, HeapAllocation>& data, User* localUser)
{
    return false;
}

bool OnlinePlatformEOS::SetSaveGame(const StringView& name, const Span<byte>& data, User* localUser)
{
    return false;
}

void OnlinePlatformEOS::SetEOSLogLevel(EOSLogCategory logCategory, EOSLogLevel logLevel)
{
    auto category = static_cast<EOS_ELogCategory>(logCategory);
    EOS_ELogLevel level = static_cast<EOS_ELogLevel>(logLevel);
    
    EOS_Logging_SetLogLevel(category, level);
}

void OnlinePlatformEOS::CheckApplicationStatus()
{
    // TODO move this to only trigger on window state change.
#if PLATFORM_WINDOWS
    //auto settings = GameSettings::Get();
    //auto windowsSettings = Content::Load<WindowsPlatformSettings>(settings->WindowsPlatform);
#endif
    
    if (Engine::MainWindow->IsForegroundWindow())
    {
        EOS_Platform_SetApplicationStatus(_hPlatform, EOS_EApplicationStatus::EOS_AS_Foreground);
    }
    else if (Time::GetGamePaused() && !Engine::HasFocus)
    {
        EOS_Platform_SetApplicationStatus(_hPlatform, EOS_EApplicationStatus::EOS_AS_BackgroundSuspended);
    }
    /*
#if PLATFORM_WINDOWS
    else if (windowsSettings->RunInBackground && !Engine::HasFocus)
    {
        EOS_Platform_SetApplicationStatus(_hPlatform, EOS_EApplicationStatus::EOS_AS_BackgroundUnconstrained);
    }
#endif
*/
    else if (!Engine::HasFocus)
    {
        EOS_Platform_SetApplicationStatus(_hPlatform, EOS_EApplicationStatus::EOS_AS_BackgroundConstrained);
    }
}

bool OnlinePlatformEOS::RequestCurrentStats()
{
    return false;
}

void OnlinePlatformEOS::OnUpdate()
{
    //CheckApplicationStatus();
    EOS_Platform_Tick(_hPlatform);
}
