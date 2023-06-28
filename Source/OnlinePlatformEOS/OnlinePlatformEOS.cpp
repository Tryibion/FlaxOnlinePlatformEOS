#include "OnlinePlatformEOS.h"

#include "Engine/Content/Content.h"
#include "Engine/Content/JsonAsset.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Config/GameSettings.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Engine.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Utilities/StringConverter.h"
#include "Engine/Scripting/Enums.h"
#include "Engine/Platform/FileSystem.h"
#if USE_EDITOR
#include "Engine/Platform/File.h"
#endif
#include <EOSSDK/Include/eos_sdk.h>
#include "Engine/Engine/Time.h"
#include "Engine/Platform/Base/UserBase.h"
#include "Engine/Platform/Windows/WindowsWindow.h"
#include "Engine/Scripting/ManagedCLR/MUtils.h"
#include "EOSSDK/Include/eos_auth.h"
#include "EOSSDK/Include/eos_friends.h"
#include "EOSSDK/Include/eos_logging.h"
#include "EOSSDK/Include/eos_types.h"
#include "EOSSDK/Include/eos_ui.h"

IMPLEMENT_GAME_SETTINGS_GETTER(EOSSettings, "EOS");
EOS_HPlatform OnlinePlatformEOS::_platformInterface = nullptr;
EOS_HUserInfo OnlinePlatformEOS::_userInfoInterface = nullptr;
EOS_HAuth OnlinePlatformEOS::_authInterface = nullptr;
EOS_HAchievements OnlinePlatformEOS::_achievementsInterface = nullptr;
EOS_HStats OnlinePlatformEOS::_statsInterface = nullptr;
EOS_HFriends OnlinePlatformEOS::_friendsInterface = nullptr;
EOS_HConnect OnlinePlatformEOS::_connectInterface = nullptr;
EOS_HLeaderboards OnlinePlatformEOS::_leaderboardsInterface = nullptr;
EOS_HPlayerDataStorage OnlinePlatformEOS::_playerDataStorageInterface = nullptr;
EOS_EpicAccountId OnlinePlatformEOS::_accountID = nullptr;
Array<EOS_ProductUserId, HeapAllocation> OnlinePlatformEOS::_productUserIDs;

bool OnlinePlatformEOS::_friendsQueryComplete = false;
bool OnlinePlatformEOS::_friendsQueryFailed = false;

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

extern "C" void* EOS_MEMORY_CALL EOSAllocateMemory(size_t sizeInBytes, size_t alignment)
{
    return Allocator::Allocate(sizeInBytes, alignment);
}

void* Realloc(void* ptr, uint64 newSize, uint64 alignment)
{
    if (newSize == 0)
    {
        Allocator::Free(ptr);
        return nullptr;
    }
    if (!ptr)
        return Allocator::Allocate(newSize, alignment);
    void* result = Allocator::Allocate(newSize, alignment);
    if (result)
    {
        Platform::MemoryCopy(result, ptr, newSize);
        Allocator::Free(ptr);
    }
    return result;
}


extern "C" void* EOS_MEMORY_CALL EOSReallocateMemory(void* pointer, size_t sizeInBytes, size_t alignment)
{
    return Realloc(pointer, sizeInBytes, alignment); // TODO: if this ever gets put in engine, use that function
}

extern "C" void EOS_MEMORY_CALL EOSReleaseMemory(void* pointer)
{
    Allocator::Free(pointer);
}

void OnlinePlatformEOS::OnLoginComplete(const EOS_Connect_LoginCallbackInfo* data)
{
    if (data->ResultCode == EOS_EResult::EOS_InvalidUser)
    {
        EOS_Connect_CreateUserOptions options = {};
        options.ApiVersion = EOS_CONNECT_CREATEUSER_API_LATEST;
        options.ContinuanceToken = data->ContinuanceToken;
        EOS_Connect_CreateUser(_connectInterface, &options, nullptr, &OnlinePlatformEOS::OnCreateUserComplete);
    }
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
        return;
    }
    
    _productUserIDs.AddUnique(data->LocalUserId);
}

void OnlinePlatformEOS::OnCreateUserComplete(const EOS_Connect_CreateUserCallbackInfo* data)
{
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
        return;
    }
    _productUserIDs.AddUnique(data->LocalUserId);
}

void OnlinePlatformEOS::OnCreateDeviceIDComplete(const EOS_Connect_CreateDeviceIdCallbackInfo* data)
{
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS failed to create device ID: {0}", String(EOS_EResult_ToString(data->ResultCode)));
        return;
    }
}

void OnlinePlatformEOS::OnAuthLoginComplete(const EOS_Auth_LoginCallbackInfo* data)
{
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS failed to auth login: {0}", String(EOS_EResult_ToString(data->ResultCode)));
        return;
    }
    
    EOS_Connect_LoginOptions connectLoginOptions = {};
    connectLoginOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    EOS_Connect_Credentials connectCreds = {};
    connectCreds.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    connectCreds.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC_ID_TOKEN;
    EOS_Auth_CopyIdTokenOptions idCopyOptions = {};
    idCopyOptions.ApiVersion = EOS_AUTH_COPYIDTOKEN_API_LATEST;
    idCopyOptions.AccountId = data->LocalUserId;
    EOS_Auth_IdToken* idToken;
    auto result = EOS_Auth_CopyIdToken(_authInterface, &idCopyOptions, &idToken);
    if (result != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS failed connect via auth login: {0}", String(EOS_EResult_ToString(data->ResultCode)));
        return;
    }
    connectCreds.Token = idToken->JsonWebToken;
    connectLoginOptions.Credentials = &connectCreds;
    EOS_Connect_Login(_connectInterface, &connectLoginOptions, nullptr, &OnlinePlatformEOS::OnLoginComplete);
    EOS_Auth_IdToken_Release(idToken);
    _accountID = data->LocalUserId;
}

void OnlinePlatformEOS::OnQueryFriendsComplete(const EOS_Friends_QueryFriendsCallbackInfo* data)
{
    if (data->ResultCode != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS failed to query friends: {0}", String(EOS_EResult_ToString(data->ResultCode)));
        _friendsQueryFailed = true;
        return;
    }
    
    _friendsQueryComplete = true;
}

OnlinePlatformEOS::OnlinePlatformEOS(const SpawnParams& params)
    : ScriptingObject(params)
{
}

bool OnlinePlatformEOS::Initialize()
{
    const auto settings = EOSSettings::Get();
    if (!settings)
    {
        LOG(Error, "EOS Settings failed to load.");
        return true;
    }
    
    // Initialize EOS
    EOS_InitializeOptions initOptions = {};
    initOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
    initOptions.Reserved = nullptr;
    initOptions.ProductName = settings->ProductName.Get();
    initOptions.ProductVersion = settings->ProductVersion.Get();
    initOptions.AllocateMemoryFunction = &EOSAllocateMemory;
    initOptions.ReallocateMemoryFunction = &EOSReallocateMemory;
    initOptions.ReleaseMemoryFunction = &EOSReleaseMemory;
    initOptions.SystemInitializeOptions = nullptr;
    initOptions.OverrideThreadAffinity = nullptr;

    EOS_EResult initResult = EOS_Initialize(&initOptions);
    if (initResult != EOS_EResult::EOS_Success)
    {
        LOG(Error, "EOS init failed. Init result: {0}", String(EOS_EResult_ToString(initResult)));
        return true;
    }
    LOG(Error, "Command line args: {0}", Engine::GetCommandLine());
    // Set Logging callback
    EOS_Logging_SetCallback(&EOSSDKLogCallback);
    SetEOSLogLevel(EOSLogCategory::AllCategories, EOSLogLevel::VeryVerbose); // TODO: disable once released
    
    // TODO: put these options in settings in editor
    EOS_Platform_Options platformOptions = {};
    platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;
    platformOptions.Reserved = nullptr;
    platformOptions.ProductId = settings->ProductID.Get();
    platformOptions.SandboxId = settings->SandboxID.Get();
    platformOptions.DeploymentId = settings->DeploymentID.Get();
    platformOptions.ClientCredentials.ClientId = settings->DefaultClientID.Get();
    platformOptions.ClientCredentials.ClientSecret = settings->DefaultClientSecret.Get();
    
    if (Engine::IsHeadless())
        platformOptions.bIsServer = EOS_TRUE;
    else
        platformOptions.bIsServer = EOS_FALSE;

    platformOptions.EncryptionKey = "0"; //"1111111111111111111111111111111111111111111111111111111111111111";
    platformOptions.OverrideCountryCode = nullptr;
    platformOptions.OverrideLocaleCode = nullptr;
    platformOptions.Flags = 0;
    platformOptions.Flags |= EOS_PF_DISABLE_OVERLAY;

#if USE_EDITOR
    platformOptions.Flags |= EOS_PF_LOADING_IN_EDITOR | EOS_PF_DISABLE_OVERLAY;
#endif

    const StringAsANSI<> cacheDirectory(Globals::TemporaryFolder.Get(), Globals::TemporaryFolder.Length());
    platformOptions.CacheDirectory = cacheDirectory.Get();
    platformOptions.TickBudgetInMilliseconds = 0;
    platformOptions.RTCOptions = nullptr;
    platformOptions.IntegratedPlatformOptionsContainerHandle = nullptr;

    _platformInterface = EOS_Platform_Create(&platformOptions);
    EOS_Platform_SetApplicationStatus(_platformInterface, EOS_EApplicationStatus::EOS_AS_Foreground);
    
/*
    // Restart with Epic Launcher if not already launched
    auto checkResult = EOS_Platform_CheckForLauncherAndRestart(_hPlatform);
    if (checkResult != EOS_EResult::EOS_NoChange)
    {
        LOG(Info, "Restarting game via Epic Games launcher.");
        Engine::RequestExit(0);
        return true;
    }
*/
    _connectInterface = EOS_Platform_GetConnectInterface(_platformInterface);
    _authInterface = EOS_Platform_GetAuthInterface(_platformInterface);
    _userInfoInterface = EOS_Platform_GetUserInfoInterface(_platformInterface);
    _achievementsInterface = EOS_Platform_GetAchievementsInterface(_platformInterface);
    _statsInterface = EOS_Platform_GetStatsInterface(_platformInterface);
    _friendsInterface = EOS_Platform_GetFriendsInterface(_platformInterface);
    _leaderboardsInterface = EOS_Platform_GetLeaderboardsInterface(_platformInterface);
    _playerDataStorageInterface = EOS_Platform_GetPlayerDataStorageInterface(_platformInterface);
    
    _productUserIDs.Clear();

    // Let Epic Launcher pass auth
    if (!Engine::GetCommandLine().IsEmpty())
    {
        Array<String> splitArgs;
        Engine::GetCommandLine().Split('-', splitArgs);
        StringAnsi token;
        EOS_ELoginCredentialType loginType;
        for (auto arg : splitArgs)
        {
            auto trimmedArg = arg.TrimTrailing();
            if (trimmedArg.Contains(TEXT("AUTH_PASSWORD")))
            {
                auto replacedArg = trimmedArg;
                replacedArg.Replace(TEXT("AUTH_PASSWORD="), TEXT(""));
                const StringAsANSI<> password(replacedArg.Get(), replacedArg.Length());
                token = password.Get();
            }
            else if (trimmedArg.Contains(TEXT("AUTH_TYPE")))
            {
                if (trimmedArg.Contains(TEXT("exchangecode"), StringSearchCase::IgnoreCase))
                {
                    loginType = EOS_ELoginCredentialType::EOS_LCT_ExchangeCode;
                }
            }
        }
        
        if (!token.IsEmpty())
        {
            EOS_Auth_Credentials Credentials = {};
            Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
            Credentials.Type = loginType;
            Credentials.Token = token.Get();

            EOS_Auth_LoginOptions LoginOptions = {};
            LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
            LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;
            LoginOptions.Credentials = &Credentials;

            EOS_Auth_Login(_authInterface, &LoginOptions, nullptr, &OnlinePlatformEOS::OnAuthLoginComplete);
        }
    }
    /*
    // Account portal auth
    EOS_Auth_Credentials Credentials = {};
    Credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
    Credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;

    EOS_Auth_LoginOptions LoginOptions = {};
    LoginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
    LoginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList | EOS_EAuthScopeFlags::EOS_AS_Presence;
    LoginOptions.Credentials = &Credentials;

    EOS_Auth_Login(_authInterface, &LoginOptions, nullptr, &OnlinePlatformEOS::OnAuthLoginComplete);
    */
    /*
    // Create Device ID
    EOS_Connect_CreateDeviceIdOptions deviceIDOptions = {};
    deviceIDOptions.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
    auto deviceIdentifier = String::Format(TEXT("{0} {1} {2}"), ScriptingEnum::ToString<PlatformType>(Platform::GetPlatformType()), Platform::GetComputerName(), Platform::GetUniqueDeviceId().ToString());
    const StringAsANSI<> deviceID(deviceIdentifier.Get(), deviceIdentifier.Length());
    deviceIDOptions.DeviceModel =  deviceID.Get();
    EOS_Connect_CreateDeviceId(_connectInterface, &deviceIDOptions, nullptr, &OnlinePlatformEOS::OnCreateDeviceIDComplete);
    */
    /*
    // Initial login with device
    EOS_Connect_LoginOptions connectLoginOptions = {};
    connectLoginOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    EOS_Connect_Credentials connectCreds = {};
    connectCreds.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    connectCreds.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
    //connectCreds.Token = "jwt token"; // TODO: figure out how to get this...
    connectCreds.Token = nullptr;
    connectLoginOptions.Credentials = &connectCreds;
    EOS_Connect_UserLoginInfo userLoginInfo = {};
    userLoginInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
    const StringAsANSI<> displayName(Platform::GetComputerName().Get(), Platform::GetComputerName().Length());
    userLoginInfo.DisplayName = "Tom";//displayName.Get();
    connectLoginOptions.UserLoginInfo = &userLoginInfo;
    EOS_Connect_Login(_connectInterface, &connectLoginOptions, nullptr, &OnlinePlatformEOS::OnLoginComplete);
    */
    
    //TODO: hook into changing EOS network status on game network status change
    Engine::LateUpdate.Bind<OnlinePlatformEOS, &OnlinePlatformEOS::OnUpdate>(this);
    return false;
}

void OnlinePlatformEOS::Deinitialize()
{
    Engine::LateUpdate.Unbind<OnlinePlatformEOS, &OnlinePlatformEOS::OnUpdate>(this);
    _productUserIDs.Clear();
    _platformInterface = nullptr;
    _userInfoInterface = nullptr;
    _authInterface = nullptr;
    _achievementsInterface = nullptr;
    _statsInterface = nullptr;
    _friendsInterface = nullptr;
    _connectInterface = nullptr;
    _leaderboardsInterface = nullptr;
    _playerDataStorageInterface = nullptr;
    EOS_Platform_Release(_platformInterface);
    EOS_Shutdown();
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
    if (!_platformInterface || !_accountID)
    {
        return false;
    }
    EOS_Friends_QueryFriendsOptions queryOptions = {};
    queryOptions.ApiVersion = EOS_FRIENDS_QUERYFRIENDS_API_LATEST;
    queryOptions.LocalUserId = _accountID;

    EOS_Friends_QueryFriends(_friendsInterface, &queryOptions, nullptr, &OnlinePlatformEOS::OnQueryFriendsComplete);

    while (!_friendsQueryComplete)
    {
        if (_friendsQueryFailed)
        {
            _friendsQueryFailed = false;
            return false;
        }
    }
    _friendsQueryComplete = false;
    EOS_Friends_GetFriendsCountOptions countOptions = {};
    countOptions.ApiVersion = EOS_FRIENDS_GETFRIENDSCOUNT_API_LATEST;
    countOptions.LocalUserId = _accountID;
    auto friendsCount = EOS_Friends_GetFriendsCount(_friendsInterface, &countOptions);
    for (int i = 0; i < friendsCount; i++)
    {
        EOS_Friends_GetFriendAtIndexOptions indexOptions = {};
        indexOptions.ApiVersion = EOS_FRIENDS_GETFRIENDATINDEX_API_LATEST;
        indexOptions.Index = i;
        indexOptions.LocalUserId = _accountID;
        auto friendsAccount = EOS_Friends_GetFriendAtIndex(_friendsInterface, &indexOptions);
        // query friends info
    }
    
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
    EOS_PlayerDataStorage_QueryFileOptions fileQueryOptions = {};
    fileQueryOptions.ApiVersion = EOS_PLAYERDATASTORAGE_QUERYFILEOPTIONS_API_LATEST;
    //fileQueryOptions.LocalUserId = 
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
    
    EOS_EResult result = EOS_Logging_SetLogLevel(category, level);
    if (result != EOS_EResult::EOS_Success)
    {
        LOG(Warning, "EOS failed to set logging level. Error: {1}", String(EOS_EResult_ToString(result)));
    }
}

void OnlinePlatformEOS::CheckApplicationStatus()
{
    if (!_platformInterface || Engine::ShouldExit())
        return;

    auto status = EOS_Platform_GetApplicationStatus(_platformInterface);
    if (Engine::MainWindow->IsForegroundWindow() && status != EOS_EApplicationStatus::EOS_AS_Foreground)
    {
        EOS_Platform_SetApplicationStatus(_platformInterface, EOS_EApplicationStatus::EOS_AS_Foreground);
    }
    else if (Time::GetGamePaused() && !Engine::HasFocus && status != EOS_EApplicationStatus::EOS_AS_BackgroundSuspended)
    {
        EOS_Platform_SetApplicationStatus(_platformInterface, EOS_EApplicationStatus::EOS_AS_BackgroundSuspended);
    }
    /*
    else if (!Engine::HasFocus && status != EOS_EApplicationStatus::EOS_AS_BackgroundConstrained)
    {
        EOS_Platform_SetApplicationStatus(_platformInterface, EOS_EApplicationStatus::EOS_AS_BackgroundConstrained);
    }
    */
}

bool OnlinePlatformEOS::RequestCurrentStats()
{
    return false;
}

void OnlinePlatformEOS::OnUpdate()
{
    EOS_Platform_Tick(_platformInterface);
    CheckApplicationStatus();
}
