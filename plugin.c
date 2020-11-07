/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "FreeImageLib.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <Wincrypt.h>
#include <urlmon.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"

#include "FreeImage/FreeImage.h"



static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 23

#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"EasyAvatar";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "EasyAvatar";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "EasyAvatar";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
	return "1.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "AlEscher";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Easily set your avatar from an URL.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
	ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
	char appPath[PATH_BUFSIZE];
	char resourcesPath[PATH_BUFSIZE];
	char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

	/* Your plugin init code here */

	ts3Functions.logMessage("Plugin Init", LogLevel_DEBUG, EASYAVATAR_LOGCHANNEL, 0);

	if (!EasyAvatar_CreateDirectory())
		return 1;

	FreeImage_Initialise(1);

	ts3Functions.logMessage("Init successfull", LogLevel_DEBUG, EASYAVATAR_LOGCHANNEL, 0);
	/* Example on how to query application, resources and configuration paths from client */
	/* Note: Console client returns empty string for app and resources path */
	ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
	ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
	ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	/*char buffer[4096];
	snprintf(buffer, sizeof(buffer), "PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);
	ts3Functions.logMessage(buffer, LogLevel_DEBUG, EASYAVATAR_LOGCHANNEL, 0);*/

	return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	/* Your plugin cleanup code here */
	
	FreeImage_DeInitialise();

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	/*
	 * Return values:
	 * PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	 * PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	 * PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	 */
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "";
}

static void print_and_free_bookmarks_list(struct PluginBookmarkList* list)
{
	int i;
	for (i = 0; i < list->itemcount; ++i) {
		if (list->items[i].isFolder) {
			printf("Folder: name=%s\n", list->items[i].name);
			print_and_free_bookmarks_list(list->items[i].folder);
			ts3Functions.freeMemory(list->items[i].name);
		} else {
			printf("Bookmark: name=%s uuid=%s\n", list->items[i].name, list->items[i].uuid);
			ts3Functions.freeMemory(list->items[i].name);
			ts3Functions.freeMemory(list->items[i].uuid);
		}
	}
	ts3Functions.freeMemory(list);
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	return 1;
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
//const char* ts3plugin_infoTitle() {
//	return "Test plugin info";
//}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	*data = NULL;
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
 * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
 * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
 * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
 */
enum {
	MENU_ID_CLIENT_1 = 1,
	MENU_ID_CLIENT_2,
	MENU_ID_CHANNEL_1,
	MENU_ID_CHANNEL_2,
	MENU_ID_CHANNEL_3,
	MENU_ID_GLOBAL_1,
	MENU_ID_GLOBAL_2
};

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	BEGIN_CREATE_MENUS(1);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT,  MENU_ID_CLIENT_1,  "Set Image",  "1.png");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	/*
	 * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	 * Test it with plugin command: /test enablemenu <menuID> <0|1>
	 * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	 * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	 * menu is displayed.
	 */
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

/* Helper function to create a hotkey */
static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
	struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
	_strcpy(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
	_strcpy(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
	return hotkey;
}

/* Some makros to make the code to create hotkeys a bit more readable */
#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
#define CREATE_HOTKEY(a, b) (*hotkeys)[n++] = createHotkey(a, b);
#define END_CREATE_HOTKEYS (*hotkeys)[n++] = NULL; assert(n == sz);

/*
 * Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
 * Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
 * This function is automatically called by the client after ts3plugin_init.
 */
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
	/* Register hotkeys giving a keyword and a description.
	 * The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
	 * The description is shown in the clients hotkey dialog. */
	BEGIN_CREATE_HOTKEYS(1);
	CREATE_HOTKEY("ez_set_avatar", "Set Avatar");
	END_CREATE_HOTKEYS;

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}

/************************** TeamSpeak callbacks ***************************/
/*
 * Following functions are optional, feel free to remove unused callbacks.
 * See the clientlib documentation for details on each function.
 */

/* Clientlib */

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
}

void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID) {
}

void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
}

void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility) {
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage) {
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName) {
}

void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier) {
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID) {
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	if(returnCode) {
		/* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
		/* In case of using a a plugin return code, the plugin can return:
		 * 0: Client will continue handling this error (print to chat tab)
		 * 1: Client will ignore this error, the plugin announces it has handled it */
		return 1;
	}
	return 0;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage) {
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {

	/* Friend/Foe manager has ignored the message, so ignore here as well. */
	if(ffIgnored) {
		return 0; /* Client will ignore the message anyways, so return value here doesn't matter */
	}

	return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
}

void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID) {
}

void ts3plugin_onServerConnectionInfoEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelSubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelUnsubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelUnsubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelDescriptionUpdateEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelPasswordChangedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onPlaybackShutdownCompleteEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onSoundDeviceListChangedEvent(const char* modeID, int playOrCap) {
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels) {
}

void ts3plugin_onEditPostProcessVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited) {
}

void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume) {
}

void ts3plugin_onCustom3dRolloffCalculationWaveEvent(uint64 serverConnectionHandlerID, uint64 waveHandle, float distance, float* volume) {
}

void ts3plugin_onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
}

/* Clientlib rare */

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage) {
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {
	return 0;  /* 0 = handle normally, 1 = client will ignore the poke */
}

void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue) {
}

void ts3plugin_onFileListEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path, const char* name, uint64 size, uint64 datetime, int type, uint64 incompletesize, const char* returnCode) {
}

void ts3plugin_onFileListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path) {
}

void ts3plugin_onFileInfoEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* name, uint64 size, uint64 datetime) {
}

void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupByClientIDEvent(uint64 serverConnectionHandlerID, const char* name, uint64 serverGroupList, uint64 clientDatabaseID) {
}

void ts3plugin_onServerGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onServerGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID) {
}

void ts3plugin_onServerGroupClientListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, uint64 clientDatabaseID, const char* clientNameIdentifier, const char* clientUniqueID) {
}

void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID) {
}

void ts3plugin_onChannelPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onClientPermListEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID) {
}

void ts3plugin_onChannelClientPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID) {
}

void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

int ts3plugin_onServerPermissionErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, unsigned int failedPermissionID) {
	return 0;  /* See onServerErrorEvent for return code description */
}

void ts3plugin_onPermissionListGroupEndIDEvent(uint64 serverConnectionHandlerID, unsigned int groupEndID) {
}

void ts3plugin_onPermissionListEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, const char* permissionName, const char* permissionDescription) {
}

void ts3plugin_onPermissionListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onPermissionOverviewEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, uint64 channelID, int overviewType, uint64 overviewID1, uint64 overviewID2, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onPermissionOverviewFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onClientNeededPermissionsEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, int permissionValue) {
}

void ts3plugin_onClientNeededPermissionsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onFileTransferStatusEvent(anyID transferID, unsigned int status, const char* statusMessage, uint64 remotefileSize, uint64 serverConnectionHandlerID) {
}

void ts3plugin_onClientChatClosedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onClientChatComposingEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onServerLogEvent(uint64 serverConnectionHandlerID, const char* logMsg) {
}

void ts3plugin_onServerLogFinishedEvent(uint64 serverConnectionHandlerID, uint64 lastPos, uint64 fileSize) {
}

void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead) {
}

void ts3plugin_onMessageGetEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, const char* message, uint64 timestamp) {
}

void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID) {
}

void ts3plugin_onClientNamefromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onClientNamefromDBIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onComplainListEvent(uint64 serverConnectionHandlerID, uint64 targetClientDatabaseID, const char* targetClientNickName, uint64 fromClientDatabaseID, const char* fromClientNickName, const char* complainReason, uint64 timestamp) {
}

void ts3plugin_onBanListEvent(uint64 serverConnectionHandlerID, uint64 banid, const char* ip, const char* name, const char* uid, uint64 creationTime, uint64 durationTime, const char* invokerName,
							  uint64 invokercldbid, const char* invokeruid, const char* reason, int numberOfEnforcements, const char* lastNickName) {
}

void ts3plugin_onClientServerQueryLoginPasswordEvent(uint64 serverConnectionHandlerID, const char* loginPassword) {
}

void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand) {
}

void ts3plugin_onIncomingClientQueryEvent(uint64 serverConnectionHandlerID, const char* commandText) {
}

void ts3plugin_onServerTemporaryPasswordListEvent(uint64 serverConnectionHandlerID, const char* clientNickname, const char* uniqueClientIdentifier, const char* description, const char* password, uint64 timestampStart, uint64 timestampEnd, uint64 targetChannelID, const char* targetChannelPW) {
}

/* Client UI callbacks */

/*
 * Called from client when an avatar image has been downloaded to or deleted from cache.
 * This callback can be called spontaneously or in response to ts3Functions.getAvatar()
 */
void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath) {
	/* If avatarPath is NULL, the avatar got deleted */
	/* If not NULL, avatarPath contains the path to the avatar file in the TS3Client cache */

	// Delete the avatar file locally. If we didn't set the avatar through our plugin
	// this function will fail but we don't need to worry about that, 
	// although onAvatarUpdated doesn't get called if we update our avatar manually.
	// We delete here because at this point teamspeak won't have any more handles open to our local file
	DeleteFileA(EASYAVATAR_IMAGEPATH);
}

/*
 * Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
 *
 * Parameters:
 * - serverConnectionHandlerID: ID of the current server tab
 * - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
 * - menuItemID: Id used when creating the menu item
 * - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
 */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) 
{
	if (type == PLUGIN_MENU_TYPE_CLIENT && menuItemID == MENU_ID_CLIENT_1)
	{
		// If we fail setting the avatar, your Avatar becomse a 404 red image
		// and subsequent attempts at setting a new image may have no effects, so we reset it
		if (!EasyAvatar_SetAvatar(serverConnectionHandlerID))
		{
			EasyAvatar_DeleteAvatar(serverConnectionHandlerID);
		}
	}
}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword) 
{
	if (strncmp(keyword, "ez_set_avatar", 13) == 0)
	{
		// If we fail setting the avatar, your Avatar becomse a 404 red image
		// and subsequent attempts at setting a new image may have no effects, so we reset it
		if (!EasyAvatar_SetAvatar(ts3Functions.getCurrentServerConnectionHandlerID()))
		{
			EasyAvatar_DeleteAvatar(ts3Functions.getCurrentServerConnectionHandlerID());
		}
	}
}

/* Called when recording a hotkey has finished after calling ts3Functions.requestHotkeyInputDialog */
void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key) {
	char buf[64];
	snprintf(buf, sizeof(buf), "Hotkey for %s registered: %s", keyword, key);
	ts3Functions.logMessage(buf, LogLevel_INFO, EASYAVATAR_LOGCHANNEL, 0);
}

// This function receives your key Identifier you send to notifyKeyEvent and should return
// the friendly device name of the device this hotkey originates from. Used for display in UI.
const char* ts3plugin_keyDeviceName(const char* keyIdentifier) {
	return NULL;
}

// This function translates the given key identifier to a friendly key name for display in the UI
const char* ts3plugin_displayKeyText(const char* keyIdentifier) {
	return NULL;
}

// This is used internally as a prefix for hotkeys so we can store them without collisions.
// Should be unique across plugins.
const char* ts3plugin_keyPrefix() {
	return "ez_avatar";
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier) {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* My functions */

BOOL EasyAvatar_SetAvatar(uint64 serverConnectionHandlerID)
{
	ts3Functions.logMessage("Called EasyAvatar_SetAvatar", LogLevel_DEBUG, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Uniquely identifies our client on the server
	anyID myID;
	if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
	{
		ts3Functions.logMessage("Error querying own client id", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	anyID transferID;
	int channelID = 0;
	// Avatar file must be named avatar_ followed by the base64 hash of "CLIENTID=" (note trailing '=') with CLIENTID being your clientID on the server, no extension
	char* clientIDHash = NULL;
	char* md5Hash = NULL;

	// Client ID as a string to be passed to the base64 encode function
	char clientID[64];
	// Add trailing '=' to clientID
	snprintf(clientID, sizeof(clientID), "%hu=", myID);
	size_t clientIDHashLen = strnlen_s(clientID, sizeof(clientID));

	clientIDHash = EasyAvatar_b64encode(clientID, clientIDHashLen, clientIDHashLen);
	if (!clientIDHash)
	{
		ts3Functions.logMessage("Failed to create base64 hash of clientID", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	char fileName[128];
	snprintf(fileName, sizeof(fileName), "avatar_%s", clientIDHash);

	// Get image URL from Clipboard, if that fails try to see if a file is copied
	char* imageURL = EasyAvatar_GetLinkFromClipboard(serverConnectionHandlerID);
	if (!imageURL)
	{
		// Try to get a file from clipboard
		// EasyAvatar_GetFileFromClipboard(serverConnectionHandlerID);
		ts3Functions.logMessage("Failed to get Image URL from Clipboard", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(clientIDHash);
		return FALSE;
	}
	

	snprintf(EASYAVATAR_IMAGEPATH, sizeof(EASYAVATAR_IMAGEPATH), "%s\\%s", EASYAVATAR_FILEPATH, fileName);

	HRESULT downloadRes = URLDownloadToFileA(NULL, imageURL, EASYAVATAR_IMAGEPATH, 0, NULL);
	if (downloadRes != S_OK)
	{
		ts3Functions.logMessage("Download of image failed", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(imageURL);
		ts3Functions.freeMemory(clientIDHash);
		return FALSE;
	}

	ts3Functions.freeMemory(imageURL);

	// Failure in this function isn't fatal, we still have a valid image
	EasyAvatar_ResizeAvatar();

	md5Hash = EasyAvatar_CreateMD5Hash(EASYAVATAR_IMAGEPATH, serverConnectionHandlerID);
	if (!md5Hash)
	{
		ts3Functions.logMessage("Failed to create MD5 hash of file contents", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(clientIDHash);
		return FALSE;
	}

	// Upload the image to the virtual servers internal file repository (channel with ID 0)
	// Apparently still returns ERROR_ok even if the path to the image is invalid
	if (ts3Functions.sendFile(serverConnectionHandlerID, channelID, "", fileName, 1, 0, EASYAVATAR_FILEPATH, &transferID, NULL) != ERROR_ok)
	{
		ts3Functions.logMessage("Failed to upload file.", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(clientIDHash);
		ts3Functions.freeMemory(md5Hash);
		return FALSE;
	}

	char msg[1024];
	snprintf(msg, sizeof(msg), "Successfully uploaded file. Transfer ID: %hu ; ClientID: %hu; clientIDHash: %s, MD5Hash: %s", transferID, myID, clientIDHash, md5Hash);
	ts3Functions.logMessage(msg, LogLevel_DEBUG, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Set the CLIENT_FLAG_AVATAR attribute of our client to the md5 hash of the image file in order to register it as our Avatar
	if (ts3Functions.setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_FLAG_AVATAR, md5Hash) != ERROR_ok)
	{
		ts3Functions.logMessage("Failed to set CLIENT_FLAG_AVATAR", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(clientIDHash);
		ts3Functions.freeMemory(md5Hash);
		return FALSE;
	}

	// Flush all changes to the server
	if (ts3Functions.flushClientSelfUpdates(serverConnectionHandlerID, NULL) != ERROR_ok)
	{
		ts3Functions.logMessage("Failed to flush changes", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions.freeMemory(clientIDHash);
		ts3Functions.freeMemory(md5Hash);
		return FALSE;
	}

	ts3Functions.freeMemory(clientIDHash);
	ts3Functions.freeMemory(md5Hash);
	ts3Functions.logMessage("Avatar set successfully!", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Return true if everything worked as expected
	return TRUE;
}

BOOL EasyAvatar_DeleteAvatar(uint64 serverConnectionHandlerID)
{
	ts3Functions.logMessage("Something went wrong, deleting avatar", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Set the CLIENT_FLAG_AVATAR to an empty string to signal that we want to delete / reset it
	if (ts3Functions.setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_FLAG_AVATAR, "") != ERROR_ok)
	{
		ts3Functions.logMessage("Failed to set CLIENT_FLAG_AVATAR while deleting Avatar", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	// Flush all changes to the server
	if (ts3Functions.flushClientSelfUpdates(serverConnectionHandlerID, NULL) != ERROR_ok)
	{
		ts3Functions.logMessage("Failed to flush changes while deleting Avatar", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	return TRUE;
}

// If anything in this function fails, the plugin will be unloaded
BOOL EasyAvatar_CreateDirectory()
{
	char currentDirectory[PATH_BUFSIZE];
	char pluginDirectory[PATH_BUFSIZE];

	// One of the few ts3Functions that doesn't return an error code...
	ts3Functions.getPluginPath(currentDirectory, PATH_BUFSIZE, pluginID);
	
	// Construct the path for our plugin's directory and then create the directory
	snprintf(pluginDirectory, sizeof(pluginDirectory), "%s\\%s", currentDirectory, EASYAVATAR_DIR);
	if (!CreateDirectoryA(pluginDirectory, NULL))
	{
		// CreateDirectory can fail if the directory already exists, check last error
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			// CreateDirectory failed for another reason, abort
			ts3Functions.logMessage("Failed to create plugin directory!", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, 0);
			return FALSE;
		}
		else
		{
			ts3Functions.logMessage("Plugin directory already exists", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, 0);
		}
	}
	else
	{
		ts3Functions.logMessage("Successfully created plugin directory", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, 0);
	}

	// Copy the path to the directory we just created into a global buffer
	_strcpy(EASYAVATAR_FILEPATH, PATH_BUFSIZE, pluginDirectory);

	return TRUE;
}

char* EasyAvatar_GetLinkFromClipboard(uint64 serverConnectionHandlerID)
{
	if (!OpenClipboard(NULL))
		return NULL;

	if (!IsClipboardFormatAvailable(CF_TEXT))
	{
		CloseClipboard();
		return NULL;
	}

	// Allocate a global memory object for the text. +1 for null termination
	HGLOBAL hGlobalMem = GetClipboardData(CF_TEXT);
	const char* clipboardStr = (const char*)(GlobalLock(hGlobalMem));
	size_t clipBoardDataLength = strnlen_s(clipboardStr, 2048) + 1;
	// Check that the clipboard wasn't empty
	if (!hGlobalMem || !clipboardStr || !clipBoardDataLength)
	{
		ts3Functions.logMessage("Failed to get Clipboard contents.", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		GlobalUnlock(hGlobalMem);
		CloseClipboard();
		return NULL;
	}

	// Copy content into a heap allocated buffer
	char* clipBoardContent = (char*)malloc(clipBoardDataLength * sizeof(char));
	if (!clipBoardContent)
	{
		GlobalUnlock(hGlobalMem);
		CloseClipboard();
		return NULL;
	}

	errno_t retValue = _strcpy(clipBoardContent, clipBoardDataLength * sizeof(char), clipboardStr);

	// Unlock GlobalMem handle, according to Documentation don't free it
	GlobalUnlock(hGlobalMem);
	CloseClipboard();

	if (retValue != 0)
	{
		free(clipBoardContent);
		return NULL;
	}
	else
	{
		return clipBoardContent;
	}
}

BOOL EasyAvatar_GetFileFromClipboard(uint64 serverConnectionHandlerID)
{
	if (!OpenClipboard(NULL))
		return FALSE;

	// Never works
	if (!IsClipboardFormatAvailable(CF_DIBV5))
	{
		CloseClipboard();
		return FALSE;
	}

	HGLOBAL hGlobalMem = GetClipboardData(CF_DIBV5);
	BITMAPINFO* bitMapInfo = (BITMAPINFO*)GlobalLock(hGlobalMem);

	GlobalUnlock(hGlobalMem);
	CloseClipboard();
	return TRUE;


}

// Taken from https://stackoverflow.com/a/48818578
	
char* EasyAvatar_b64encode(const unsigned char* data, size_t input_length, size_t output_length)
{
	const int mod_table[] = { 0, 2, 1 };

	output_length = 4 * ((input_length + 2) / 3);

	// One extra character for correct null termination
	char* encoded_data = (char*)malloc(output_length + 1);

	if (encoded_data == NULL)
		return NULL;

	for (int i = 0, j = 0; i < input_length;) 
	{
		unsigned int octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		unsigned int octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		unsigned int triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];

	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[output_length - 1 - i] = '=';

	// Null terminate
	encoded_data[output_length] = 0;
	

	return encoded_data;
};

// As specified by https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content

char* EasyAvatar_CreateMD5Hash(const char* filePath, uint64 serverConnectionHandlerID)
{
	DWORD dwStatus = 0;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	HANDLE hFile = NULL;
	BYTE rgbFile[BUFSIZE];
	DWORD cbRead = 0;
	BYTE rgbHash[MD5LEN];
	DWORD cbHash = 0;
	CHAR rgbDigits[] = "0123456789abcdef";

	hFile = CreateFileA(filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		char errorMessage[1024];
		snprintf(errorMessage, sizeof(errorMessage), "Error openen file for hashing: Filepath: %s  Error Code: %d", filePath, GetLastError());
		ts3Functions.logMessage(errorMessage, LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return NULL;
	}

	// Get handle to the crypto provider
	if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		CloseHandle(hFile);
		return NULL;
	}

	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
	{
		CloseHandle(hFile);
		CryptReleaseContext(hProv, 0);
		return NULL;
	}

	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
	{
		if (0 == cbRead)
		{
			break;
		}

		if (!CryptHashData(hHash, rgbFile, cbRead, 0))
		{
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hFile);
			return NULL;
		}
	}

	if (!bResult)
	{
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hFile);
		return NULL;
	}

	cbHash = MD5LEN;
	char* imageMD5Hash = NULL;
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
	{
		// +1 for Null termination
		imageMD5Hash = (char*)malloc((unsigned long long)cbHash * 2 * sizeof(char) + 1);
		if (!imageMD5Hash)
		{
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			CloseHandle(hFile);
			return NULL;
		}

		// Create our hash
		for (DWORD i = 0; i < cbHash; i++)
		{
			imageMD5Hash[i * 2] = rgbDigits[rgbHash[i] >> 4];
			imageMD5Hash[i * 2 + 1] = rgbDigits[rgbHash[i] & 0xf];
		}
		// Null terminate
		imageMD5Hash[cbHash * 2] = 0;
	}
	else
	{
		// Nothing todo, imageMD5Hash will be NULL which will be handled by caller
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);

	return imageMD5Hash;
}

BOOL EasyAvatar_ResizeAvatar()
{
	// Dynamically get the image type (.png, .jpg, etc...)
	FREE_IMAGE_FORMAT imgFormat = FreeImage_GetFileType(EASYAVATAR_IMAGEPATH, 0);
	FIBITMAP* avatarImage = FreeImage_Load(imgFormat, EASYAVATAR_IMAGEPATH, 0);
	if (!avatarImage)
	{
		return FALSE;
	}

	unsigned int originalH = FreeImage_GetHeight(avatarImage);
	unsigned int originalW = FreeImage_GetWidth(avatarImage);
	unsigned int targetH = originalH;
	unsigned int targetW = originalW;
	
	// TODO: Keep aspect ratio
	if (originalH > 300)
		targetH = 300;
	if (originalW > 300)
		targetW = 300;
	
	// Resize our avatar
	FIBITMAP* resizedImage = FreeImage_Rescale(avatarImage, targetW, targetH, FILTER_BOX);
	if (!resizedImage)
	{
		FreeImage_Unload(avatarImage);
		return FALSE;
	}

	// FreeImage_Save overwriting old avatar file
	FreeImage_Save(imgFormat, resizedImage, EASYAVATAR_IMAGEPATH, 0);

	FreeImage_Unload(avatarImage);
	FreeImage_Unload(resizedImage);

	return TRUE;
}
