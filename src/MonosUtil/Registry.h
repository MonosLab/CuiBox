#ifndef __REGISTRY_H_MONO_20230214__
#define __REGISTRY_H_MONO_20230214__

#include <winreg.h>

class CRegistry
{
public:
	CRegistry();
	virtual ~CRegistry();

public:
	BOOL DeleteKey(LPCTSTR lpSubKey, HKEY hKey = HKEY_CURRENT_USER);
	BOOL IsValid(LPCTSTR lpKey);
	BOOL Delete(LPCTSTR lpKey);
	BOOL CreateKey(LPCTSTR lpSubKey, HKEY hKey = HKEY_CURRENT_USER);
	BOOL Open(LPCTSTR lpSubKey, HKEY hKey = HKEY_CURRENT_USER, BOOL bCreate = TRUE);
	void Close();
	BYTE ReadByte(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, BYTE BData);
	int ReadInt(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, int nData);
	CString ReadStr(LPCTSTR lpKey);
	BOOL Write(LPCTSTR lpKey, LPCTSTR lpData);
	HKEY GetCurKey(){ return m_hKey; };

private:
	HKEY 	m_hKey;
};

#endif	// #ifndef __REGISTRY_H_MONO_20230214__