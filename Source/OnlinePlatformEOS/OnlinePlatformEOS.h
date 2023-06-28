#pragma once

#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Config/Settings.h"
#include "Engine/Online/IOnlinePlatform.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "EOSSDK/Include/eos_achievements_types.h"
#include "EOSSDK/Include/eos_auth_types.h"
#include "EOSSDK/Include/eos_connect_types.h"
#include "EOSSDK/Include/eos_friends_types.h"
#include "EOSSDK/Include/eos_leaderboards_types.h"
#include "EOSSDK/Include/eos_playerdatastorage_types.h"
#include "EOSSDK/Include/eos_stats_types.h"
#include "EOSSDK/Include/eos_types.h"
#include "EOSSDK/Include/eos_userinfo_types.h"

///<summary>
/// Logging Categories
///</summary>
API_ENUM() enum class EOSLogCategory
{
    /** Low level logs unrelated to specific services */
	Core = 0,
	/** Logs related to the Auth service */
	Auth = 1,
	/** Logs related to the Friends service */
	Friends = 2,
	/** Logs related to the Presence service */
	Presence = 3,
	/** Logs related to the UserInfo service */
	UserInfo = 4,
	/** Logs related to HTTP serialization */
	HttpSerialization = 5,
	/** Logs related to the Ecommerce service */
	Ecom = 6,
	/** Logs related to the P2P service */
	P2P = 7,
	/** Logs related to the Sessions service */
	Sessions = 8,
	/** Logs related to rate limiting */
	RateLimiter = 9,
	/** Logs related to the PlayerDataStorage service */
	PlayerDataStorage = 10,
	/** Logs related to sdk analytics */
	Analytics = 11,
	/** Logs related to the messaging service */
	Messaging = 12,
	/** Logs related to the Connect service */
	Connect = 13,
	/** Logs related to the Overlay */
	Overlay = 14,
	/** Logs related to the Achievements service */
	Achievements = 15,
	/** Logs related to the Stats service */
	Stats = 16,
	/** Logs related to the UI service */
	UI = 17,
	/** Logs related to the lobby service */
	Lobby = 18,
	/** Logs related to the Leaderboards service */
	Leaderboards = 19,
	/** Logs related to an internal Keychain feature that the authentication interfaces use */
	Keychain = 20,
	/** Logs related to integrated platforms */
	IntegratedPlatform = 21,
	/** Logs related to Title Storage */
	TitleStorage = 22,
	/** Logs related to the Mods service */
	Mods = 23,
	/** Logs related to the Anti-Cheat service */
	AntiCheat = 24,
	/** Logs related to reports client. */
	Reports = 25,
	/** Logs related to the Sanctions service */
	Sanctions = 26,
	/** Logs related to the Progression Snapshot service */
	ProgressionSnapshots = 27,
	/** Logs related to the Kids Web Services integration */
	KWS = 28,
	/** Logs related to the RTC API */
	RTC = 29,
	/** Logs related to the RTC Admin API */
	RTCAdmin = 30,
	/** Logs related to the Custom Invites API */
	CustomInvites = 31,

	/** Not a real log category. Used by EOS_Logging_SetLogLevel to set the log level for all categories at the same time */
	AllCategories = 0x7fffffff
};

///<summary>
/// The EOS Logging levels.
/// Messages will be sent to the callback function if the message's associated log level is less than or equal to the configured level for a category
///</summary>
API_ENUM() enum class EOSLogLevel
{
	Off = 0,
	Fatal = 100,
	Error = 200,
	Warning = 300,
	Info = 400,
	Verbose = 500,
	VeryVerbose = 600
};

/// <summary>
/// The settings for EOS online platform.
/// </summary>
API_CLASS(Namespace="FlaxEngine.Online.EOS") class ONLINEPLATFORMEOS_API EOSSettings : public SettingsBase
{
	API_AUTO_SERIALIZATION();
	DECLARE_SCRIPTING_TYPE_NO_SPAWN(EOSSettings);
	DECLARE_SETTINGS_GETTER(EOSSettings);
public:
	/* TODO: may need this for steam interaction at some point
	// App ID of the game.
	API_FIELD(Attributes="EditorOrder(0)")
	uint32 AppId = 0;
	*/
	API_FIELD() StringAnsi ProductName;
	API_FIELD() StringAnsi ProductVersion;
	API_FIELD() StringAnsi ProductID;
	API_FIELD() StringAnsi SandboxID;
	API_FIELD() StringAnsi DeploymentID;

	API_FIELD() StringAnsi DefaultClientID;
	API_FIELD() StringAnsi DefaultClientSecret;
	API_FIELD() StringAnsi EncryptionKey;
};

///<summary>
/// The online platform implementation for EOS.
///</summary>
API_CLASS(Sealed, Namespace="FlaxEngine.Online.EOS") class ONLINEPLATFORMEOS_API OnlinePlatformEOS : public ScriptingObject, public IOnlinePlatform
{
    DECLARE_SCRIPTING_TYPE(OnlinePlatformEOS);
private:
	static EOS_HPlatform _platformInterface;
	static EOS_HUserInfo _userInfoInterface;
	static EOS_HAuth _authInterface;
	static EOS_HAchievements _achievementsInterface;
	static EOS_HStats _statsInterface;
	static EOS_HFriends _friendsInterface;
	static EOS_HConnect _connectInterface;
	static EOS_HLeaderboards _leaderboardsInterface;
	static EOS_HPlayerDataStorage _playerDataStorageInterface;
    static Array<EOS_ProductUserId, HeapAllocation> _productUserIDs;
	static EOS_EpicAccountId _accountID;
	
	static bool _friendsQueryComplete;
	static bool _friendsQueryFailed;
	
public:
    // [IOnlinePlatform]
    bool Initialize() override;
    void Deinitialize() override;
    bool UserLogin(User* localUser) override;
    bool UserLogout(User* localUser) override;
    bool GetUserLoggedIn(User* localUser) override;
    bool GetUser(OnlineUser& user, User* localUser) override;
    bool GetFriends(Array<OnlineUser, HeapAllocation>& friends, User* localUser) override;
    bool GetAchievements(Array<OnlineAchievement, HeapAllocation>& achievements, User* localUser) override;
    bool UnlockAchievement(const StringView& name, User* localUser) override;
    bool UnlockAchievementProgress(const StringView& name, float progress, User* localUser) override;
#if !BUILD_RELEASE
    bool ResetAchievements(User* localUser) override;
#endif
    bool GetStat(const StringView& name, float& value, User* localUser) override;
    bool SetStat(const StringView& name, float value, User* localUser) override;
    bool GetSaveGame(const StringView& name, API_PARAM(Out) Array<byte, HeapAllocation>& data, User* localUser) override;
    bool SetSaveGame(const StringView& name, const Span<byte>& data, User* localUser) override;
    API_FUNCTION() void SetEOSLogLevel(EOSLogCategory logCategory, EOSLogLevel logLevel);
	void CheckApplicationStatus();

private:
    bool RequestCurrentStats();
    void OnUpdate();
	static void EOS_CALL OnLoginComplete(const EOS_Connect_LoginCallbackInfo* data);
	static void EOS_CALL OnCreateUserComplete(const EOS_Connect_CreateUserCallbackInfo* data);
	static void EOS_CALL OnCreateDeviceIDComplete(const EOS_Connect_CreateDeviceIdCallbackInfo* data);
	static void EOS_CALL OnAuthLoginComplete(const EOS_Auth_LoginCallbackInfo* data);
	static void EOS_CALL OnQueryFriendsComplete( const EOS_Friends_QueryFriendsCallbackInfo* data);
};
