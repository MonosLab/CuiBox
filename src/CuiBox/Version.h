#ifndef __MONOSLAB_VERSION_H_20240716__
#define __MONOSLAB_VERSION_H_20240716__

class CVersion
{
public:
	CVersion();
	~CVersion();

	bool CheckUpdate(TCHAR* pszDomain, TCHAR* pszUrl);

protected:
	bool GetServerVersion(TCHAR* pszDomain, TCHAR* pszUrl, std::string& strVersion);
	bool GetLocalVersion(std::string& strVersion);
	
};

#endif	// #ifndef __MONOSLAB_VERSION_H_20240716__