

#include "Common.h"



std::string CString2string(CString csStrData)
{
	int lengthOfWcs1 = MultiByteToWideChar(CP_ACP, 0, csStrData, -1, NULL, 0);
	wchar_t* wcs1 = new wchar_t[lengthOfWcs1 + 1];
	MultiByteToWideChar(CP_ACP, 0, csStrData, -1, wcs1, lengthOfWcs1);

	char* pOut = nullptr;
	int n = WChar2AChar(wcs1, &pOut, CP_UTF8);
	if (pOut)
	{
		std::string strRet = std::string(pOut);
		delete wcs1;
		wcs1 = NULL;
		return strRet;
	}


	delete wcs1;
	wcs1 = NULL;

	return "";
}


std::string GetStrName(const char* url) {
	std::string strs(url);
	//rfind()函数的使用，返回查找对象最后出现的位置
	int pos = strs.rfind("/") + 1;
	//截取s中从从pos开始（包括0）到末尾的所有字符的子串，并返回
	std::string s = strs.substr(pos);
	return s;
}

//这个函数为CURLOPT_HEADERFUNCTION参数构造
/* 从http头部获取文件size*/
size_t ContentLengthFunc(void* ptr, size_t size, size_t nmemb, void* stream) {
	int ret;
	long len = 0;
	ret = sscanf_s((const char*)ptr, "%ld", &len);
	if (ret == 1)
		*((long*)stream) = len;

	return size * nmemb;
}

/*
ptr表示收到服务器返回数据的首地址
size表示返回每个数据的大小
nmemb表示返回数据的个数
userdata用户给该回调函数传递的形参
*/
//处理从服务器返回的数据，将数据copy到userdata

size_t WriteDataCallback(void* contents, size_t size, size_t nmemb, void* userdata)
{
	g_LogMgr.TraceLog("WriteDataCallback count count");
	size_t realsize = size * nmemb;
	ResponseData* mem = (ResponseData*)userdata;

	char* ptr = (char *)realloc(mem->data, mem->datalen + realsize + 1);
	if (!ptr) {
		/* out of memory! */
		g_LogMgr.TraceLog("not enough memory (realloc returned NULL)");
		return 0;
	}

	mem->data = (char *)ptr;
	memcpy(&(mem->data[mem->datalen]), contents, realsize);
	mem->datalen += realsize;
	mem->data[mem->datalen] = 0;

	g_LogMgr.TraceLog("WriteDataCallback ptr ptr %s", contents);
	g_LogMgr.TraceLog("WriteDataCallback ((ResponseData*)userdata)->data ptr %s", ((ResponseData*)userdata)->data);
	g_LogMgr.TraceLog("WriteDataCallback count count %d", ((ResponseData*)userdata)->datalen);

	return realsize;

	
	return ((ResponseData*)userdata)->datalen;
}

int HttpPost(std::string JsonData, std::string url, int timeout, void* userdata) {
	g_LogMgr.TraceLog("HttpPost start");
	g_LogMgr.TraceLog("HttpPost data --%s", JsonData.c_str());
	CURL* curl = NULL;
	CURLcode res = CURLE_GOT_NOTHING;

	//1.初始化
	curl = curl_easy_init();
	if (curl == NULL) {
		return -2;
	}
	//2.ssl认证
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, true);
	//3.URL地址
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "");

	struct curl_slist* head = NULL;
	head = curl_slist_append(head, "Content-Type:application/x-www-form-urlencoded;charset=UTF-8");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, head);
	
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	//
	//4.设置为非0表示本次操作为POST
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	//5.Post数据
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, JsonData);
	//6.设置一个处理服务器响应的回调函数
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataCallback);
	//7.对回调函数第四个参数进行设置
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
	//8.向服务器发送请求
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		
		return res;
	}
	//9.释放内存
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	g_LogMgr.TraceLog("HttpPost end");
	return 0;
}

int HttpGet(std::string url, int port, void* userdata) {
	//1.初始化
	int ret = -1;
	CURL* curl = NULL;
	CURLcode res = CURLE_GOT_NOTHING;

	curl = curl_easy_init();
	if (curl == NULL) {
		return -2;
	}
	//2.ssl认证
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, true);
	//3.URL地址
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_PORT, port);
	//4.设置为非0表示本次操作为
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
	//6.设置一个处理服务器响应的回调函数
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteDataCallback);
	//7.对回调函数第四个参数进行设置
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, userdata);
	//8.向服务器发送请求
	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		return -3;
	}
	//9.释放内存
	curl_easy_cleanup(curl);
	curl_global_cleanup();
	ret = 0;

	return ret;

}


size_t WriteToFile(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}


int  DownloadFile(const char* url,std::string name, std::string localPath,long timeout) {

	int ret = 0;
	int errNumber = 0;
	//采用追加方式打开文件，便于实现文件断点续传工作
	localPath += "\\";
	localPath += name;
	localPath += ".exe";

	g_LogMgr.TraceLog("DownloadFile localPath.c_str() msg is %s", localPath.c_str());

	FILE* fp;
	errNumber = fopen_s(&fp, localPath.c_str(), "ab+");
	if (errNumber) {
		return -1;
	}

	//1.初始化
	CURL* handle = NULL;
	curl_global_init(CURL_GLOBAL_ALL);
	handle = curl_easy_init();
	if (handle == NULL) {
		return -1;
	}

	//得到本地文件大小 
	curl_off_t localFileLength = -1;
	long fileSize = 0;
	CURLcode res = CURLE_GOT_NOTHING;
	struct stat fileInfo;
	int useResume = 0;
	if (stat(localPath.c_str(), &fileInfo) == 0)
	{
		localFileLength = fileInfo.st_size;
		useResume = 1;
	}

	//3.URL地址
	curl_easy_setopt(handle, CURLOPT_URL, url);

	//设置连接超时，单位秒
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, timeout);

	//设置http 头部处理函数
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, ContentLengthFunc);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, &fileSize);

	//设置文件续传的位置给libcurl
	curl_easy_setopt(handle, CURLOPT_RESUME_FROM_LARGE, useResume ? localFileLength : 0);

	//7.对回调函数第四个参数进行设置
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
	//6.设置一个处理服务器响应的回调函数
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteToFile);

	//启用时关闭curl传输的进度条，此项的默认设置为启用。
	curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 1L);
	//启用时会汇报所有的信息，存放在STDERR或指定的CURLOPT_STDERR中。
	curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);

	//8.向服务器发送请求
	res = curl_easy_perform(handle);
	if (res != CURLE_OK) {
		curl_easy_cleanup(handle);
		curl_global_cleanup();
		ret = -1;
	}
	
	fclose(fp);
	//9.释放内存
	curl_easy_cleanup(handle);
	curl_global_cleanup();

	return ret;
}


double getDownloadFileLenth(const char* url) {

	int downloadFileLenth = 0;

	CURL* handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);

	curl_easy_setopt(handle, CURLOPT_HEADER, 1);    //只需要header头

	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);    //不需要body

	if (curl_easy_perform(handle) == CURLE_OK) {

		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);

	}
	else {

		downloadFileLenth = -1;

	}

	return downloadFileLenth;

}

//若已经安装将返回0，其他返回值为未安装
int GetAppname(std::string appname, std::string load, int Judge) {
	int ret = -1;

	HKEY hKEY;
	HKEY hKEYResult;

	char tchData[512];//存储字符串	
	long lRet;//记录函数的返回值方便调试	
	DWORD dwSize;

	TCHAR szKeyName[255] = { 0 };// 注册表项名称
	DWORD index = 0;
	DWORD dwKeyLen = 255;
	std::string strBuffer;
	std::string  strMidReg;

	//std::string data_Set ="SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";//示例的注册表操作路径

	std::string data_Set;//示例的注册表操作路径
	data_Set += load;
	if (Judge == 1) {
		lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, data_Set.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEY);
	}
	else {
		lRet = RegOpenKeyExA(HKEY_CURRENT_USER, data_Set.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEY);
	}

	if (ERROR_SUCCESS == lRet) {

		while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKEY, index, szKeyName, &dwKeyLen, 0, NULL, NULL, NULL)) {
			index++;

			strBuffer += TCHAR2STRING(szKeyName);
			printf("index:%d\n", index);
			if (!strBuffer.empty())
			{
				long temp = -1;
				strMidReg += data_Set;
				strMidReg += "\\";
				strMidReg += strBuffer;
				printf("%s\n", strMidReg.c_str());
				if (Judge == 1) {
					temp = RegOpenKeyExA(HKEY_LOCAL_MACHINE, strMidReg.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEYResult);
				}
				else {
					temp = RegOpenKeyExA(HKEY_CURRENT_USER, strMidReg.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEYResult);
				}

				if (temp == ERROR_SUCCESS)
				{
					dwSize = sizeof(tchData);
					RegQueryValueExA(hKEYResult, "DisplayName", 0, 0, (LPBYTE)tchData, &dwSize);

					printf("DisplayName:%s\n", tchData);
					ret = strcmp(appname.c_str(), tchData);
					printf("ret:%d\n", ret);
					if (ret == 0) {
						return ret;
					}
				}

			}
			memset(szKeyName, 0, 255);
			dwKeyLen = 255;
			strMidReg = "";
			strBuffer = "";

		}
	}
	else {
		printf("RegOpenKeyExA open fail\n");
	}
	return ret;
}

int WChar2AChar(const WCHAR* lpstr, char** pOut, UINT type)
{
	int count = wcslen(lpstr);
	if (pOut == NULL || count == 0)
		return 0;

	int nACharCount = ::WideCharToMultiByte(type, 0, lpstr, -1, NULL, 0, NULL, NULL);
	*pOut = new char[nACharCount];
	memset(*pOut, 0, sizeof(char) * (nACharCount));

	int result = ::WideCharToMultiByte(type, 0, lpstr, -1, *pOut, nACharCount, NULL, NULL);

	if (*pOut == NULL || result > nACharCount)
	{
		return 0;
	}

	return nACharCount;
}

//TCHAR转string--中文转换会有问题乱码
std::string TCHAR2STRING(TCHAR* STR)
{
	int lengthOfWcs1 = MultiByteToWideChar(CP_ACP, 0, STR, -1, NULL, 0);
	wchar_t* wcs1 = new wchar_t[lengthOfWcs1 + 1];
	MultiByteToWideChar(CP_ACP, 0, STR, -1, wcs1, lengthOfWcs1);

	char* pOut = nullptr;
	int n = WChar2AChar(wcs1, &pOut, CP_UTF8);
	if (pOut)
	{
		std::string strRet = std::string(pOut);
		delete wcs1;
		wcs1 = NULL;
		return strRet;
	}


	delete wcs1;
	wcs1 = NULL;

	return "";
}

//返回app的版本
std::string GetAppVersion(std::string appname, std::string load, int Judge) {
	std::string version;

	int ret = -1;

	HKEY hKEY;
	HKEY hKEYResult;

	char tchData[512];//存储字符串	
	long lRet;//记录函数的返回值方便调试	
	DWORD dwSize;

	TCHAR szKeyName[255] = { 0 };// 注册表项名称
	DWORD index = 0;
	DWORD dwKeyLen = 255;
	std::string strBuffer;
	std::string  strMidReg;
	//std::string data_Set = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
	std::string data_Set;//示例的注册表操作路径
	data_Set += load;

	if (Judge == 1) {
		lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE, data_Set.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEY);
	}
	else {
		lRet = RegOpenKeyExA(HKEY_CURRENT_USER, data_Set.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEY);
	}

	if (ERROR_SUCCESS == lRet) {

		while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hKEY, index, szKeyName, &dwKeyLen, 0, NULL, NULL, NULL)) {
			index++;

			strBuffer += TCHAR2STRING(szKeyName);
			printf("index:%d\n", index);
			if (!strBuffer.empty())
			{
				long temp = -1;
				strMidReg += data_Set;
				strMidReg += "\\";
				strMidReg += strBuffer;
				printf("%s\n", strMidReg.c_str());

				if (Judge == 1) {
					temp = RegOpenKeyExA(HKEY_LOCAL_MACHINE, strMidReg.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEYResult);
				}
				else {
					temp = RegOpenKeyExA(HKEY_CURRENT_USER, strMidReg.c_str(), 0, KEY_WOW64_64KEY | KEY_READ, &hKEYResult);
				}
				if (temp == ERROR_SUCCESS)
				{
					dwSize = sizeof(tchData);
					RegQueryValueExA(hKEYResult, "DisplayName", 0, 0, (LPBYTE)tchData, &dwSize);
					printf("DisplayName:%s\n", tchData);
					ret = strcmp(appname.c_str(), tchData);
					printf("ret:%d\n", ret);
					if (ret == 0) {
						memset(tchData, 0, 255);
						dwSize = 255;
						RegQueryValueExA(hKEYResult, "DisplayVersion", 0, 0, (LPBYTE)tchData, &dwSize);
						printf("DisplayVersion:%s\n", tchData);
						version += tchData;
						return version;
					}
				}

			}
			memset(szKeyName, 0, 255);
			dwKeyLen = 255;
			strMidReg = "";
			strBuffer = "";
		}
	}
	else {
		printf("RegOpenKeyExA open fail\n");
	}

	return version;
}
