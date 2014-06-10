#include "StdAfx.h"
#include "DLNAParseConfig.h"

#define DEFAULT_EPGTIMER_SERVERNAME L"EpgTimerSrv MediaServer"

bool CDLNAParseConfig::bFirst = true;

CDLNAParseConfig::CDLNAParseConfig(void)
{
	Clear();
}

CDLNAParseConfig::~CDLNAParseConfig(void)
{
	Log(LOG_INFO, L"Log end.\n");
	Log(LOG_INFO, L"\n");
}

void CDLNAParseConfig::Clear(void)
{
	config_error.clear();
	servername = DEFAULT_EPGTIMER_SERVERNAME;
	logfile.clear();
	log_level = 0;
	log_clear = true;
	contentlist_file.clear();
#ifdef MAC_FILTER
	listmode = 0;
	localmode = 1;
	macList.clear();
#endif
}

BOOL CDLNAParseConfig::GetConfigError(int idx, wstring &error)
{
	try {
		error = config_error.at(idx);
	} catch(std::out_of_range e) {
		return FALSE;
	}
	return TRUE;
}

BOOL CDLNAParseConfig::ParseText(LPCWSTR filePath)
{
	if( filePath == NULL ){
		return FALSE;
	}

	Clear();

	HANDLE hFile = CreateFile( filePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return FALSE;
	}
	DWORD dwFileSize = GetFileSize( hFile, NULL );
	if( dwFileSize == 0 ){
		CloseHandle(hFile);
		return TRUE;
	}
	char* pszBuff = new char[dwFileSize+1];
	if( pszBuff == NULL ){
		CloseHandle(hFile);
		return FALSE;
	}
	ZeroMemory(pszBuff,dwFileSize+1);
	DWORD dwRead=0;
	ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );

	string strRead = pszBuff;

	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(pszBuff)

	string parseLine="";
	size_t iIndex = 0;
	size_t iFind = 0;
	size_t iLine = 0;
	while( iFind != string::npos ){
		iFind = strRead.find("\r\n", iIndex);
		if( iFind == (int)string::npos ){
			parseLine = strRead.substr(iIndex);
			//strRead.clear();
		}else{
			parseLine = strRead.substr(iIndex,iFind-iIndex);
			//strRead.erase( 0, iIndex+2 );
			iIndex = iFind + 2;
			iLine++;
		}
		if( parseLine.find(";") != 0 ){
			DLNA_CONFIG Item;			
			if( Parse1Line(parseLine, &Item) == TRUE ){
				// キー名は大文字小文字を区別しない
				transform(Item.key.begin(),Item.key.end(), Item.key.begin(), tolower);
				if(Item.key.compare(L"servername") == 0) {
					servername = Item.value;
				} else if(Item.key.compare(L"log") == 0) {
					ConvertToFullPath(logfile, Item.value, filePath);
				} else if(Item.key.compare(L"loglevel") == 0) {
					log_level = _wtoi(Item.value.c_str());
				} else if(Item.key.compare(L"logclear") == 0) {
					transform(Item.value.begin(),Item.value.end(), Item.value.begin(), tolower);
					int ret = SelectString(Item.value, 2, L"false", L"true");
					if(ret!=-1) {
						log_clear = ret==1 ? true : false;
					} else {
						Error(iLine, L"logclear value error \'%s\'", Item.value.c_str());
					}
				} else if(Item.key.compare(L"contentlist") == 0) {
					ConvertToFullPath(contentlist_file, Item.value, filePath);
				}
#ifdef MAC_FILTER
				 else if(Item.key.compare(L"listmode") == 0) {
					transform(Item.value.begin(),Item.value.end(), Item.value.begin(), tolower);
					int ret = SelectString(Item.value, 2, L"deny", L"accept");
					if(ret!=-1) {
						listmode = ret;
					} else {
						Error(iLine, L"listmode value error \'%s\'", Item.value.c_str());
					}
				} else if(Item.key.compare(L"local") == 0) {
					transform(Item.value.begin(),Item.value.end(), Item.value.begin(), tolower);
					int ret = SelectString(Item.value, 2, L"deny", L"accept");
					if(ret!=-1) {
						localmode = ret;
					} else {
						Error(iLine, L"local value error \'%s\'", Item.value.c_str());
					}
				} else if(Item.key.compare(L"mac") == 0) {
					DLNA_MAC_ADDRESS mac;
					if(ParseMacAddress(Item.value, &mac))
					{
						macList.push_back(mac);
					} else {
						Error(iLine, L"Invalid MAC Address \'%s\'", Item.value.c_str());
					}
				 }
#endif
				else {
					Error(iLine, L"Unknown config key \'%s\'", Item.key.c_str());
				}
			}
		}
	}

	if(bFirst){
		bFirst=false;
		if(log_clear){
			LogClear();
		}
	}

	Log(LOG_INFO, L"\n");
	Log(LOG_INFO, L"Log start.\n");
	Log(LOG_INFO, L"Config Loaded.\n");
	Log(LOG_DEBUG, L"  servername=%s\n", servername.c_str());
	Log(LOG_DEBUG, L"  loglevel=%d\n", log_level);
	Log(LOG_DEBUG, L"  logclear=%s\n", log_clear? L"True" : L"False");
	Log(LOG_DEBUG, L"  contentlist=%s\n", contentlist_file.c_str());
#ifdef MAC_FILTER
	Log(LOG_DEBUG, L"  listmode=%s\n", listmode==0 ? L"Deny" : L"Accept");
	Log(LOG_DEBUG, L"  local=%s\n", localmode==0 ? L"Deny" : L"Accept");
	Log(LOG_DEBUG, L"  MAC\n");
	list<DLNA_MAC_ADDRESS>::iterator it;
	for(it=macList.begin(); it!=macList.end(); it++){
		Log(LOG_DEBUG, L"   >%02X:%02X:%02X:%02X:%02X:%02X\n", it->b1, it->b2, it->b3, it->b4, it->b5, it->b6);
	}
#endif
	Log(LOG_INFO, L"\n");
	if(config_error.size()>0)
	{
		wstring err;
		Log(LOG_INFO, L"<< Config Error >>\n");
		for(int i=0; GetConfigError(i, err); i++)
		{
			Log(LOG_WARN, L" %s\n", err.c_str());
		}
	}
	return TRUE;
}


void CDLNAParseConfig::GetServerName(wstring& name)
{
	name = servername;
}
void CDLNAParseConfig::GetServerName(string& name)
{
	WtoA(servername, name);
}

void CDLNAParseConfig::WriteContentList(SYSTEMTIME* pTime, list<DLNA_DMS_CONTENT_INFO>* contentList)
{
	if(!contentList) return;
	
	wstring str;
	FILE *fp = NULL;

	Format(str, L"Media server is %Iu content.\n", contentList->size());
//	Log(CDLNAParseConfig::LOG_INFO, L"%s", str.c_str());
	if(_wfopen_s(&fp, contentlist_file.c_str(), L"wt,ccs=UTF-8")!=0) fp=NULL;
	if(fp) {	
		SYSTEMTIME stTime;
		if(!pTime) {
			pTime = &stTime;
			GetLocalTime(pTime);
		}

		wchar_t date[30], time[30];
		GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, pTime, NULL, date, 30);
		GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, pTime, NULL, time, 30);
		fputws(str.c_str(), fp);
		Format(str, L"  Update Time : %s %s\n\n", date, time);
		fputws(str.c_str(), fp);

		fputws(L"Content List\n", fp);
	}		

	BOOL bLog = IsLogDebug();
	if(fp || bLog) {
		for(list<DLNA_DMS_CONTENT_INFO>::iterator it=contentList->begin(); it!=contentList->end(); it++)
		{		
			// MBに変換
			long long fileSize = it->info.fileSize >> 20;		
			Format(str, L"  [%5s]%5.lldMB %s : %s\n", it->objectID.c_str(), fileSize, it->uri.c_str(), it->info.filePath.c_str());

			if(fp) {
				fputws(str.c_str(), fp);
			}
			if(bLog) {						
				Log(CDLNAParseConfig::LOG_DEBUG, L"%s", str.c_str());
			}
		}
	}
	if(fp) fclose(fp);
}

BOOL CDLNAParseConfig::Parse1Line(string parseLine, DLNA_CONFIG* cfg )
{
	if( parseLine.empty() == true || cfg == NULL ){
		return FALSE;
	}
	string strBuff="";

	// Key(大文字が含まれる場合小文字にする)
	Separate( parseLine, "=", strBuff, parseLine);
	AtoW(strBuff, cfg->key);
	transform(cfg->key.begin(),cfg->key.end(), cfg->key.begin(), tolower);

	// Value
	Separate( parseLine, "=", strBuff, parseLine);
	AtoW(strBuff, cfg->value);
	return TRUE;
}

void CDLNAParseConfig::ConvertToFullPath(wstring& ret, wstring& path, LPCWSTR config_path)
{
	size_t pos = path.rfind('\\');
	if(pos != string::npos) {
		// フルパス
		ret =path;
	} else {
		// ファイル名のみ
		ret = config_path;
		pos = ret.rfind('\\');
		ret = ret.substr(0, pos+1);
		ret.append(path);
	}
}

int CDLNAParseConfig::SelectString(wstring &str, int num,...)
{	
    va_list ap;
    va_start(ap, num);
	int ret = -1;
	for(int i=0; i<num && ret==-1; i++) {
		wchar_t *p= va_arg(ap, wchar_t*);
		if(str.compare(p)==0) ret=i;
	}

	va_end(ap);
	return ret;
}

void CDLNAParseConfig::Format(string& ret, const char *format, va_list &arg)
{
	int len = _vscprintf(format, arg)+1;
	char* buf = new char[len];
	vsprintf_s(buf, len, format, arg);
	ret = buf;
	delete []buf;
}
void CDLNAParseConfig::Format(wstring& ret, const wchar_t *format, va_list &arg)
{
	int len = _vscwprintf(format, arg)+1;
	wchar_t* buf = new wchar_t[len];
	vswprintf_s(buf, len, format, arg);
	ret = buf;
	delete []buf;
}
void CDLNAParseConfig::Format(string& ret, const char *format,...)
{
    va_list ap;
    va_start(ap, format);
	Format(ret, format, ap);
	va_end(ap);
}
void CDLNAParseConfig::Format(wstring& ret, const wchar_t *format,...)
{
    va_list ap;
    va_start(ap, format);
	Format(ret, format, ap);
	va_end(ap);
}

#ifdef MAC_FILTER
// MACアドレスを数値に直す
BOOL CDLNAParseConfig::ParseMacAddress(wstring address, DLNA_MAC_ADDRESS* mac)
{
	if( address.empty() == true || mac == NULL ){
		return FALSE;
	}

	wstring tmp(address);
	size_t pos;
	while((pos = address.find_first_not_of(L"0123456789ABCDEFabcdef")) != string::npos) {
		address.erase(pos,1);
	}

	if(address.size() < 12){
		return FALSE;
	}
	wchar_t *p;
	mac->b1 = (u_char)(wcstol(address.substr( 0,2).c_str(), &p, 16));
	mac->b2 = (u_char)(wcstol(address.substr( 2,2).c_str(), &p, 16));
	mac->b3 = (u_char)(wcstol(address.substr( 4,2).c_str(), &p, 16));
	mac->b4 = (u_char)(wcstol(address.substr( 6,2).c_str(), &p, 16));
	mac->b5 = (u_char)(wcstol(address.substr( 8,2).c_str(), &p, 16));
	mac->b6 = (u_char)(wcstol(address.substr(10,2).c_str(), &p, 16));

	return TRUE;
}

// 指定アドレスがローカルのいんたーふぇーすのものかを判定
BOOL CDLNAParseConfig::IsLocalInterfaceAddress(struct in_addr *pSearch)
{
	DWORD dwRes;
	DWORD dwSize;
	dwRes = GetIpAddrTable(NULL, &dwSize, 0);
	if(dwRes != ERROR_INSUFFICIENT_BUFFER) {
		return FALSE;
	}
	PMIB_IPADDRTABLE pTbl = (PMIB_IPADDRTABLE)malloc(dwSize);
	if(!pTbl) return FALSE;
	BOOL bFind = FALSE;
	dwRes = GetIpAddrTable(pTbl, &dwSize, 0);
	for (DWORD i=0; i<pTbl->dwNumEntries && !bFind; i++) {
		MIB_IPADDRROW& Item = pTbl->table[i];
		struct in_addr *pIpAddr = (struct in_addr *)&Item.dwAddr;
		if(pSearch) {
			if(pSearch->S_un.S_addr == pIpAddr->S_un.S_addr) {
				bFind = TRUE;
			}
		}
	}
	free(pTbl);
	return bFind;
}

// ArpテーブルからMacアドレスを検索
BOOL CDLNAParseConfig::IPAddr_to_MACAddr(struct in_addr *pSearch, DLNA_MAC_ADDRESS *pMac)
{
	DWORD dwRes;
	DWORD dwSize;
	dwRes = GetIpNetTable(NULL, &dwSize, 0);
	if(dwRes != ERROR_INSUFFICIENT_BUFFER) {
		return FALSE;
	}	

	PMIB_IPNETTABLE pTbl = (PMIB_IPNETTABLE)malloc(dwSize);
	if(!pTbl) return FALSE;
	BOOL bFind = FALSE;
	dwRes = GetIpNetTable(pTbl, &dwSize, 0);
	for (DWORD i=0; i<pTbl->dwNumEntries && !bFind; i++) {
		MIB_IPNETROW& Item = pTbl->table[i];
		struct in_addr *pIpAddr = (struct in_addr *)&Item.dwAddr;
		if(pSearch) {
			if(pSearch->S_un.S_addr == pIpAddr->S_un.S_addr) {
				if(Item.dwPhysAddrLen==6) {
					if(pMac)
					{
						pMac->b1 = Item.bPhysAddr[0];
						pMac->b2 = Item.bPhysAddr[1];
						pMac->b3 = Item.bPhysAddr[2];
						pMac->b4 = Item.bPhysAddr[3];
						pMac->b5 = Item.bPhysAddr[4];
						pMac->b6 = Item.bPhysAddr[5];							
					}
					bFind = TRUE;
				} else {
					// Macアドレスが42bitでない場合
				}
			}
		}
	}
	free(pTbl);
	return bFind;
}

BOOL CDLNAParseConfig::AcceptCheck(struct in_addr *pIpAddr, DLNA_MAC_ADDRESS *pMac)
{
	DLNA_MAC_ADDRESS mac;
	if(pIpAddr){
		if(pIpAddr->S_un.S_un_b.s_b1==127){
			// ローカルホスト(127.0.0.0/8)はarpテーブルにないので許可
			Log(LOG_DEBUG, L"%d.%d.%d.%d is localhost\n", pIpAddr->S_un.S_un_b.s_b1, pIpAddr->S_un.S_un_b.s_b2, pIpAddr->S_un.S_un_b.s_b3, pIpAddr->S_un.S_un_b.s_b4);
			if(pMac) memset(pMac, 0, sizeof(DLNA_MAC_ADDRESS));
			return localmode==1 ? TRUE : FALSE;
		}
		if(IsLocalInterfaceAddress(pIpAddr)) {
			// ローカルインターフェースに一致するものは許可
			Log(LOG_DEBUG, L"%d.%d.%d.%d is local interface\n", pIpAddr->S_un.S_un_b.s_b1, pIpAddr->S_un.S_un_b.s_b2, pIpAddr->S_un.S_un_b.s_b3, pIpAddr->S_un.S_un_b.s_b4);
			if(pMac) memset(pMac, 0, sizeof(DLNA_MAC_ADDRESS));
			return localmode==1 ? TRUE : FALSE;
		}
	}
	if(!IPAddr_to_MACAddr(pIpAddr, &mac)) {
		// arpから見つからない
		Log(LOG_WARN, L"%d.%d.%d.%d is unknown MAC Address\n", pIpAddr->S_un.S_un_b.s_b1, pIpAddr->S_un.S_un_b.s_b2, pIpAddr->S_un.S_un_b.s_b3, pIpAddr->S_un.S_un_b.s_b4);
		if(pMac) memset(pMac, 0, sizeof(DLNA_MAC_ADDRESS));
		return FALSE;
	}
	if(pMac) *pMac = mac;

	BOOL bFind = FALSE;
	list<DLNA_MAC_ADDRESS>::iterator it;
	for(it=macList.begin(); it!=macList.end() && !bFind; it++) {
		if(memcmp(&(*it), &mac, sizeof(DLNA_MAC_ADDRESS))==0)
		{
			bFind = TRUE;
		}
	}

	switch(listmode) {
	case 0:
		// Deny(Blacklist) mode
		return !bFind;
		break;
	case 1:
		// Accept(Whitelist) mode
		return bFind;
		break;
	default:
		break;
	}
	return TRUE;
}
#endif