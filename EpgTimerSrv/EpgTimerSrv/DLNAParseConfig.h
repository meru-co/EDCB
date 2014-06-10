#pragma once

#define MAC_FILTER

#include <list>
#include "../../Common/CommonDef.h"
#include "../../Common/StringUtil.h"
#include "../../Common/LockUtil.h"

#include "DLNADmsContentDB.h"

#ifdef MAC_FILTER
#include <iphlpapi.h>
#endif

class CDLNAParseConfig
{
public:
	CDLNAParseConfig(void);
	~CDLNAParseConfig(void);
	void Clear(void);
	BOOL GetConfigError(int idx, wstring &error);

	BOOL ParseText(
		LPCWSTR filePath
		);	

	void GetServerName(wstring &name);
	void GetServerName(string &name);
	void WriteContentList(SYSTEMTIME* pTime, list<DLNA_DMS_CONTENT_INFO>* contentList);

#ifdef MAC_FILTER
	typedef struct _DLNA_MAC_ADDRESS{
		u_char b1, b2, b3, b4, b5, b6;
	}DLNA_MAC_ADDRESS;
	BOOL AcceptCheck(struct in_addr *pIpAddr, DLNA_MAC_ADDRESS *pMac=NULL);
#endif

protected:
	typedef struct _DLNA_CONFIG{
		wstring key;
		wstring value;
	}DLNA_CONFIG;
	vector<wstring> config_error;

protected:
	wstring servername;
	wstring logfile;
	int log_level;
	bool log_clear;
	wstring contentlist_file;

	static bool bFirst;

protected:
	void ConvertToFullPath(wstring& ret, wstring& path, LPCWSTR config_path);
	int SelectString(wstring &str, int num,...);
	void Format(string& ret, const char *format, va_list &arg);
	void Format(wstring& ret, const wchar_t *format, va_list &arg);
	void Format(string& ret, const char *format,...);
	void Format(wstring& ret, const wchar_t *format,...);
	BOOL Parse1Line(string parseLine, DLNA_CONFIG* cfg);

#ifdef MAC_FILTER
	int listmode;
	int localmode;
	list<DLNA_MAC_ADDRESS> macList;
	BOOL ParseMacAddress(wstring address, DLNA_MAC_ADDRESS* mac);
	BOOL IsLocalInterfaceAddress(struct in_addr *pSearch);
	BOOL IPAddr_to_MACAddr(struct in_addr *pSearch, DLNA_MAC_ADDRESS *pMac);
#endif

public:
	enum {
		LOG_INFO     =  10,
		LOG_ERROR    =  20,
		LOG_WARN     =  30,		
		LOG_DEBUG    = 255
	};
	void LogOutput(const char* s=NULL, const wchar_t* ws=NULL)
	{
		CLockUtil lock;
		lock.Lock(L"LogOutput()");
		FILE* fp;
		wchar_t *mode = (s||ws) ? L"at+,ccs=UTF-8" : L"wt+,ccs=UTF-8";
		if(_wfopen_s(&fp, logfile.c_str(), mode)==0) {
			if(s) {
				string str(s);
				wstring wstr;
				AtoW(str,wstr);
				fputws(wstr.c_str(), fp);
			}
			if(ws) fputws(ws, fp);
			fclose(fp);
		}
#ifdef _DEBUG
		if(s) OutputDebugStringA(s);
		if(ws) OutputDebugStringW(ws);
#endif
	}
	void Log(const int level, const wchar_t* format, va_list &arg)
	{
		if(logfile.empty() || log_level==0) return;
		if(level>log_level) return;

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);

		wchar_t date[30], time[30];
		GetDateFormat(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, &stTime, NULL, date, 30);
		GetTimeFormat(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, &stTime, NULL, time, 30);

		wstring buf, log;
		Format(buf, format, arg);
		Format(log, L"%10s %8s : %s", date, time, buf.c_str());
		LogOutput(NULL, log.c_str());
	}
	void Log(const int level, const char* format, va_list &arg)
	{
		if(logfile.empty() || log_level==0) return;
		if(level>log_level) return;
		char date[30], time[30];		

		SYSTEMTIME stTime;
		GetLocalTime(&stTime);
		GetDateFormatA(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, &stTime, NULL, date, 30);
		GetTimeFormatA(LOCALE_USER_DEFAULT, LOCALE_NOUSEROVERRIDE, &stTime, NULL, time, 30);

		string buf, log;
		Format(buf, format, arg);
		Format(log, "%10s %8s : %s", date, time, buf.c_str());
		LogOutput(log.c_str(), NULL);
	}
	void Log(const int level, const wchar_t* format,...)
	{
		va_list arg;
		va_start(arg, format);
		Log(level, format, arg);
		va_end(arg);
	}
	void Log(const int level, const char* format,...)
	{
		va_list arg;
		va_start(arg, format);
		Log(level, format, arg);
		va_end(arg);
	}
	BOOL IsLogDebug()
	{
		return log_level>=LOG_DEBUG;
	}
protected:
	void LogClear()
	{
		LogOutput();
	}
	void Error(size_t iLine, const wchar_t* format,...)
	{
		va_list arg;
		va_start(arg, format);

		const wchar_t* err_fmt = L"Line %d : %s";
		int fmtlen = _scwprintf(err_fmt, iLine, format)+1;
		wchar_t* fmt_buf = new wchar_t[fmtlen];
		swprintf_s(fmt_buf, fmtlen, err_fmt, iLine, format);

		int len = _vscwprintf(fmt_buf, arg)+1;
		wchar_t* buf = new wchar_t[len];
		vswprintf_s(buf, len, fmt_buf, arg);

		wstring tmp(buf);
		config_error.push_back(tmp);

		delete []fmt_buf;
		delete []buf;
		va_end(arg);
	}
};
