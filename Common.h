#pragma once
#include<fstream>
#include <atlstr.h>
#include<string>
#include "curl/curl.h"
#include"cJSON.h"
#include <CMsgCenter.h>
#include "LogMgr.h"

#define ResponseDataLen 1024*100


//�������շ�������һ��buffer
typedef struct ResponseData_t {
	ResponseData_t() {
		data=nullptr;
		datalen = 0;
	}
	char *data;
	int datalen;
}ResponseData;

//��װ��ص��ֶ�
typedef struct MyData
{
	int ret;
	char data[256];
	char name[256];
	char localPath[256];
public:
	MyData() {
		memset(data, 0, 256);
		memset(name, 0, 256);
		memset(localPath, 0, 256);
		ret = -1;
	}
	void Clear()
	{
		g_LogMgr.TraceLog("Clear  start");
		g_LogMgr.TraceLog("data's address is:0x%x", data);
		
		memset(data, 0, 256);
		memset(name, 0, 256);
		memset(localPath, 0, 256);
		g_LogMgr.TraceLog("Clear  end");
		ret = -1;
	}
	void SignData(const std::string& mdata)
	{
		if (mdata.size() > 256)
			return;
		else
		{
			memset(data, 0, 256);
			memcpy(data, mdata.c_str(), mdata.size());
		}
	}
}MyData_t;


std::string CString2string(CString csStrData);

//����ӷ��������ص����ݣ�������copy��userdata
size_t WriteDataCallback(void* ptr, size_t size, size_t nmemb, void* userdata);

int HttpPost(std::string JsonData, std::string url, int timeout, void* userdata);

int HttpGet(std::string url, int port, void* userdata);



//���ػص�����
size_t WriteToFile(void* ptr, size_t size, size_t nmemb, FILE* stream);

//�����ļ�
int DownloadFile(const char* url, std::string name, std::string localPath,long timeout);

//��ȡ�����ļ���С
double getDownloadFileLenth(const char* url);

int WChar2AChar(const WCHAR* lpstr, char** pOut, UINT type);

std::string TCHAR2STRING(TCHAR* STR);

//��ȡ�����װ�ɹ�����״̬������0��װ�ɹ�
int GetAppname(std::string appname, std::string load, int Judge);


//��������İ汾
std::string GetAppVersion(std::string appname, std::string load, int Judge);