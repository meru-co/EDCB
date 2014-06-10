#pragma once

#include "../../Common/CommonDef.h"
#include "../../UPnPCtrl/UpnpSsdpUtil.h"

#include "DLNADmsManager.h"
#include "DLNAParseConfig.h"
#include "DLNAParseProtocolInfo.h"
#include "DLNAParsePublicFolder.h"

class CDLNAManager
{
public:
	CDLNAManager(void);
	~CDLNAManager(void);

	int StartSSDPServer(DWORD);
	int StopSSDPServer();

	int LoadPublicFolder();
	int AddDMSRecFile(wstring filePath);
	int StartDMS();
	int StopDMS();
	int EnumContentList(bool bWrite=false);

	int HttpRequest(string method, string uri, nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, struct sockaddr_in* client, HANDLE stopEvent);

protected:
	UPNP_SERVER_HANDLE upnpCtrl;

	CDLNAParseConfig config;
	CDLNAParseProtocolInfo protocolInfo;
	DLNAParsePublicFolder publicFolder;

	CDLNADmsManager dms;
	BOOL startDMS;
    DWORD httpPort;
	wstring recFolderObjectID;
	wstring publicFolderObjectID;

	map<wstring, wstring> recFileList;
protected:
	int AddFolderItem(wstring folderPath, wstring parentObjectID);
	int AddFileItem(wstring parentObjectID, wstring filePath, wstring& objectID);

	static void UpnpSSDPCallback( SSDP_NOTIFY_STATUS notifyType, void* notifyParam, void* param);
	static int UpnpMSearchReqCallback( UPNP_MSEARCH_REQUEST_INFO* requestParam, void* param, SORT_LIST_HANDLE resDeviceList);

public:
	void Log(const int level, const char* format,...)
	{
		va_list arg;
		va_start(arg, format);
		config.Log(level, format, arg);
		va_end(arg);
	}
	void Log(const int level, const wchar_t* format,...)
	{
		va_list arg;
		va_start(arg, format);
		config.Log(level, format, arg);
		va_end(arg);
	}
	BOOL IsLogDebug()
	{
		return config.IsLogDebug();
	}
};

