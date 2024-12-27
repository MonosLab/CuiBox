#include "pch.h"
#include "Version.h"
#include "wininet.h"
#include "json/json.h"

CVersion::CVersion()
{
}

CVersion::~CVersion()
{
}

bool CVersion::CheckUpdate(TCHAR* pszDomain, TCHAR* pszUrl)
{
	std::string strSvrVer("");
	bool bRet = GetServerVersion(pszDomain, pszUrl, strSvrVer);
	if (bRet)
	{
		PrintOutA(">> Get version info of server.\r\n%s\r\n", strSvrVer.c_str());
		std::string strLocVer("");
		if (!GetLocalVersion(strLocVer))
		{
			PrintOutA(">> Failed to get version info of local file.\r\n");
			return true;		// update
		}
		else
		{
			PrintOutA(">> Get version info of local file.\r\n%s\r\n", strLocVer.c_str());

			vector<VERSION_ITEM> vecLocVer;
			Json::Value root;
			Json::Value files;
			Json::Reader jsonDoc;
			if (jsonDoc.parse(strLocVer, root))
			{
				files = root["files"];
				int nCount = files.size();
				if (nCount == 0)
				{
					return true;		// update
				}
				else
				{
					for (int i = 0; i < nCount; ++i)
					{
						VERSION_ITEM item;
						item.filename = files[i].get("name", "").asString();
						item.dir = files[i].get("dir", "").asString();
						item.version = files[i].get("ver", 0).asInt();
						vecLocVer.push_back(item);
					}
				}
			}

			vector<VERSION_ITEM>::iterator its;
			vector<VERSION_ITEM>::iterator ite = vecLocVer.end();

			if (jsonDoc.parse(strSvrVer, root))
			{
				bool bUpdate = false;
				int ver = 0;
				string filename("");
				files = root["files"];
				int nCount = files.size();
				for (int i = 0; i < nCount; ++i)
				{
					its = vecLocVer.begin();
					filename = files[i].get("name", "").asString();
					for (its; its != ite; ++its)
					{
						if (filename.compare(its->filename) == 0)
						{
							ver = files[i].get("ver", 0).asInt();
							bUpdate = (ver != its->version);
							PrintOutA(">> %s : %ld (Is update : %s)\r\n", filename.c_str(), its->version, bUpdate ? "Yes" : "No");
							break;
						}
					}

					if (bUpdate) return true;
				}
			}
		}
	}
	else
	{
		PrintOutA(">> Failed to get version info of server.\r\n");
	}

	return false;
}

bool CVersion::GetServerVersion(TCHAR* pszDomain, TCHAR* pszUrl, std::string& strVersion)
{
	bool bOK = false;
	bool bError = false;
	HINTERNET hInet = InternetOpen(INET_AGENT, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

	int nProtocol = HTTP;
	CString strUrl(pszDomain);
	CString strServer, strObject;
	DWORD dwServiceType = 0;
	INTERNET_PORT nPort = 0;

	if (strUrl.Find(_T("https://"), 0) != -1)
	{
		nProtocol = HTTPS;
	}

	AfxParseURL(strUrl, dwServiceType, strServer, strObject, nPort);
	PrintOut(_T("%s, %d, %s, %s, %d\r\n"), strUrl, dwServiceType, strServer, strObject, nPort);

	if (hInet)
	{
		char buf[1024] = { NULL, };
		HINTERNET hCon = NULL;

		switch (nProtocol)
		{
			case HTTP:
				hCon = InternetConnect(hInet, strServer, nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
				break;
			case HTTPS:
				hCon = InternetConnect(hInet, strServer, nPort, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
				break;
		}

		if (hCon)
		{
			HINTERNET hRes = NULL;
			DWORD dwFlags = 0;
			DWORD dwLen = 0, nTotallen = 0;

			switch (nProtocol)
			{
				case HTTP:
					dwFlags = INTERNET_FLAG_RELOAD;
					if ((hRes = HttpOpenRequest(hCon, _T("GET"), pszUrl, HTTP_VERSION, NULL, NULL, dwFlags, 0)) != NULL)
					{
						HttpSendRequest(hRes, NULL, 0, NULL, 0);
					}
					break;
				case HTTPS:
				{
					LPTSTR szAcceptType[2] = { _T("*/*"), NULL };
					dwFlags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT | INTERNET_FLAG_SECURE;
					if ((hRes = HttpOpenRequest(hCon, _T("GET"), pszUrl, HTTP_VERSION, NULL, (LPCTSTR*)szAcceptType, dwFlags, 0)) != NULL)
					{
						USHORT nCount = 0;
						while (nCount < 5)
						{
							if (!::HttpSendRequest(hRes, NULL, 0, NULL, 0))
							{
#ifdef IGNORE_HTTPS_SECURITY
								DWORD dwFlags;
								DWORD dwBuffLen = sizeof(dwFlags);

								InternetQueryOption(hRes, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);
								dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
								dwFlags |= SECURITY_FLAG_IGNORE_REVOCATION;
								dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP;
								dwFlags |= SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS;
								dwFlags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
								dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
								InternetSetOption(hRes, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
#else
								if (GetLastError() == ERROR_INTERNET_INVALID_CA)
								{
									DWORD dwFlags;
									DWORD dwBuffLen = sizeof(dwFlags);

									InternetQueryOption(hRes, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);

									dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
									InternetSetOption(hRes, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
								}
#endif
							}
							else
							{
								break;
							}

							++nCount;
						}
					}
				}
				break;
			}

			if (hRes)
			{
				// 연결정보 확인
				DWORD dwInfoSize = 32;
				TCHAR szStatusCode[32];
				memset(szStatusCode, 0x00, sizeof(szStatusCode));
				HttpQueryInfo(hRes, HTTP_QUERY_STATUS_CODE, szStatusCode, &dwInfoSize, NULL);
				szStatusCode[dwInfoSize] = 0x00;
				long nStatusCode = _ttol(szStatusCode);
				if (nStatusCode == HTTP_STATUS_OK)
				{
					do
					{
						if (InternetReadFile(hRes, (LPVOID)buf, 1024, &dwLen))
						{
							if (dwLen != 0)
							{
								strVersion.insert(nTotallen, buf);
								memset(&buf, 0x00, sizeof(buf));
								nTotallen += dwLen;
							}
						}
						else
						{
							dwLen = 0;
							bError = true;
						}

					} while (dwLen != 0);

					//PrintOutA(">> Get version info.\r\n%s\r\n", szVer.c_str());
				}
				else
				{
					bError = true;
				}

				if (!bError)
				{
					strVersion.resize(nTotallen);
					bOK = TRUE;
				}
				InternetCloseHandle(hRes);
			}
			InternetCloseHandle(hCon);
		}
		InternetCloseHandle(hInet);
	}

	return bOK;
}

bool CVersion::GetLocalVersion(std::string& strVersion)
{
	TCHAR szVerFile[MAX_PATH2];
	memset(szVerFile, 0x00, sizeof(szVerFile));
	_stprintf_s(szVerFile, _T("%s%s"), CUtil::GetAppRootPath(), APP_VER_FILE);
	//PrintOut(szVerFile);
	FILE* fp = NULL;
	if (_wfopen_s(&fp, szVerFile, _T("rb")) == 0)
	{
		if (fp)
		{
			fseek(fp, 0, SEEK_END);
			long lSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			if (lSize != -1L)
			{
				char* buf = (char*)malloc(lSize + 1);
				if (buf != NULL)
				{
					memset(buf, 0x00, lSize + 1);
					fread(buf, lSize, 1, fp);
					buf[lSize] = 0x00;
					strVersion = string(buf);
				}
				free(buf);
			}
			fclose(fp);

			return true;
		}
		else
		{
			PrintOut(_T(">> File not found.\r\n"));
		}
	}
	return false;
}