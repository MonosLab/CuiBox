#ifndef __MONOS_UTIL_H_20241226__
#define __MONOS_UTIL_H_20241226__

#include <string>
using namespace std;

enum MNL_TEXTSTYLE
{
	TXC_NORMAL = 0x00,
	TXC_BOLD,
	TXC_UNDERLINE = 0x04,
	TXC_BLINKING,
	TXC_REVERSED = 0x07,
	TXC_CONCEALED,
};

enum MNL_FOREGROUNDCOLOR
{
	FGC_BLACK = 0x1E,
	FGC_RED,
	FGC_GREEN,
	FGC_ORANGE,
	FGC_BLUE,
	FGC_PURPLE,
	FGC_CYAN,
	FGC_GRAY,				// or Grey (영국)
	FGC_DARKGRAY = 0x5A,	// or DarkGrey (영국)
	FGC_LIGHTRED,
	FGC_LIGHTGREEN,
	FGC_YELLOW,
	FGC_LIGHTBLUE,
	FGC_LIGHTPURPLE,
	FGC_TURQUOISE,
};

enum MNL_BACKGROUNDCOLOR
{
	BGC_BLACK = 0x28,
	BGC_RED,
	BGC_GREEN,
	BGC_ORANGE,
	BGC_BLUE,
	BGC_PURPLE,
	BGC_CYAN,
	BGC_GRAY,				// or Grey (영국)
	BGC_DARKGRAY = 0x64,	// or DarkGrey (영국)
	BGC_LIGHTRED,
	BGC_LIGHTGREEN,
	BGC_YELLOW,
	BGC_LIGHTBLUE,
	BGC_LIGHTPURPLE,
	BGC_TURQUOISE,
};

#define CASE_TEXT_STYLE \
  case TXC_NORMAL: case TXC_BOLD: case TXC_UNDERLINE: case TXC_BLINKING: case TXC_REVERSED: case TXC_CONCEALED
#define CASE_FOREGROUND_COLOR \
  case FGC_BLACK: case FGC_RED: case FGC_GREEN: case FGC_ORANGE: case FGC_BLUE: case FGC_PURPLE: case FGC_CYAN: case FGC_GRAY: \
  case FGC_DARKGRAY: case FGC_LIGHTRED: case FGC_LIGHTGREEN: case FGC_YELLOW: case FGC_LIGHTBLUE: case FGC_LIGHTPURPLE: case FGC_TURQUOISE
#define CASE_BACKROUND_COLOR \
  case BGC_BLACK: case BGC_RED: case BGC_GREEN: case BGC_ORANGE: case BGC_BLUE: case BGC_PURPLE: case BGC_CYAN: case BGC_GRAY: \
  case BGC_DARKGRAY: case BGC_LIGHTRED: case BGC_LIGHTGREEN: case BGC_YELLOW: case BGC_LIGHTBLUE: case BGC_LIGHTPURPLE: case BGC_TURQUOISE


extern CString g_strVersion;

extern int WideCharToChar(WCHAR* szSource, char* szTarget, int nLen, UINT nCodePage = CP_ACP);
extern int CharToWideChar(char* szSource, WCHAR* szTarget, int nLen, UINT nCodePage = CP_ACP);

// inline
extern inline void PrintOut(const wchar_t* fmt, ...);
extern inline void PrintOutA(const char* fmt, ...);
extern inline const char* string_to_hex(const char* str, char* hex, size_t maxlen);

class CUtil
{
public:
    CUtil();
    ~CUtil();

    static char* RemoveLineFeed(char* pszData);
    static TCHAR* RemoveLineFeed(TCHAR* pszData);
    static BOOL IsAnsiCommand(char* pszData);
    static COLORREF GetAnsiColor(DWORD dwCode);
    static string GetAppRootPathA();
    static TCHAR* GetAppRootPath();
    static CString GetVersion(TCHAR *pszCode = NULL);       // CODE: korean = 041204b0, english = 040904b0, ...

    static bool ExistsLastPython(DWORD dwPid);
    static TCHAR* GetProcName(DWORD dwPid);
    static void TerminateProcessByPID(DWORD dwPid);
    static HWND FindTopWindow(DWORD dwPid);
};

#endif	// #ifndef __MONOS_UTIL_H_20241226__