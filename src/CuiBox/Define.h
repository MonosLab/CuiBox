// Version histroy
// 0.1.0.1   : first version.
// 0.1.1.2   : Only graphics cards using NVIDIA chipsets display CPU, GPU, RAM, and VRAM usage percentages, but non-NVIDIA chipsets such as Intel and AMD display only CPU and RAM usage percentages.
//             * NVIDIA 칩셋 -> CPU, GPU, RAM, VRAM 사용률 표시
//             * 그 외(Intel, AMD 등) -> CPU, RAM 사용률만 표시
// 0.1.2.4   : General-purpose functions were made into static libraries.

#ifndef __MONO_DEFINE_H_20240508__
#define __MONO_DEFINE_H_20240508__

#define MODE_DEBUG						// Enable "PrintOut"
#define PEEK_NAMED_PIPE					// Use named pipes
#define IGNORE_HTTPS_SECURITY			// Ignore SSL security authentication failure
#define VER_0_2_7						// If comfyui version is 0.2.7 or later

#define PX_CALIBRATE					-7
#define PX_TASKBAR_HEIGHT				32				// default or medium taskbar height is 48 pixels,
														// the large height is 72 pixels, and the small height was 32 pixels.

#define TTB_ICON_MARGIN					8
#define TTB_BORDER						5
#define TTB_BORDER_TOP_MARGIN			6				// Top border area of Windows theme
#define TTB_TEXT_TOP_POS				12
#define TTB_TF_PLUS_BORDER				3
#define TTB_TF_MINUS_BORDER				-3
#define TTB_HEIGHT						39
#define TTB_ICONX						40
#define TTB_ICONY						40
#define TTB_BTN_X						30
#define TTB_BTN_Y						30
#define TTB_USAGE_INFO					210

// Font size
#define MN_FONT_DEFAULTSIZE				12
#define MN_FONT_TITLE_SIZE				14
#define MN_FONT_BODY_SIZE				15

// Sizes
#define SIZE_MEGA						1048576.0f		// Mega bytes
#define SIZE_GIGA						1073741824.0f	// Giga bytes
#define SIZE_EXCLUDE					8
#define SIZE_BROWSER					404				// Browser
#define SIZE_CONSOLE					0				// Console
#define SIZE_MAX_RBROWSER				404				// Maximum
#define SIZE_MIN_RBROWSER				51				// Minimum
#define SIZE_RCE_CONSOLEW				300				// RCE width
#define SIZE_RCE_CONSOLEH				300				// RCE height
#define SIZE_SPLITTER_BORDER			1
#define SIZE_SPLITTER_GAP				2				// SIZE_SPLITTER_BORDER * 2
#define SIZE_CONSOLE_DATA				4097
#define MAX_PATH2						512
#define MAINWINDOW_WIDTH				900
#define MAINWINDOW_HEIGHT				640
#define WND_MIN_X						720				// ... + Sadow(20px)
#define WND_MIN_Y						600
#define CHILD_MIN_X						0
#define CHILD_MIN_Y						0

// Controls
#define IDC_CTL_BROWSER					9999
#define IDC_CTL_SPLITTER				10000
#define IDC_CTL_RICHEDIT				10001
#define IDC_STT_USAGEINFO				10002
#define IDC_STT_LWINDOW					10003
#define IDC_STT_RWINDOW					10004

// ANSI
#define ANSI_ESC						0x1B

#define APP_CLASS						_T("MonoslabCuiBoxClass")
#define APP_MUTEX						_T("Global\\Monoslab_CuiBoxClass_1_0")
#define INET_AGENT						_T("HttpApp/1.1")

// Registry
#define REGPATH_CUIBOX					_T("Software\\Monoslab\\CuiBox")
#define REGPATH_AUTORUN					_T("Software\\Microsoft\\Windows\\CurrentVersion\\Run")
#define REGPATH_POS						_T("pos")
#define REGKEY_POS_MAIN_L				_T("pl")
#define REGKEY_POS_MAIN_T				_T("pt")
#define REGKEY_POS_MAIN_R				_T("pr")
#define REGKEY_POS_MAIN_B				_T("pb")
#define REGKEY_POS_DIRECT				_T("dir")
#define REGKEY_POS_SHOWWINDOW			_T("rce_show")
#define REGKEY_POS_RCE_W				_T("rce_w")
#define REGKEY_POS_RCE_H				_T("rce_h")
#define REGKEY_PID						_T("last_pid")

#define LOGO_IMAGE						_T("images\\wbicon.png")
#define PATH_BTN_CLOSE					_T("images\\btn_close.png")
#define PATH_BTN_MAX					_T("images\\btn_max.png")
#define PATH_BTN_MIN					_T("images\\btn_min.png")
#define PATH_BTN_SHOW_CONSOLE			_T("images\\btn_show_con.png")
#define PATH_BTN_POS_CONSOLE			_T("images\\btn_pos_con.png")
#define FLJ_CUIBOX						_T("CuiBox.json")
#define FLJ_CUIBOXA						"CuiBox.json"

#define APP_NAME						_T("CuiBox")
#define APP_FOR_COMFYUI					_T("for ComfyUI")
#define APP_VER_FILE					_T("CuiBox.ver")
#define APP_EXE_FILE					_T("CuiBox.exe")
#define APP_UPDATE_FILE					_T("CuiBoxUpdate.exe")
#define APP_UPDATENEW_FILE				_T("CuiBoxUpdateNew.exe")
#define TU_CUI_BROWSER					_T("CuiBrowser")
#define TU_URL							_T("http://localhost:8188")
										// CallComfyUI=http://127.0.0.1:8188
#define TU_CALL_COMFYUI					"CallComfyUI="
#define SIZE_CALL_COMFYUI				12

#define URL_GITHUB						_T("https://github.com")
#define PATH_VER						_T("/MonosLab/CuiBox/raw/master/CuiBox.ver")
#define PATH_OUTPUT_FOLDER				_T("ComfyUI\\output\\")

// Get resource file information
#define RES_ENGLISH						_T("040904b0")
#define RES_KOREAN						_T("041204b0")

// TitleBar
const BLENDFUNCTION TTB_ICON_BF			= { AC_SRC_OVER, 0x00, 0xFF, AC_SRC_ALPHA };
#define TTB_BRD_BRUSH					RGB(255, 255, 255)
#define TTB_BGC_BRUSH					RGB(255, 255, 255)
#define TTB_BGC_PEN						RGB(220, 220, 220)

// RichEdit Color
#define RCE_BG_COLOR					RGB(34, 34, 34)
#define RCE_FG_COLOR					RGB(230, 230, 230)

// Line Color
#define TTB_LINE_PEN					RGB(180, 180, 180)

// Macro
#ifndef SAFE_DELETE 
#define SAFE_DELETE(p)				{ if(p) { delete (p); (p) = NULL; } } 
#endif // SAFE_DELETE 
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)		{ if(p) { delete[] (p); (p) = NULL; } } 
#endif // SAFE_DELETE_ARRAY
#ifndef SAFE_FREE 
#define SAFE_FREE(p)				{ if(p) { free(p); (p) = NULL; } } 
#endif // SAFE_FREE 
#ifndef SAFE_CLOSE_HANDLE
#define SAFE_CLOSE_HANDLE(h)        { if(h != NULL && h != INVALID_HANDLE_VALUE) { CloseHandle(h); h = NULL; } }
#endif // SAFE_CLOSE_HANDLE
#define UPDATE_USAGE(x, y)			{ if(y != 0 && x != y) x = y; }
#define DEBUG_VIEW(b, t)			if(b) { t; }

// Static
static LPSTR COMFYUI_MAINA = "ComfyUI\\main.py";
static LPSTR CUIBOX_MAINA = "ComfyUI\\cuibox_main.py";
static LPTSTR CUIBOX_MAINW = _T("ComfyUI\\cuibox_main.py");
static LPTSTR COMFYUI = _T(".\\python_embeded\\python.exe");
static LPTSTR PARAMS[2] = {
	_T("-s .\\ComfyUI\\cuibox_main.py --windows-standalone-build"),
	_T("-s .\\ComfyUI\\cuibox_main.py --cpu --windows-standalone-build")
};

// const variants
const std::string find_map[] = {
	"import webbrowser",
	"if os.name == 'nt' and address == '0.0.0.0':",
	"address = '127.0.0.1'",
	"if ':' in address:",
	"address = \"[{}]\"",
	"webbrowser.open(f\"{scheme}://{address}:{port}\")",
};

const std::string replace_map[] = {
	"address = 'localhost'",
	"print(f\"CallComfyUI={scheme}://{address}:{port}\", flush=True)",
};


// enum
enum GRAPHICS
{
	G_NVIDIA = 1,			// NVIDIA Geforce
	G_AMD,					// AMD Radeon
	G_INTEL,				// Intel Arc (CPU Chips)
	G_UNKNOWN,
};

enum OPT_RUN
{
	NVIDIA = 0,
	CPU = 1,
	RESTART,
};

enum CALLER
{
	CALL_MAIN = 0,
	CALL_SPLITTER,
	CALL_SHOW_HIDE,
};

enum _WEB_PROTOCOL
{
	HTTP = 0,
	HTTPS,
};

enum _LANGUAGE
{
	LANG_NULL = 0,
	KO,
	EN,
	JA,
	ZH,
	VI,
};

enum _BINARY_LANG_CODE
{
	BL_NONE = 0,			// 0000 0000
	BL_KO = 1,				// 0000 0001
	BL_EN = 2,				// 0000 0010
	BL_JA = 4,				// 0000 0100
	BL_ZH = 8,				// 0000 1000
	BL_VI = 16,				// 0001 0000
};

enum class TYPE_DIRECTION
{
	TD_VERT = 1,
	TD_HORZ,
};

enum TYPE_RESULT
{
	TR_NORMAL = 1,
	TR_EMPHASIS,
	TR_ERROR,
};

enum _USER_MESSAGE
{
	UM_TRAYICON = WM_USER + 1974,
	UM_MUTEX_OPEN,
	UM_EXEC_NAVIGATE,
	UM_CHANGE_SPLITTER,
	UM_RUN_ASYNC_CALLBACK,
};

enum _USER_MENU
{
	UDM_OPEN = 9000,
	UDM_OPEN_OUTPUT,
	UDM_CLOSE,
	UDM_STARTUP,
	UDM_RESET,
	UDM_APPLY_CHANGES,
	UDM_RUN_CPU,
	UDM_RUN_GPU,
	UDM_SET_KO,
	UDM_SET_EN,
	UDM_SET_JA,
	UDM_SET_ZH,
	UDM_SET_VI,
};

enum _ERROR_CODE
{
	ERR_SUCCESS = 0,
	ERR_NVML_INIT,
	ERR_NVML_COUNT,
	ERR_PDH_OPEN,
	ERR_PDH_COUNTER,
	ERR_PDH_QUERY,
	ERR_PDH_FORMAT,
};

typedef struct _IMAGEDATA
{
	HBITMAP hBitmap;
	short width;
	short height;
} IMAGEDATA;

struct VERSION_ITEM
{
	string filename;
	string dir;
	int version;
};

struct CONSOLE_DATA
{
	UINT nType;
	DWORD dwSize;
	char szData[SIZE_CONSOLE_DATA];		// 4096 + 1
};

struct TEXT_INFO_DATA
{
	DWORD dwTextStyle;
	DWORD dwForeColor;
	DWORD dwBackColor;
	wstring data;
};

struct GPUInfo
{
	UINT ram;				// AdapterRAM
	UINT hres;				// CurrentHorizontalResolution
	UINT vres;				// CurrentVerticalResolution
	wstring ramtype;		// Ram type
	wstring name;			// Name
	wstring descript;		// Description
	wstring vprocessor;		// VideoProcessor
};

struct _RAM_DATA
{
	DWORD dwRamType;
	TCHAR szName[32];
};

struct _PIPE_INFO
{
	CWnd* pWnd;
	HANDLE hReadPipe;
	bool bError;
};

struct _THREAD_INFO
{
	bool is_exit;
	bool exe_gpu;
	bool exe_cpu;
};

static _RAM_DATA RAM_INFO[] = {
	{0, _T("Error")},
	{1, _T("Other")},
	{2, _T("Unknown")},
	{3, _T("VRAM")},
	{4, _T("DRAM")},
	{5, _T("SRAM")},
	{6, _T("WRAM")},
	{7, _T("EDO RAM")},
	{8, _T("Burst Synchronous DRAM")},
	{9, _T("Pipelined Burst SRAM")},
	{10, _T("CDRAM")},
	{11, _T("3DRAM")},
	{12, _T("SDRAM")},
	{13, _T("SGRAM")},
};

#endif	// #ifndef __MONO_DEFINE_H_20240508__