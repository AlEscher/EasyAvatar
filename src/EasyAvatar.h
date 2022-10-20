#pragma once
#include <Windows.h>

#include "../TeamSpeakSDK/teamspeak/public_definitions.h"

#define PATH_BUFSIZE 512

struct TS3Functions;
/*
	Main function. Gets the URL from our clipboard and downloads the image to our plugin's directory.
	Performs some necessary hashing and passes all the data including the avatar image to the server.
	Returns true if everything went as expected, false otherwise.
*/
BOOL EasyAvatar_SetAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions);
/*
	Deletes your avatar in case something went wrong while setting it.
*/
BOOL EasyAvatar_DeleteAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions);

/*
	Called once on initialization.
	Creates a directory for our plugin inside of %appdata%/TS3Client/plugins
*/
BOOL EasyAvatar_CreateDirectory(struct TS3Functions* ts3Functions, char* pluginID);

/*
	Returns a heap allocated string of the base64 encoded version of data.
	Returns NULL if anything fails.
*/
char* EasyAvatar_b64encode(const unsigned char* data, size_t input_length, size_t output_length);

/*
	Retrieves the data stored in the user's clipboard, returns a heap allocated string.
	Returns NULL if anything fails.
*/
char* EasyAvatar_GetLinkFromClipboard(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions);
int EasyAvatar_GetFileFromClipboard(uint64 serverConnectionHandlerID);

/*
	Returns a heap allocated MD5 Hash of the given file.
	Returns NULL if anything fails.
*/
char* EasyAvatar_CreateMD5Hash(const char* filePath, uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions);

/*
	Resizes our avatar file on disk before we upload it to the server.
	Will only fail if the file isn't an image
*/
BOOL EasyAvatar_ResizeAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions);


/* Other stuff */

#define EASYAVATAR_NAME "EasyAvatar"
#define EASYAVATAR_LOGCHANNEL "EasyAvatar"
#define EASYAVATAR_DIR "easy_avatar"
#define BUFSIZE 1024
#define MD5LEN  16
#define PLUGIN_VERSION "1.1"
// Path to our plugin's directory
char EASYAVATAR_FILEPATH[PATH_BUFSIZE];
// Absolute file path to our image file
char EASYAVATAR_IMAGEPATH[PATH_BUFSIZE];

static const char encoding_table[] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8', '9', '+', '/' };