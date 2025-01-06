#include "EasyAvatar.h"

#include <stdio.h>

#include "FreeImage.h"
#include "../TeamSpeakSDK/teamspeak/public_errors.h"
#include "../TeamSpeakSDK/teamspeak/public_rare_definitions.h"
#include "../TeamSpeakSDK/ts3_functions.h"

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

char lastAvatarHash[MD5LEN * 2 + 1];

BOOL EasyAvatar_SetAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
{
	// Uniquely identifies our client on the server
	anyID myID;
	if (ts3Functions->getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
	{
		ts3Functions->logMessage("Error querying own client id", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
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

	clientIDHash = EasyAvatar_b64encode(clientID, clientIDHashLen);
	if (!clientIDHash)
	{
		ts3Functions->logMessage("Failed to create base64 hash of clientID", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	char fileName[128];
	snprintf(fileName, sizeof(fileName), "avatar_%s", clientIDHash);
	ts3Functions->freeMemory(clientIDHash);

	// Get image URL from Clipboard, if that fails try to see if a file is copied
	char* clipboardData = EasyAvatar_GetStringFromClipboard(serverConnectionHandlerID, ts3Functions);
	if (!clipboardData)
	{
		// Try to get a file from clipboard
		// EasyAvatar_GetFileFromClipboard(serverConnectionHandlerID);
		ts3Functions->logMessage("Failed to get Image URL from Clipboard", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	snprintf(EASYAVATAR_IMAGEPATH, sizeof(EASYAVATAR_IMAGEPATH), "%s\\%s", EASYAVATAR_FILEPATH, fileName);

	if (!EasyAvatar_HandleClipboardContent(clipboardData, serverConnectionHandlerID, ts3Functions))
	{
		ts3Functions->freeMemory(clipboardData);
		return FALSE;
	}
	
	ts3Functions->freeMemory(clipboardData);

	// Failure in this function means the file isn't an image
	// If this function returns true it doesn't indicate that we successfully resized
	if (!EasyAvatar_ResizeAvatar(serverConnectionHandlerID, ts3Functions))
	{
		return FALSE;
	}
	// Check file size after resizing
	if (!EasyAvatar_CheckFileSize(serverConnectionHandlerID, ts3Functions))
	{
		return FALSE;
	}

	md5Hash = EasyAvatar_CreateMD5Hash(EASYAVATAR_IMAGEPATH, serverConnectionHandlerID, ts3Functions);
	if (!md5Hash)
	{
		ts3Functions->logMessage("Failed to create MD5 hash of file contents", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	const int hashLength = MD5LEN * 2 + 1;
	// For some reason, when using a hotkey to set the avatar the callback gets called twice, so that the second execution fails as the file is already uploaded to the TS server
	// Check that we are not trying to upload the same file as last time
	if (strncmp(md5Hash, lastAvatarHash, hashLength) == 0)
	{
		ts3Functions->logMessage("Skipping duplicate avatar", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions->freeMemory(md5Hash);
		return TRUE;
	}
	strncpy_s(lastAvatarHash, hashLength, md5Hash, hashLength-1);

	// Upload the image to the virtual servers internal file repository (channel with ID 0)
	// Apparently still returns ERROR_ok even if the path to the image is invalid
	if (ts3Functions->sendFile(serverConnectionHandlerID, channelID, "", fileName, 1, 0, EASYAVATAR_FILEPATH, &transferID, NULL) != ERROR_ok)
	{
		ts3Functions->logMessage("Failed to upload file.", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions->freeMemory(md5Hash);
		return FALSE;
	}


	// Set the CLIENT_FLAG_AVATAR attribute of our client to the md5 hash of the image file in order to register it as our Avatar
	if (ts3Functions->setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_FLAG_AVATAR, md5Hash) != ERROR_ok)
	{
		ts3Functions->logMessage("Failed to set CLIENT_FLAG_AVATAR", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions->freeMemory(md5Hash);
		return FALSE;
	}

	// Flush all changes to the server
	if (ts3Functions->flushClientSelfUpdates(serverConnectionHandlerID, NULL) != ERROR_ok)
	{
		ts3Functions->logMessage("Failed to flush changes", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		ts3Functions->freeMemory(md5Hash);
		return FALSE;
	}

	ts3Functions->freeMemory(md5Hash);
	ts3Functions->logMessage("Avatar set successfully!", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Return true if everything worked as expected
	return TRUE;
}

BOOL EasyAvatar_DeleteAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
{
	ts3Functions->logMessage("Something went wrong, deleting avatar", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);

	// Set the CLIENT_FLAG_AVATAR to an empty string to signal that we want to delete / reset it
	if (ts3Functions->setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_FLAG_AVATAR, "") != ERROR_ok)
	{
		ts3Functions->logMessage("Failed to set CLIENT_FLAG_AVATAR while deleting Avatar", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	// Flush all changes to the server
	if (ts3Functions->flushClientSelfUpdates(serverConnectionHandlerID, NULL) != ERROR_ok)
	{
		ts3Functions->logMessage("Failed to flush changes while deleting Avatar", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	return TRUE;
}

// If anything in this function fails, the plugin will be unloaded
BOOL EasyAvatar_CreateDirectory(struct TS3Functions* ts3Functions, char* pluginID)
{
	char currentDirectory[PATH_BUFSIZE];
	char pluginDirectory[PATH_BUFSIZE];

	// One of the few ts3Functions that doesn't return an error code...
	ts3Functions->getPluginPath(currentDirectory, PATH_BUFSIZE, pluginID);

	// Construct the path for our plugin's directory and then create the directory
	snprintf(pluginDirectory, sizeof(pluginDirectory), "%s\\%s", currentDirectory, EASYAVATAR_DIR);
	if (!CreateDirectoryA(pluginDirectory, NULL))
	{
		// CreateDirectory can fail if the directory already exists, check last error
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			// CreateDirectory failed for another reason, abort
			ts3Functions->logMessage("Failed to create plugin directory!", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, 0);
			return FALSE;
		}
		else
		{
			ts3Functions->logMessage("Plugin directory already exists", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, 0);
		}
	}
	else
	{
		ts3Functions->logMessage("Successfully created plugin directory", LogLevel_INFO, EASYAVATAR_LOGCHANNEL, 0);
	}

	// Copy the path to the directory we just created into a global buffer
	_strcpy(EASYAVATAR_FILEPATH, PATH_BUFSIZE, pluginDirectory);

	return TRUE;
}

char* EasyAvatar_GetStringFromClipboard(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
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
	// This can be an URL to an image or the whole image encoded as base64
	const char* clipboardStr = (const char*)(GlobalLock(hGlobalMem));
	size_t clipBoardDataLength = strnlen_s(clipboardStr, INT_MAX) + 1;
	// Check that the clipboard wasn't empty
	if (!hGlobalMem || !clipboardStr || !clipBoardDataLength)
	{
		ts3Functions->logMessage("Failed to get Clipboard contents.", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
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

BOOL EasyAvatar_HandleClipboardContent(char* clipboardData, uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
{
	if (strncmp(clipboardData, "data:image/", 11U) == 0 && strstr(clipboardData, "base64") != NULL)
	{
		// Clipboard data contains a base64 encoded image	
		char* encodedImage = strstr(clipboardData, ",") + 1;
		if (!encodedImage)
		{
			ts3Functions->logMessage("Could not parse base64 encoded image", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
			return FALSE;
		}
		
		size_t decodedLength = 0;
		BYTE* decodedImage = EasyAvatar_b64decode(encodedImage, strlen(encodedImage), &decodedLength);
		FIMEMORY* mem = FreeImage_OpenMemory(decodedImage, decodedLength);
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(mem, 0);
		FreeImage_CloseMemory(mem);
		if (fif == FIF_UNKNOWN)
		{
			ts3Functions->logMessage("Invalid image from base64 decoding", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
			return FALSE;
		}

		FILE* fp;
		errno_t result = fopen_s(&fp, EASYAVATAR_IMAGEPATH, "wb");
		if (result != 0)
		{
			ts3Functions->logMessage("Failed to write decoded image to file", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
			return FALSE;
		}

		fwrite(decodedImage, 1, decodedLength, fp);
		fclose(fp);
		ts3Functions->freeMemory(decodedImage);
	} else // Treat clipboard data as an URL
	{
		HRESULT downloadRes = URLDownloadToFileA(NULL, clipboardData, EASYAVATAR_IMAGEPATH, 0, NULL);
		if (downloadRes != S_OK)
		{
			ts3Functions->logMessage("Download of image failed", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
			return FALSE;
		}
	}
	
	return TRUE;
}

BOOL EasyAvatar_GetFileFromClipboard(uint64 serverConnectionHandlerID)
{
	if (!OpenClipboard(NULL))
		return FALSE;

	UINT format = EnumClipboardFormats(0);

	// Never works
	if (!IsClipboardFormatAvailable(CF_BITMAP))
	{
		CloseClipboard();
		return FALSE;
	}

	HGLOBAL hGlobalMem = GetClipboardData(CF_BITMAP);
	BITMAPINFO* bitMapInfo = (BITMAPINFO*)GlobalLock(hGlobalMem);

	GlobalUnlock(hGlobalMem);
	CloseClipboard();
	return TRUE;


}

// Taken from https://stackoverflow.com/a/48818578

char* EasyAvatar_b64encode(const unsigned char* data, size_t input_length)
{
	const int mod_table[] = { 0, 2, 1 };

	size_t output_length = 4 * ((input_length + 2) / 3);

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
}

BYTE* EasyAvatar_b64decode(const char* data, size_t input_length, size_t* output_length)
{

	if (input_length % 4 != 0)
		return NULL;

	*output_length = input_length / 4 * 3;

	if (data[input_length - 1] == '=') (*output_length)--;
	if (data[input_length - 2] == '=') (*output_length)--;

	unsigned char* decoded_data = (unsigned char*)malloc(*output_length);

	if (decoded_data == NULL)
		return NULL;

	for (int i = 0, j = 0; i < input_length;) {

		unsigned int sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		unsigned int sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		unsigned int sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
		unsigned int sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

		unsigned int triple = (sextet_a << 3 * 6)
			+ (sextet_b << 2 * 6)
			+ (sextet_c << 1 * 6)
			+ (sextet_d << 0 * 6);

		if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;

	}

	return decoded_data;

}

// As specified by https://docs.microsoft.com/en-us/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content

char* EasyAvatar_CreateMD5Hash(const char* filePath, uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
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
		snprintf(errorMessage, sizeof(errorMessage), "Error opening file for hashing: Filepath: %s  Error Code: %d", filePath, GetLastError());
		ts3Functions->logMessage(errorMessage, LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
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
		// Nothing to do, imageMD5Hash will be NULL which will be handled by caller
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);

	return imageMD5Hash;
}

BOOL EasyAvatar_ResizeAvatar(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
{
	// Dynamically get the image type (png, jpg, etc...)
	FREE_IMAGE_FORMAT imgFormat = FreeImage_GetFileType(EASYAVATAR_IMAGEPATH, 0);
	if (imgFormat == FIF_UNKNOWN)
	{
		ts3Functions->logMessage("Tried loading unknown image format", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
		return FALSE;
	}

	// Skip resizing GIFs for now as they break while Saving
	if (imgFormat == FIF_GIF)
		return TRUE;

	FIBITMAP* avatarImage = FreeImage_Load(imgFormat, EASYAVATAR_IMAGEPATH, 0);
	if (!avatarImage)
	{
		// At this point we know the file is an image, only the resize process failed which isn't fatal
		return TRUE;
	}

	unsigned int originalH = FreeImage_GetHeight(avatarImage);
	unsigned int originalW = FreeImage_GetWidth(avatarImage);
	unsigned int targetH = originalH;
	unsigned int targetW = originalW;
	float aspectRatio = (float)originalW / (float)originalH;

	if (originalW > 300)
	{
		targetW = 300;
		targetH = (unsigned int)(targetW / aspectRatio);
	}
	else if (originalH > 300)
	{
		targetH = 300;
		targetW = (unsigned int)(targetH * aspectRatio);
	}

	// Resize our avatar
	FIBITMAP* resizedImage = FreeImage_Rescale(avatarImage, targetW, targetH, FILTER_BOX);
	if (!resizedImage)
	{
		FreeImage_Unload(avatarImage);
		return TRUE;
	}

	// FreeImage_Save overwriting old avatar file
	FreeImage_Save(imgFormat, resizedImage, EASYAVATAR_IMAGEPATH, 0);

	FreeImage_Unload(avatarImage);
	FreeImage_Unload(resizedImage);

	return TRUE;
}

BOOL EasyAvatar_CheckFileSize(uint64 serverConnectionHandlerID, struct TS3Functions* ts3Functions)
{
	// Teamspeak only accepts avatars under 200KB
	FILE* fp;
	errno_t result = fopen_s(&fp, EASYAVATAR_IMAGEPATH, "rb");
	if (fp)
	{
		fseek(fp, 0L, SEEK_END);
		size_t fileSize = ftell(fp);
		fclose(fp);

		if (fileSize > 200000)
		{
			ts3Functions->logMessage("Image is too large (> 200KB)", LogLevel_ERROR, EASYAVATAR_LOGCHANNEL, serverConnectionHandlerID);
			return FALSE;
		}
	}

	return TRUE;
}