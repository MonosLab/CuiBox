#include "pch.h"
#include "ResourceEx.h"

CResourceEx* CResourceEx::m_pInstance = nullptr;

CResourceEx::CResourceEx()
	: m_nLangType(LANG_NULL)
{
	memset(m_szLang, 0x00, sizeof(m_szLang));
	_stprintf_s(m_szLang, _T("ko"));
}

CResourceEx::~CResourceEx()
{
}

CString CResourceEx::LoadStringEx(UINT32 nType)
{
	CString strResult(_T(""));

	switch (nType)
	{
		case IDS_FONT_DEFAULTFACE:
			if		(m_nLangType == KO)	strResult = _T("맑은 고딕");
			else if (m_nLangType == EN)	strResult = _T("MS Shell Dlg");
			else if (m_nLangType == JA)	strResult = _T("Arial");
			else if (m_nLangType == ZH)	strResult = _T("Arial");
			else if (m_nLangType == VI)	strResult = _T("Arial");
			else strResult = _T("MS Shell Dlg");
			break;
		case IDS_CUIBOX:
			if		(m_nLangType == KO)	strResult = _T("CUIBOX");
			else if (m_nLangType == EN)	strResult = _T("CUIBOX");
			else if (m_nLangType == JA)	strResult = _T("CUIBOX");
			else if (m_nLangType == ZH)	strResult = _T("CUIBOX");
			else if (m_nLangType == VI)	strResult = _T("CUIBOX");
			else strResult = _T("CUIBOX");
			break;
		case IDS_STARTUP:
			if (m_nLangType == KO)	strResult = _T("윈도우 시작시 자동 실행");
			else if (m_nLangType == EN)	strResult = _T("Run program automatically when windows starts");
			else if (m_nLangType == JA)	strResult = _T("ウィンドウ開始時にメッセンジャーを自動実行する");
			else if (m_nLangType == ZH)	strResult = _T("启动Windows时自动启动即时通讯");
			else if (m_nLangType == VI)	strResult = _T("Tự động khởi chạy khi cửa sổ bắt đầu");
			else strResult = _T("Run program automatically when windows starts");
			break;
		case IDS_CLOSE:
			if (m_nLangType == KO)	strResult = _T("종료");
			else if (m_nLangType == EN)	strResult = _T("Close");
			else if (m_nLangType == JA)	strResult = _T("終了");
			else if (m_nLangType == ZH)	strResult = _T("打开");
			else if (m_nLangType == VI)	strResult = _T("Đóng");
			else strResult = _T("Close");
			break;
		case IDS_OPEN:
			if (m_nLangType == KO)	strResult = _T("열기");
			else if (m_nLangType == EN)	strResult = _T("Open");
			else if (m_nLangType == JA)	strResult = _T("開く");
			else if (m_nLangType == ZH)	strResult = _T("结束");
			else if (m_nLangType == VI)	strResult = _T("Kết nối");
			else strResult = _T("Open");
			break;
		case IDS_OPEN_OUTPUT:
			if (m_nLangType == KO)	strResult = _T("저장 폴더 열기");
			else if (m_nLangType == EN)	strResult = _T("Open save folder");
			else if (m_nLangType == JA)	strResult = _T("保存フォルダーを開く");
			else if (m_nLangType == ZH)	strResult = _T("打开保存文件夹");
			else if (m_nLangType == VI)	strResult = _T("Mở thư mục lưu");
			else strResult = _T("Open save folder");
			break;
		case IDS_RESET:
			if (m_nLangType == KO)	strResult = _T("초기화");
			else if (m_nLangType == EN)	strResult = _T("Reset");
			else if (m_nLangType == JA)	strResult = _T("初期化");
			else if (m_nLangType == ZH)	strResult = _T("初始化");
			else if (m_nLangType == VI)	strResult = _T("Cài lại");
			else strResult = _T("Reset");
			break;
		case IDS_APPLY_CHANGES:
			if (m_nLangType == KO)	strResult = _T("변경된 설정 적용");
			else if (m_nLangType == EN)	strResult = _T("Apply changed settings");
			else if (m_nLangType == JA)	strResult = _T("変更された設定を適用します");
			else if (m_nLangType == ZH)	strResult = _T("应用更改后的设置");
			else if (m_nLangType == VI)	strResult = _T("Áp dụng cài đặt đã thay đổi");
			else strResult = _T("Apply changes");
			break;
		case IDS_RUN_MODE :
			if (m_nLangType == KO)	strResult = _T("실행 모드");
			else if (m_nLangType == EN)	strResult = _T("Run mode");
			else if (m_nLangType == JA)	strResult = _T("実行モード");
			else if (m_nLangType == ZH)	strResult = _T("运行模式");
			else if (m_nLangType == VI)	strResult = _T("Chế độ chạy");
			else strResult = _T("Run mode");
			break;
		case IDS_RESTART_PROGRAM:
			if (m_nLangType == KO)	strResult = _T("변경된 설정을 적용하기 위해 프로그램을 재시작 합니다.");
			else if (m_nLangType == EN)	strResult = _T("The program will restart to apply the changed settings.");
			else if (m_nLangType == JA)	strResult = _T("決定事項または未決定事項が少なくとも1つ必要です。");
			else if (m_nLangType == ZH)	strResult = _T("为应用更改后的设置，重新启动程序。");
			else if (m_nLangType == VI)	strResult = _T("Khởi động lại chương trình để áp dụng các thay đổi cài đặt.");
			else strResult = _T("The program will restart to apply the changed settings.");
			break;
		case IDS_SELECT_LANGUAGE:
			if (m_nLangType == KO)	strResult = _T("언어 선택 (Language)");
			else if (m_nLangType == EN)	strResult = _T("Select language");
			else if (m_nLangType == JA)	strResult = _T("言語選択 (Language)");
			else if (m_nLangType == ZH)	strResult = _T("选择语言 (Language)");
			else if (m_nLangType == VI)	strResult = _T("Chọn ngôn ngữ (Language)");
			else strResult = _T("Select language");
			break;
		case IDS_KOREAN:
			if (m_nLangType == KO)	strResult = _T("한국어");
			else if (m_nLangType == EN)	strResult = _T("Korean");
			else if (m_nLangType == JA)	strResult = _T("韓国語");
			else if (m_nLangType == ZH)	strResult = _T("韩语");
			else if (m_nLangType == VI)	strResult = _T("Tiếng Hàn");
			else strResult = _T("Korean");
			break;
		case IDS_ENGLISH:
			if (m_nLangType == KO)	strResult = _T("영어");
			else if (m_nLangType == EN)	strResult = _T("English");
			else if (m_nLangType == JA)	strResult = _T("英語");
			else if (m_nLangType == ZH)	strResult = _T("英語");
			else if (m_nLangType == VI)	strResult = _T("Tiếng Anh");
			else strResult = _T("English");
			break;
		case IDS_CHINESE:
			if (m_nLangType == KO)	strResult = _T("중국어");
			else if (m_nLangType == EN)	strResult = _T("Chinese");
			else if (m_nLangType == JA)	strResult = _T("中国語");
			else if (m_nLangType == ZH)	strResult = _T("汉语");
			else if (m_nLangType == VI)	strResult = _T("Tiếng trung");
			else strResult = _T("Chinese");
			break;
		case IDS_JAPANESE:
			if (m_nLangType == KO)	strResult = _T("일본어");
			else if (m_nLangType == EN)	strResult = _T("Japanese");
			else if (m_nLangType == JA)	strResult = _T("日本語");
			else if (m_nLangType == ZH)	strResult = _T("日语");
			else if (m_nLangType == VI)	strResult = _T("Tiếng Nhật");
			else strResult = _T("Japanese");
			break;
		case IDS_VIETNAMESE:
			if (m_nLangType == KO)	strResult = _T("베트남어");
			else if (m_nLangType == EN)	strResult = _T("Vietnamese");
			else if (m_nLangType == JA)	strResult = _T("ベトナム語");
			else if (m_nLangType == ZH)	strResult = _T("越南语");
			else if (m_nLangType == VI)	strResult = _T("Tiếng Việt");
			else strResult = _T("Vietnamese");
			break;
		case IDS_INSTALL_NVIDIA_DRV:
			if (m_nLangType == KO)	strResult = _T("최근 NVIDIA 드라이버를 설치한 후 프로그램을 다시 시작하세요.");
			else if (m_nLangType == EN)	strResult = _T("After installing the latest NVIDIA driver, please restart the program.");
			else if (m_nLangType == JA)	strResult = _T("最新の NVIDIA ドライバーをインストールした後、プログラムを再起動してください。");
			else if (m_nLangType == ZH)	strResult = _T("安装最新的 NVIDIA 驱动程序后，请重新启动程序。");
			else if (m_nLangType == VI)	strResult = _T("Sau khi cài đặt trình điều khiển NVIDIA mới nhất, vui lòng khởi động lại chương trình.");
			else strResult = _T("After installing the latest NVIDIA driver, please restart the program.");
			break;
	}
	
	return strResult;
}

string CResourceEx::LoadStringExA(UINT32 nType)
{
	CString str = LoadStringEx(nType);
	return string(CT2CA(str));
}