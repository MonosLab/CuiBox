#ifndef __RESOURCEEX_H_MONO_0230214__
#define __RESOURCEEX_H_MONO_0230214__

enum eResourceType
{
	IDS_FONT_DEFAULTFACE = 9000,
	IDS_CUIBOX = 10000,
	IDS_STARTUP,
	IDS_OPEN,
	IDS_OPEN_OUTPUT,
	IDS_CLOSE,
	IDS_RESET,
	IDS_APPLY_CHANGES,
	IDS_RUN_MODE,
	IDS_RESTART_PROGRAM,
	IDS_SELECT_LANGUAGE,
	IDS_KOREAN,
	IDS_ENGLISH,
	IDS_CHINESE,
	IDS_JAPANESE,
	IDS_VIETNAMESE,
	IDS_INSTALL_NVIDIA_DRV,
};

class CResourceEx
{
private:
	CResourceEx();
	CResourceEx(const CResourceEx& other);
	~CResourceEx();

	static CResourceEx* m_pInstance;
	static void Destroy()
	{
		delete m_pInstance;
	}

public:
	static CResourceEx* GetInstance()
	{
		if (m_pInstance == NULL)
		{
			m_pInstance = new CResourceEx();
			atexit(Destroy);
		}
		return m_pInstance;
	}

	void SetLanguage(_LANGUAGE nType = LANG_NULL)
	{
		m_nLangType = nType;
		memset(m_szLang, 0x00, sizeof(m_szLang));
		switch (nType)
		{
			case KO:
				_stprintf_s(m_szLang, _T("ko"));
				break;
			case EN:
				_stprintf_s(m_szLang, _T("en"));
				break;
			case JA:
				_stprintf_s(m_szLang, _T("ja"));
				break;
			case ZH:
				_stprintf_s(m_szLang, _T("zh"));
				break;
			case VI:
				_stprintf_s(m_szLang, _T("vi"));
				break;
			default:
				_stprintf_s(m_szLang, _T("ko"));
				break;
		}
	};
	UINT16 GetLanguage() { return m_nLangType; };
	TCHAR* GetLanguageString() { return m_szLang; };

	CString LoadStringEx(UINT32 nType);
	string LoadStringExA(UINT32 nType);

private:
	UINT16 m_nLangType;
	TCHAR m_szLang[8];
};

#endif	// #ifndef __RESOURCEEX_H_MONO_0230214__
