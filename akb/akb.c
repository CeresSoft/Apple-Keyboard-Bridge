/**
 * Apple Keyboard Bridge https://github.com/andantissimo/Apple-Keyboard-Bridge
 */
#include "../common/akb.h"
#include "winapi.h"

#include "resource.h"

 //2019.07.15:SUGIHARA:ADD >>>>>
static CRITICAL_SECTION _DebugCriticalSection;
static HANDLE _DebugFileHandle = INVALID_HANDLE_VALUE;
//2019.07.15:SUGIHARA:ADD <<<<<

//2019.07.15:SUGIHARA:ADD >>>>>
static void DEBUG(LPCTSTR format, ...)
{
#if _DEBUG
	va_list args;
	va_start(args, format);

	int n = _vsctprintf(format, args);
	if (n <= 0)
	{
		//出力文字列なし
		//何もしない(va_endするためreturnしてはいけない)
	}
	else
	{
		//出力文字列あり
		int len = n + 1;
		int size = sizeof(TCHAR) * len;
		TCHAR* buff = (TCHAR*)malloc(size);
		if (buff == NULL)
		{
			//バッファ確保失敗
			//何もしない(va_endするためreturnしてはいけない)
		}
		else
		{
			//バッファ確保成功
			int ret = _vstprintf_s(buff, len, format, args);
			if (ret <= 0)
			{
				//文字列化失敗
				//何もしない(freeするためreturnしてはいけない)
			}
			else
			{
				//文字列化成功
				OutputDebugString(buff);
				//ファイルに出力
				EnterCriticalSection(&_DebugCriticalSection);
				HANDLE handle = _DebugFileHandle;
				if (handle != INVALID_HANDLE_VALUE)
				{
					//最後のNUL文字を書き宇出さないようにバイト数を調整
					DWORD writeByte = _tcslen(buff) * sizeof(TCHAR);
					DWORD wroteByte = 0;
					WriteFile(handle, buff, writeByte, &wroteByte, NULL);
				}
				LeaveCriticalSection(&_DebugCriticalSection);
			}

			//バッファ開放
			free(buff);
		}
	}

	va_end(args);
#endif
}
//2019.07.15:SUGIHARA:ADD <<<<<

BYTE addsb(BYTE x, int a)
{
	/* Add with Saturation in BYTE */
	int r = (int)x + a;
	if (r < 0x00) r = 0x00;
	if (r > 0xFF) r = 0xFF;
	return (BYTE)r;
}

const CLSID CLSID_WbemLocator = { 0x4590F811, 0x1D3A, 0x11D0, { 0x89, 0x1F, 0x00, 0xAA, 0x00, 0x4B, 0x2E, 0x24 } };
const IID IID_IWbemLocator = { 0xDC12A687, 0x737F, 0x11CF, { 0x88, 0x4D, 0x00, 0xAA, 0x00, 0x4B, 0x2E, 0x24 } };

enum
{
	VID_APPLE = 0x05AC,
};
//2019.07.15:SUGIHARA:CHANGE >>>>>
//const static WORD PID_APPLE_KEYBOARD[] =
//{
//	0x022C, /* Apple Wireless Keyboard US  2007 */
//	0x022E, /* Apple Wireless Keyboard JIS 2007 */
//	0x0239, /* Apple Wireless Keyboard US  2009 */
//	0x023B, /* Apple Wireless Keyboard JIS 2009 */
//	0x0255, /* Apple Wireless Keyboard US  2011 */
//	0x0257, /* Apple Wireless Keyboard JIS 2011 */
//	0x0265, /* Apple Magic Keyboard US  2015    */
//	0x0267, /* Apple Magic Keyboard JIS 2015    */
//	//2019.07.15:SUGIHARA:ADD >>>>>
//	0x026C, /* Apple Magic Keyboard US  2018    */
//	//2019.07.15:SUGIHARA:ADD <<<<<
//};
//----------
//akbcf.exeに引数で渡すので文字列でレイアウトを定義
typedef struct {
	WORD PID;
	LPCTSTR Name;
	LPCTSTR KeyBoardType;
	LPCTSTR KeyBoardLang;
	LPCTSTR KeyBoardSize;
} KEYBOARD_LAYOUT;
const static KEYBOARD_LAYOUT PID_APPLE_KEYBOARD[] =
{
	{ 0x022C, _T("Apple Wireless Keyboard US 2007"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_US, LAYOUT_SIZE_FULL },
	{ 0x022E, _T("Apple Wireless Keyboard JIS 2007"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_JP, LAYOUT_SIZE_FULL },
	{ 0x0239, _T("Apple Wireless Keyboard US 2009"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_US, LAYOUT_SIZE_FULL },
	{ 0x023B, _T("Apple Wireless Keyboard JIS 2009"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_JP, LAYOUT_SIZE_FULL },
	{ 0x0255, _T("Apple Wireless Keyboard US  2011"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_US, LAYOUT_SIZE_FULL },
	{ 0x0257, _T("Apple Wireless Keyboard JIS 2011"), LAYOUT_TYPE_WIRELESS, LAYOUT_LANG_JP, LAYOUT_SIZE_FULL },
	{ 0x0265, _T("Apple Magic Keyboard US  2015"), LAYOUT_TYPE_MAGIC, LAYOUT_LANG_US, LAYOUT_SIZE_FULL },
	{ 0x0267, _T("Apple Magic Keyboard JIS 2015"), LAYOUT_TYPE_MAGIC, LAYOUT_LANG_JP, LAYOUT_SIZE_FULL },
	{ 0x026C, _T("Apple Magic Keyboard US  2018 No Numpad"), LAYOUT_TYPE_MAGIC, LAYOUT_LANG_US, LAYOUT_SIZE_MINI },
	{ 0x0279, _T("Mac Book 12 2017 US"), LAYOUT_TYPE_BOOTCAMP, LAYOUT_LANG_US, LAYOUT_SIZE_MINI },
};
//2019.07.15:SUGIHARA:CHANGE <<<<<
//2019.07.16:SUGIHARA:MOVE >>>>>
static struct Status
{
	BOOL Fn;
	//2019.07.15:SUGIHARA:ADD >>>>>
	BOOL IsBootCamp;
	const KEYBOARD_LAYOUT* Layout;
	//2019.07.15:SUGIHARA:ADD <<<<<

} Status;
//2019.07.16:SUGIHARA:MOVE <<<<<

//2019.07.15:SUGIHARA:ADD >>>>>
static BOOL IsBootCamp()
{
	const KEYBOARD_LAYOUT* pLayout = Status.Layout;
	if (pLayout == NULL)
	{
		//レイアウト不明の場合はBOOTCAMPではないにする
		return FALSE;
	}
	LPCTSTR pType = pLayout->KeyBoardType;
	if (pType == NULL)
	{
		//タイプ不明の場合もBOOTCAMPではないにする
		return FALSE;
	}
	int nComp = _tcscmp(pType, LAYOUT_TYPE_BOOTCAMP);
	if (nComp != 0)
	{
		//BOOTCAMP以外の場合
		return FALSE;
	}
	//ここまで来たらBOOTCAMP環境
	return TRUE;
}
//2019.07.15:SUGIHARA:ADD <<<<<

static BOOL IsSupportedDevice(LPCTSTR path, WORD vid, WORD pid, WORD version)
{
	//2019.07.15:SUGIHARA:ADD >>>>>
	//if (vid == VID_APPLE) {
	//	UINT i;
	//	for (i = 0; i < ARRAYSIZE(PID_APPLE_KEYBOARD); i++) {
	//		if (pid == PID_APPLE_KEYBOARD[i])
	//			return TRUE;
	//	}
	//}
	//return FALSE;
	//----------
	DEBUG(_T("Layout path='%s', vid=%x pid=%x, version=%x\n"), path, vid, pid, version);

	//Apple Vender ID判定
	if (vid != VID_APPLE)
	{
		//異なる場合は終了
		return FALSE;
	}

	//Apple Keyboard
	{
		UINT i;
		UINT m = ARRAYSIZE(PID_APPLE_KEYBOARD);
		for (i = 0; i < m; i++)
		{
			const KEYBOARD_LAYOUT* pLayout = &PID_APPLE_KEYBOARD[i];
			if (pid == pLayout->PID)
			{
				//レイアウト保存
				Status.Layout = pLayout;
				Status.IsBootCamp = IsBootCamp();
				DEBUG(_T("Layout=%s\n"), pLayout->Name);
				return TRUE;
			}
		}
	}
	//ここまで来たら該当なし
	return FALSE;
	//2019.07.15:SUGIHARA:ADD <<<<<
}

const struct AppIcon
{
	LPCTSTR File;
	struct Index
	{
		WORD XP, Vista;
	} Index;
} AppIcon =
{
	TEXT("main.cpl"), { 7, 5 }
};

BOOL IsVistaOrGreater(void)
{
	OSVERSIONINFOEX osvi;
	DWORDLONG condition = 0;
	ZeroMemory(&osvi, sizeof osvi);
	osvi.dwOSVersionInfoSize = sizeof osvi;
	osvi.dwMajorVersion = 6;
	VER_SET_CONDITION(condition, VER_MAJORVERSION, VER_GREATER_EQUAL);
	return VerifyVersionInfo(&osvi, VER_MAJORVERSION, condition);
}

enum
{
	MY_EXTRA_INFO = 0x37564,
};
void SendKey(UINT vkCode)
{
	INPUT inputs[2];
	ZeroMemory(inputs, sizeof inputs);
	inputs[0].type           = INPUT_KEYBOARD;
	inputs[0].ki.wVk         = (WORD)vkCode;
	inputs[0].ki.dwExtraInfo = MY_EXTRA_INFO;

	inputs[1].type           = INPUT_KEYBOARD;
	inputs[1].ki.wVk         = (WORD)vkCode;
	inputs[1].ki.dwFlags     = KEYEVENTF_KEYUP;
	inputs[1].ki.dwExtraInfo = MY_EXTRA_INFO;
	SendInput(ARRAYSIZE(inputs), inputs, sizeof*inputs);
}

HRESULT WmiGetNamespace(BSTR strNamespace, IWbemServices **ppNamespace)
{
	HRESULT hr;
	IWbemLocator *pLocator;
	hr = CoCreateInstance(&CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, &IID_IWbemLocator, (LPVOID *)&pLocator);
	if (hr != S_OK)
		return hr;
	hr = pLocator->lpVtbl->ConnectServer(pLocator, strNamespace, NULL, NULL, NULL, 0, NULL, NULL, ppNamespace);
	pLocator->lpVtbl->Release(pLocator);
	return hr;
}

HRESULT WmiQueryObject(IWbemServices *pNamespace, const BSTR strQuery, IWbemClassObject **ppObject)
{
	HRESULT hr;
	IEnumWbemClassObject *pEnum;
	ULONG uReturned;
	hr = pNamespace->lpVtbl->ExecQuery(pNamespace, L"WQL", strQuery, WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnum);
	if (hr != S_OK)
		return hr;
	hr = pEnum->lpVtbl->Next(pEnum, WBEM_INFINITE, 1, ppObject, &uReturned);
	pEnum->lpVtbl->Release(pEnum);
	return hr;
}

HRESULT WmiCreateParams(IWbemServices *pNamespace, const BSTR strClass, const BSTR strMethod, IWbemClassObject **ppParams)
{
	HRESULT hr;
	IWbemClassObject *pClass, *pMethod;
	hr = pNamespace->lpVtbl->GetObjectW(pNamespace, strClass, 0, NULL, &pClass, NULL);
	if (hr != S_OK)
		return hr;
	hr = pClass->lpVtbl->GetMethod(pClass, strMethod, 0, &pMethod, NULL);
	pClass->lpVtbl->Release(pClass);
	if (hr != S_OK)
		return hr;
	hr = pMethod->lpVtbl->SpawnInstance(pMethod, 0, ppParams);
	pMethod->lpVtbl->Release(pMethod);
	return hr;
}

HRESULT WmiExecMethod(IWbemServices *pNamespace, IWbemClassObject *pObject, const BSTR strMethod, IWbemClassObject *pParams)
{
	HRESULT hr;
	VARIANT varObjectPath;
	hr = pObject->lpVtbl->Get(pObject, L"__PATH", 0, &varObjectPath, NULL, NULL);
	if (hr != S_OK)
		return hr;
	hr = pNamespace->lpVtbl->ExecMethod(pNamespace, varObjectPath.bstrVal, strMethod, 0, NULL, pParams, NULL, NULL);
	VariantClear(&varObjectPath);
	return hr;
}

void Power(void)
{
	/* do nothing */
}

void Eject(void)
{
	MCI_OPEN_PARMS mop;
	ZeroMemory(&mop, sizeof mop);
	mop.lpstrDeviceType = (LPCTSTR)MCI_DEVTYPE_CD_AUDIO;
	WinAPI.MCI.SendCommand(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&mop);
	WinAPI.MCI.SendCommand(mop.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, 0);
	WinAPI.MCI.SendCommand(mop.wDeviceID, MCI_CLOSE, 0, 0);
}

void Flip3D(void)
{
	INPUT inputs[4];
	ZeroMemory(inputs, sizeof inputs);
	inputs[0].type           = INPUT_KEYBOARD;
	inputs[0].ki.wVk         = VK_LWIN;
	inputs[0].ki.dwExtraInfo = MY_EXTRA_INFO;

	inputs[1].type           = INPUT_KEYBOARD;
	inputs[1].ki.wVk         = VK_TAB;
	inputs[1].ki.dwExtraInfo = MY_EXTRA_INFO;

	inputs[2].type           = INPUT_KEYBOARD;
	inputs[2].ki.wVk         = VK_TAB;
	inputs[2].ki.dwFlags     = KEYEVENTF_KEYUP;
	inputs[2].ki.dwExtraInfo = MY_EXTRA_INFO;

	inputs[3].type           = INPUT_KEYBOARD;
	inputs[3].ki.wVk         = VK_LWIN;
	inputs[3].ki.dwFlags     = KEYEVENTF_KEYUP;
	inputs[3].ki.dwExtraInfo = MY_EXTRA_INFO;
	SendInput(ARRAYSIZE(inputs), inputs, sizeof*inputs);
}

void Bright(int delta)
{
	IWbemServices *pRootWmi;
	IWbemClassObject *pMonitorBrightness;
	IWbemClassObject *pMonitorBrightnessMethods, *pParams;
	VARIANT varCurrentBrightness, varTimeout, varBrightness;
	if (WmiGetNamespace(L"ROOT\\WMI", &pRootWmi) == S_OK) {
		if (WmiQueryObject(pRootWmi, L"Select * From WmiMonitorBrightness Where Active = True", &pMonitorBrightness) == S_OK) {
			if (pMonitorBrightness->lpVtbl->Get(pMonitorBrightness, L"CurrentBrightness", 0, &varCurrentBrightness, NULL, NULL) == S_OK) {
				if (WmiQueryObject(pRootWmi, L"Select * From WmiMonitorBrightnessMethods Where Active = True", &pMonitorBrightnessMethods) == S_OK) {
					if (WmiCreateParams(pRootWmi, L"WmiMonitorBrightnessMethods", L"WmiSetBrightness", &pParams) == S_OK) {
						varTimeout.vt = VT_I4, varTimeout.lVal = 0;
						varBrightness.vt = VT_UI1, varBrightness.bVal = addsb(varCurrentBrightness.bVal, delta);
						pParams->lpVtbl->Put(pParams, L"Timeout", 0, &varTimeout, CIM_UINT32);
						pParams->lpVtbl->Put(pParams, L"Brightness", 0, &varBrightness, CIM_UINT8);
						WmiExecMethod(pRootWmi, pMonitorBrightnessMethods, L"WmiSetBrightness", pParams);
						pParams->lpVtbl->Release(pParams);
					}
					pMonitorBrightnessMethods->lpVtbl->Release(pMonitorBrightnessMethods);
				}
			}
			pMonitorBrightness->lpVtbl->Release(pMonitorBrightness);
		}
		pRootWmi->lpVtbl->Release(pRootWmi);
	}
}

void Alpha(int delta)
{
	HWND hWnd = GetForegroundWindow();
	DWORD xstyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	BOOL layered = (xstyle & WS_EX_LAYERED);
	if (!layered) {
		if (delta < 0) {
			SetWindowLong(hWnd, GWL_EXSTYLE, xstyle | WS_EX_LAYERED);
			SetLayeredWindowAttributes(hWnd, 0, addsb(0xFF, delta), LWA_ALPHA);
		}
	} else {
		BYTE alpha, a;
		DWORD flags;
		if (!GetLayeredWindowAttributes(hWnd, NULL, &alpha, &flags) || !(flags & LWA_ALPHA))
			alpha = 0xFF;
		SetLayeredWindowAttributes(hWnd, 0, a = addsb(alpha, delta), LWA_ALPHA);
		if (a == 0xFF)
			SetWindowLong(hWnd, GWL_EXSTYLE, xstyle & ~WS_EX_LAYERED);
	}
}

//2019.07.16:SUGIHARA:CHANGE >>>>>
//void Exec(LPCTSTR cmd)
//{
//	ShellExecute(NULL, NULL, cmd, NULL, NULL, SW_SHOWNORMAL);
//}
//----------
void Exec(LPCTSTR cmd, LPCTSTR lpParameters)
{
	ShellExecute(NULL, NULL, cmd, lpParameters, NULL, SW_SHOWNORMAL);
}
//2019.07.16:SUGIHARA:CHANGE <<<<<


static struct Config config;
static TCHAR config_szCmds[ARRAYSIZE(config.cbCmds)][80];
static void Config_Initialize(void)
{
	/* single action keys */
	config.Key.Power = CONFIG_INIT_KEY_POWER;
	config.Key.Eject = CONFIG_INIT_KEY_EJECT;
	config.Key.Alnum = CONFIG_INIT_KEY_ALNUM;
	config.Key.Kana  = CONFIG_INIT_KEY_KANA;
	//2019.07.20:SUGIHARA:ADD >>>>>
	config.Key.LWin = CONFIG_INIT_KEY_LWIN;
	config.Key.RWin = CONFIG_INIT_KEY_RWIN;
	config.Key.CAPS = CONFIG_INIT_KEY_CAPS;
	//2019.07.20:SUGIHARA:ADD <<<<<
	/* Fn combination keys */
	config.Fn.F1     = CONFIG_INIT_FN_F1;
	config.Fn.F2     = CONFIG_INIT_FN_F2;
	config.Fn.F3     = CONFIG_INIT_FN_F3;
	config.Fn.F4     = CONFIG_INIT_FN_F4;
	config.Fn.F5     = CONFIG_INIT_FN_F5;
	config.Fn.F6     = CONFIG_INIT_FN_F6;
	config.Fn.F7     = CONFIG_INIT_FN_F7;
	config.Fn.F8     = CONFIG_INIT_FN_F8;
	config.Fn.F9     = CONFIG_INIT_FN_F9;
	config.Fn.F10    = CONFIG_INIT_FN_F10;
	config.Fn.F11    = CONFIG_INIT_FN_F11;
	config.Fn.F12    = CONFIG_INIT_FN_F12;
	config.Fn.Del    = CONFIG_INIT_FN_DEL;
	config.Fn.Up     = CONFIG_INIT_FN_UP;
	config.Fn.Down   = CONFIG_INIT_FN_DOWN;
	config.Fn.Left   = CONFIG_INIT_FN_LEFT;
	config.Fn.Right  = CONFIG_INIT_FN_RIGHT;
	config.Fn.Eject  = CONFIG_INIT_FN_EJECT;
	//2019.07.15:SUGIHARA:ADD >>>>>
	config.Fn.Esc    = CONFIG_INIT_FN_ESC;
	//2019.07.15:SUGIHARA:ADD <<<<<
	/* external commands */
	ZeroMemory(config.cbCmds, sizeof config.cbCmds);
	ZeroMemory(config_szCmds, sizeof config_szCmds);
}
static void Config_Load(void)
{
	TCHAR szFile[MAX_PATH];
	HANDLE hFile;
	DWORD r, i;
	struct Config conf;

	r = GetModuleFileName(NULL, szFile, ARRAYSIZE(szFile));
	lstrcpy(szFile + r - 3, TEXT("cf"));

	hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;
	if (ReadFile(hFile, &conf, sizeof conf, &r, NULL) && r == sizeof conf) {
		if (conf.Signature == CONFIG_SIGNATURE) {
			for (i = 0; i < ARRAYSIZE(conf.cbCmds); i++) {
				ZeroMemory(config_szCmds[i], ARRAYSIZE(config_szCmds[i]));
				if (conf.cbCmds[i] < ARRAYSIZE(config_szCmds[0]))
					ReadFile(hFile, config_szCmds[i], sizeof(TCHAR) * conf.cbCmds[i], &r, NULL);
			}
			config = conf;
		}
	}
	CloseHandle(hFile);
}

//2019.07.16:SUGIHARA:MOVE >>>>>
//static struct Status
//{
//	BOOL Fn;
//	//2019.07.15:SUGIHARA:ADD >>>>>
//	BOOL IsBootCamp;
//	KEYBOARD_LAYOUT* Layout;
//	//2019.07.15:SUGIHARA:ADD <<<<<
//
//} Status;
//2019.07.16:SUGIHARA:MOVE <<<<<
//2019.07.15:SUGIHARA:ADD >>>>>
static LPTSTR GetAkbcfParameter()
{
	const KEYBOARD_LAYOUT* pLayout = Status.Layout;
	if (pLayout == NULL)
	{
		//レイアウト無効の場合はパラメータ無し
		DEBUG(_T("Param-No Layout\n"));
		return NULL;
	}
	LPCTSTR format = _T("%s-%s-%s \"%s\"");
	int n = _sctprintf(format, pLayout->KeyBoardType, pLayout->KeyBoardLang, pLayout->KeyBoardSize, pLayout->Name);
	if (n <= 0)
	{
		//フォーマットできない場合はパラメータ無し
		DEBUG(_T("Param-No Param Size\n"));
		return NULL;
	}
	int len = n + 1;
	int size = sizeof(TCHAR) * len;
	TCHAR* buff = (TCHAR*)malloc(size);
	if (buff == NULL)
	{
		//バッファ確保失敗
		DEBUG(_T("Param-Malloc Failed\n"));
		return NULL;
	}
	_stprintf_s(buff, len, format, pLayout->KeyBoardType, pLayout->KeyBoardLang, pLayout->KeyBoardSize, pLayout->Name);
	return buff;
}
//2019.07.15:SUGIHARA:ADD <<<<<

static void Status_Initialize(void)
{
	ZeroMemory(&Status, sizeof Status);
}

struct Global
{
	HHOOK hHook;
	HICON hIconLarge;
	HICON hIconSmall;
} Global;
void Global_Initialize(void)
{
	Global.hHook      = NULL;
	Global.hIconLarge = NULL;
	Global.hIconSmall = NULL;
}

enum
{
	FALL_THROUGH = 0, FIRED,
};
static UINT Fire(UINT what)
{
	switch (what) {
	case 0:
		DEBUG(_T("Fire 0\n"));
		return FALL_THROUGH;
	case FIRE_NOTHING:
		DEBUG(_T("Fire Nothing\n"));
		break;
	case FIRE_POWER:
		DEBUG(_T("Fire Power\n"));
		Power();
		break;
	case FIRE_EJECT:
		DEBUG(_T("Fire Eject\n"));
		Eject();
		break;
	case FIRE_FLIP3D:
		DEBUG(_T("Fire Flip3D\n"));
		Flip3D();
		break;
	case FIRE_BRIGHT_DN:
		DEBUG(_T("Fire Bright Down\n"));
		Bright(-BRIGHT_STEP);
		break;
	case FIRE_BRIGHT_UP:
		DEBUG(_T("Fire Bright Up\n"));
		Bright(+BRIGHT_STEP);
		break;
	case FIRE_ALPHA_DN:
		DEBUG(_T("Fire Alpha Down\n"));
		Alpha(-ALPHA_DELTA);
		break;
	case FIRE_ALPHA_UP:
		DEBUG(_T("Fire Alpha Up\n"));
		Alpha(+ALPHA_DELTA);
		break;
	default:
		if (FIRE_CMD_0 <= what && what < FIRE_CMD_0 + ARRAYSIZE(config_szCmds))
		{
			DEBUG(_T("Fire Command '%s'\n"), config_szCmds[what - FIRE_CMD_0]);
			//2019.07.16:SUGIHARA:CHANGE >>>>>
			//Exec(config_szCmds[what - FIRE_CMD_0]);
			//----------
			Exec(config_szCmds[what - FIRE_CMD_0], NULL);
			//2019.07.16:SUGIHARA:CHANGE <<<<<
		}
		else
		{
			DEBUG(_T("Fire Key 0x%02X\n"), what);
			SendKey(what);
		}
	}
	return FIRED;
}

static UINT OnKeyDown(DWORD vkCode)
{
	//2019.07.15:SUGIHARA:ADD >>>>>
	DEBUG(_T("VK=0x%X Fn=%d\n"), vkCode, Status.Fn);
	//2019.07.15:SUGIHARA:ADD <<<<<
	if (Status.Fn) {
		switch (vkCode) {
		case VK_BACK : return Fire(config.Fn.Del  );
		case VK_UP   : return Fire(config.Fn.Up   );
		case VK_DOWN : return Fire(config.Fn.Down );
		case VK_LEFT : return Fire(config.Fn.Left );
		case VK_RIGHT: return Fire(config.Fn.Right);
		case VK_F1   : return Fire(config.Fn.F1   );
		case VK_F2   : return Fire(config.Fn.F2   );
		case VK_F3   : return Fire(config.Fn.F3   );
		case VK_F4   : return Fire(config.Fn.F4   );
		case VK_F5   : return Fire(config.Fn.F5   );
		case VK_F6   : return Fire(config.Fn.F6   );
		case VK_F7   : return Fire(config.Fn.F7   );
		case VK_F8   : return Fire(config.Fn.F8   );
		case VK_F9   : return Fire(config.Fn.F9   );
		case VK_F10  : return Fire(config.Fn.F10  );
		case VK_F11  : return Fire(config.Fn.F11  );
		case VK_F12  : return Fire(config.Fn.F12  );
		//2019.07.15:SUGIHARA:ADD >>>>>
		case VK_ESCAPE: return Fire(config.Fn.Esc );
		default:
			break;
		//2019.07.15:SUGIHARA:ADD <<<<<
		}
	}
	//2019.07.15:SUGIHARA:ADD >>>>>
	else
	{
		BOOL bBootCamp = Status.IsBootCamp;
		switch (vkCode) {
		case VK_DELETE:
			if (bBootCamp)
			{
				//Fn + BackSpace(Macの場合はDELETEキー) = DELETE
				DEBUG(_T("detected - Fn + BackSpace\n"));
				return Fire(config.Fn.Del);
			}
		case VK_PRIOR:
			if (bBootCamp)
			{
				//Fn + UP = PageUp
				DEBUG(_T("detected - Fn + Up\n"));
				return Fire(config.Fn.Up);
			}
		case VK_NEXT:
			if (bBootCamp)
			{
				//Fn + DOWN = PageDown
				DEBUG(_T("detected - Fn + Down\n"));
				return Fire(config.Fn.Down);
			}
		case VK_HOME:
			if (bBootCamp)
			{
				//Fn + LEFT = HOME
				DEBUG(_T("detected - Fn + Left\n"));
				return Fire(config.Fn.Left);
			}
		case VK_END:
			if (bBootCamp)
			{
				//Fn + RIGHT = END
				DEBUG(_T("detected - Fn + Right\n"));
				return Fire(config.Fn.Right);
			}
		case VK_PAUSE:
			if (bBootCamp)
			{
				//Fn + ESC = PAUSE
				DEBUG(_T("detected - Fn + ESC\n"));
				return Fire(config.Fn.Esc);
			}
		case VK_LWIN:
			DEBUG(_T("detected - VK_LWIN\n"));
			return Fire(config.Key.LWin);
		case VK_RWIN:
			DEBUG(_T("detected - VK_RWIN\n"));
			return Fire(config.Key.RWin);
		case VK_CAPITAL:
			DEBUG(_T("detected - VK_CAPITAL\n"));
			return Fire(config.Key.CAPS);
		default:
			break;
		}
	}
	//2019.07.15:SUGIHARA:ADD <<<<<

	return FALL_THROUGH;
}

enum
{
	SCANCODE_ALNUM = 113,
	SCANCODE_KANA  = 114,
};
static UINT OnScanUp(DWORD scanCode)
{
	switch (scanCode) {
	case SCANCODE_ALNUM: return Fire(config.Key.Alnum);
	case SCANCODE_KANA : return Fire(config.Key.Kana );
	}
	return FALL_THROUGH;
}

enum /* Apple Special Keys bitfield */
{
	SPECIAL_EJECT_MASK = 0x08, SPECIAL_EJECT_ON = 0x08,
	SPECIAL_FN_MASK    = 0x10, SPECIAL_FN_ON    = 0x10,
};
static void OnSpecial(UINT state)
{
	Status.Fn = (state & SPECIAL_FN_MASK) == SPECIAL_FN_ON;
	if ((state & SPECIAL_EJECT_MASK) == SPECIAL_EJECT_ON) {
		if (Status.Fn)
			Fire(config.Fn.Eject);
		else
			Fire(config.Key.Eject);
	}
}

static void OnPower(BOOL power)
{
	if (power)
		Fire(config.Key.Power);
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(Global.hHook, nCode, wParam, lParam);
	if (nCode == HC_ACTION) {
		LPKBDLLHOOKSTRUCT pkbs = (LPKBDLLHOOKSTRUCT)lParam;
		switch (pkbs->vkCode) {
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_LMENU:
		case VK_RMENU:
		case VK_LCONTROL:
		case VK_RCONTROL:
			return CallNextHookEx(Global.hHook, nCode, wParam, lParam);
		}
		if (pkbs->dwExtraInfo == MY_EXTRA_INFO)
			return CallNextHookEx(Global.hHook, nCode, wParam, lParam);

		switch (wParam) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (OnKeyDown(pkbs->vkCode))
				return TRUE;
			break;
		case WM_KEYUP:
			if (OnScanUp(pkbs->scanCode))
				return TRUE;
			break;
		}
	}
	return CallNextHookEx(Global.hHook, nCode, wParam, lParam);
}

enum
{
	RETRY_INTERVAL = 10 * 1000 /* 10sec */
};
static DWORD CALLBACK SpecialKey_Thread(LPVOID);
static struct SpecialKey
{
	HANDLE     hDevice;
	HANDLE     hThread;
	HANDLE     evTerm;
	HANDLE     evDone;
	BYTE       buffer[22];
	OVERLAPPED overlapped;
} SpecialKey;

static void SpecialKey_Initialize(void)
{
	SpecialKey.hDevice = INVALID_HANDLE_VALUE;
	SpecialKey.hThread = NULL;
	SpecialKey.evTerm  = NULL;
	SpecialKey.evDone  = NULL;
}

static BOOL SpecialKey_Prepare(void)
{
	//スペシャルキーデバイス有効判定
	if (SpecialKey.hDevice != INVALID_HANDLE_VALUE)
	{
		//有効の場合は何もしないで終了
		DEBUG(_T("SpecialKey.hDevic Enabled\n"));
		return TRUE;
	}

	//HIDのGUIDを取得
	GUID guid;
	DEBUG(_T("Get HID Guid\n"));
	WinAPI.HID.GetHidGuid(&guid);

	//デバイス情報を取得
	HDEVINFO hDevInfo;
	DEBUG(_T("Get HID Device Info\n"));
	hDevInfo = WinAPI.Setup.GetClassDevs(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
	if (!hDevInfo)
	{
		//デバイス情報が取得できなかった場合は失敗で終了
		DEBUG(_T("Failed Get HID Device Info\n"));
		return FALSE;
	}

	//デバイスインターフェイス情報取得
	DWORD index = 0;
	SP_DEVICE_INTERFACE_DATA diData;
	ZeroMemory(&diData, sizeof(diData));
	diData.cbSize = sizeof(diData);
	DEBUG(_T("Device Info Enum Start. (index=%d)\n"), index);
	BOOL bLoop = WinAPI.Setup.EnumDeviceInterfaces(hDevInfo, NULL, &guid, index, &diData);
	while(bLoop)
	{
		//デバイス詳細取得領域初期化
		BYTE diDetailBuffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + (sizeof(TCHAR) * MAX_PATH)];//malloc()の代わり。こうすればfree()しなくて済む
		PSP_DEVICE_INTERFACE_DETAIL_DATA pdiDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)diDetailBuffer;
		ZeroMemory(pdiDetail, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
		//pdiDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)diDetailImpl;
		pdiDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		//デバイス詳細取得
		DWORD sizeDetail;
		DEBUG(_T("- Get Device Detail (index=%d)\n"), index);
		BOOL bDetail = WinAPI.Setup.GetDeviceInterfaceDetail(hDevInfo,
			&diData, pdiDetail, sizeof(diDetailBuffer), &sizeDetail, NULL);
		if (bDetail)
		{
			//デバイス詳細が取得できた場合
			DEBUG(_T("- Success Get Device Detail (index=%d, Path='%s')\n"), index, pdiDetail->DevicePath);

			//非同期読込でデバイスをオープン(書込はしてないので削除した)
			HANDLE hDevice = CreateFile(pdiDetail->DevicePath,
				GENERIC_READ /*| GENERIC_WRITE*/, FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (hDevice == INVALID_HANDLE_VALUE)
			{
				//オープンできなかった場合は次のデバイスに進む
				DEBUG(_T("-- Device Not Open. (Path='%s')\n"), pdiDetail->DevicePath);
			}
			else
			{
				//オープンできた場合
				DEBUG(_T("-- Device Opened. (Path='%s')\n"), pdiDetail->DevicePath);

				//デバイスの属性を取得
				HIDD_ATTRIBUTES attr;
				ZeroMemory(&attr, sizeof(attr));
				attr.Size = sizeof(attr);
				BOOL bAttr = WinAPI.HID.GetAttributes(hDevice, &attr);
				if (bAttr)
				{
					DEBUG(_T("--- Device Attr Success. (Path='%s')\n"), pdiDetail->DevicePath);
					BOOL bSupport = IsSupportedDevice(pdiDetail->DevicePath, attr.VenderID, attr.ProductID, attr.VersionNumber);
					if (bSupport)
					{
						DEBUG(_T("---- Support Device. (Path='%s')\n"), pdiDetail->DevicePath);
						//既にデバイスが存在する場合は最後のデバイスを使うので開放する
						if (SpecialKey.hDevice != INVALID_HANDLE_VALUE)
						{
							CloseHandle(SpecialKey.hDevice);
						}
						SpecialKey.hDevice = hDevice;
					}
					else
					{
						DEBUG(_T("---- Not Support Device. (Path='%s')\n"), pdiDetail->DevicePath);
					}
				}
				else
				{
					DEBUG(_T("--- Device Attr Failed. (Path='%s')\n"), pdiDetail->DevicePath);
				}
				//サポートデバイス以外の場合
				if (hDevice != SpecialKey.hDevice)
				{
					//デバイスをクローズする
					DEBUG(_T("-- No Support Device Closed. (Path='%s')\n"), pdiDetail->DevicePath);
					CloseHandle(hDevice);
				}
			}
		}
		else
		{
			DEBUG(_T("- Failed Get Device Detail (index=%d)\n"), index);
		}
		//次のデバイスを取得する
		index += 1;
		ZeroMemory(&diData, sizeof(diData));
		diData.cbSize = sizeof(diData);
		DEBUG(_T("Device Info Enum Next. (index=%d)\n"), index);
		bLoop = WinAPI.Setup.EnumDeviceInterfaces(hDevInfo, NULL, &guid, index, &diData);
	}
	//デバイス情報を使い終わったのでクローズ
	DEBUG(_T("Device Info Destroy\n"));
	WinAPI.Setup.DestroyDeviceInfoList(hDevInfo);

	//デバイスが有効か判定
	if (SpecialKey.hDevice != INVALID_HANDLE_VALUE)
	{
		DEBUG(_T("SpecialKey.hDevice Enabled\n"));
		//スレッド起動判定
		if (SpecialKey.hThread != NULL)
		{
			//スレッドが起動している(=スレッドから呼び出されている)ので成功で終了
			DEBUG(_T("SpecialKey.Thread Runnning\n"));
			return TRUE;
		}

		//スレッド起動
		SpecialKey.evTerm = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (SpecialKey.evTerm != NULL)
		{
			SpecialKey.evDone = CreateEvent(NULL, TRUE, FALSE, NULL);
			if (SpecialKey.evDone != NULL)
			{
				DEBUG(_T("SpecialKey.Thread Create\n"));
				SpecialKey.hThread = CreateThread(NULL, 0, SpecialKey_Thread, NULL, 0, NULL);
				if (SpecialKey.hThread != NULL)
				{
					//スレッド起動成功
					DEBUG(_T("Success SpecialKey.Thread Create\n"));
					return TRUE;
				}
				CloseHandle(SpecialKey.evDone);
			}
			CloseHandle(SpecialKey.evTerm);
		}
		CloseHandle(SpecialKey.hDevice);

		//スレッドの情報を初期化
		SpecialKey_Initialize();
	}

	//ここまで来たら失敗で終了
	return FALSE;
}
static void SpecialKey_Cleanup(void)
{
	//スレッド起動中判定
	if (SpecialKey.hThread)
	{
		DEBUG(_T("Thread Stop Signal\n"));
		SetEvent(SpecialKey.evTerm);
		DEBUG(_T("Thread Stop Wait\n"));
		WaitForSingleObject(SpecialKey.evDone, INFINITE);
		DEBUG(_T("Thread Stopped\n"));
		CloseHandle(SpecialKey.evTerm);
		CloseHandle(SpecialKey.evDone);
		CloseHandle(SpecialKey.hThread);
	}

	//スレッドの情報を初期化
	SpecialKey_Initialize();

	/* reset special key status */
	Status_Initialize();
}
static DWORD CALLBACK SpecialKey_Thread(LPVOID lpParam)
{
	DEBUG(_T("[SpecialKey_Thread] Start\n"));
	HANDLE evts[2];

	UNREFERENCED_PARAMETER(lpParam);

	ZeroMemory(&SpecialKey.overlapped, sizeof SpecialKey.overlapped);
	SpecialKey.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	evts[0] = SpecialKey.overlapped.hEvent;
	evts[1] = SpecialKey.evTerm;
	for (;;) {
		DWORD r;
		ResetEvent(SpecialKey.overlapped.hEvent);
		SpecialKey.overlapped.Offset = SpecialKey.overlapped.OffsetHigh = 0;
		DEBUG(_T("[SpecialKey_Thread] Read Start\n"));
		if (!ReadFile(SpecialKey.hDevice, SpecialKey.buffer, sizeof SpecialKey.buffer,
		              &r, &SpecialKey.overlapped))
		{
			DWORD dwError = GetLastError();
			if (dwError != ERROR_IO_PENDING)
			{
				DEBUG(_T("[SpecialKey_Thread] Read ERROR (GetLastrError=%d)\n"), dwError);
				/* disconnected */
				CloseHandle(SpecialKey.hDevice);
				SpecialKey.hDevice = INVALID_HANDLE_VALUE;
				for (;;) {
					DEBUG(_T("[SpecialKey_Thread] SpecialKey_Prepare\n"));
					if (SpecialKey_Prepare())
					{
						break;
					}
					DEBUG(_T("[SpecialKey_Thread] WaitForSingleObject - evTerm\n"));
					if (WaitForSingleObject(SpecialKey.evTerm, RETRY_INTERVAL) != WAIT_TIMEOUT)
					{
						goto term;
					}
				}
				continue;
			}
			DEBUG(_T("[SpecialKey_Thread] Read Wait - evts\n"));
			if (WaitForMultipleObjects(ARRAYSIZE(evts), evts, FALSE, INFINITE) != WAIT_OBJECT_0)
			{
				break;
			}
		}
		DEBUG(_T("[SpecialKey_Thread] Readed SP-KEY=[0]%d, [1]%d\n"), SpecialKey.buffer[0], SpecialKey.buffer[1]);
		switch (SpecialKey.buffer[0]) {
		case 0x11:
			OnSpecial(SpecialKey.buffer[1]);
			break;
		case 0x13:
			OnPower(SpecialKey.buffer[1] == 1);
			break;
		}
	}
	CancelIo(SpecialKey.hDevice);
	CloseHandle(SpecialKey.hDevice);
	SpecialKey.hDevice = INVALID_HANDLE_VALUE;

term:
	CloseHandle(SpecialKey.overlapped.hEvent);

	SetEvent(SpecialKey.evDone);
	DEBUG(_T("SpecialKey_Thread EXIT\n"));
	return 0;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	enum
	{
		WM_APP_TRAYICON = WM_APP_RELOAD + 1,
	};
	static NOTIFYICONDATA nid;
	static HMENU hMenu;

	switch (uMsg) {
	case WM_APP_TRAYICON:
		switch (lParam) {
		case WM_RBUTTONDOWN:
			DEBUG(_T("> WM_RBUTTONDOWN\n"));
			SetCapture(hWnd);
			break;
		case WM_RBUTTONUP:
			DEBUG(_T("> WM_RBUTTONUP\n"));
			ReleaseCapture();
			{
				RECT rc;
				POINT pt;
				GetCursorPos(&pt);
				GetWindowRect(GetDesktopWindow(), &rc);
				SetForegroundWindow(hWnd);
				TrackPopupMenu(GetSubMenu(hMenu, 0),
					(pt.x < (rc.left + rc.right) / 2 ? TPM_LEFTALIGN : TPM_RIGHTALIGN) |
					(pt.y < (rc.top + rc.bottom) / 2 ? TPM_TOPALIGN : TPM_BOTTOMALIGN) |
					TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
				PostMessage(hWnd, WM_NULL, 0, 0);
			}
			break;
		case NIN_BALLOONHIDE:
		case NIN_BALLOONTIMEOUT:
		case NIN_BALLOONUSERCLICK:
			DEBUG(_T("> NIN_BALLOONHIDE/NIN_BALLOONTIMEOUT/NIN_BALLOONUSERCLICK\n"));
			/* error message closed */
			DestroyWindow(hWnd);
			break;
		}
		return 0;
	case WM_APP_RELOAD:
		DEBUG(_T("> WM_APP_RELOAD\n"));
		{
			Config_Load();
		}
		return 0;
	case WM_COMMAND:
		DEBUG(_T("> WM_COMMAND\n"));
		switch (LOWORD(wParam))
		{
		case ID_CONF:
			DEBUG(_T("> WM_COMMAND - ID_CONF\n"));
			{
				TCHAR cmd[MAX_PATH];
				GetModuleFileName(NULL, cmd, ARRAYSIZE(cmd));
				lstrcpy(cmd + lstrlen(cmd) - 4, TEXT("cf"));
				//2019.07.16:SUGIHARA:CHANGE >>>>>
				{
					LPTSTR szParam = GetAkbcfParameter();
					DEBUG(_T("ExecParam='%s'\n"), szParam);
					Exec(cmd, szParam);
					if (szParam != NULL)
					{
						free(szParam);
					}
				}
				//2019.07.16:SUGIHARA:CHANGE <<<<<
			}
			break;
		case ID_QUIT:
			DEBUG(_T("> WM_COMMAND - ID_EXIT\n"));
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_CREATE:
		DEBUG(_T("> WM_CREATE\n"));
		{
			HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

			Config_Load();

			/* menu */
			hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDM_MAIN));

			/* tray icon */
			ZeroMemory(&nid, sizeof nid);
			nid.cbSize           = sizeof nid;
			nid.hWnd             = hWnd;
			nid.uID              = 100;
			nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
			nid.uCallbackMessage = WM_APP_TRAYICON;
			nid.hIcon            = Global.hIconLarge;
			lstrcpy(nid.szTip, App.Title);
			Shell_NotifyIcon(NIM_ADD, &nid);
			/* error message balloon tip */
			nid.uFlags      = NIF_INFO;
			nid.dwInfoFlags = NIIF_WARNING;
			LoadString(hInstance, IDS_DEVICE_NOT_FOUND, nid.szInfo, ARRAYSIZE(nid.szInfo));
			lstrcpy(nid.szInfoTitle, App.Title);

			/* low level keyboard hook */
			Global.hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

			if (!SpecialKey_Prepare())
				Shell_NotifyIcon(NIM_MODIFY, &nid);
		}
		break;
	case WM_DESTROY:
		DEBUG(_T("> WM_DESTROY\n"));
		{
			SpecialKey_Cleanup();
			UnhookWindowsHookEx(Global.hHook);
			Shell_NotifyIcon(NIM_DELETE, &nid);
			DestroyMenu(hMenu);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int Main(HINSTANCE hInstance)
{
	WNDCLASSEX wcx;
	HWND       wnd;
	MSG        msg;

	//2019.07.21:SUGIHARA:ADD >>>>>
	{
		TCHAR szFile[MAX_PATH];
		DWORD r = GetModuleFileName(NULL, szFile, ARRAYSIZE(szFile));
		lstrcpy(szFile + r, TEXT(".log"));

		//デバッグログ(起動毎に初期化する)
		_DebugFileHandle = CreateFile(szFile, GENERIC_WRITE, FILE_SHARE_READ| FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		//クリティカルセクション
		InitializeCriticalSection(&_DebugCriticalSection);
		//開始ログ保存
		if (_DebugFileHandle != INVALID_HANDLE_VALUE)
		{
			//メモ帳でも開けるようにWCHARの場合はBOMをつける
			if (sizeof(TCHAR) == sizeof(WCHAR))
			{
				BYTE buff[] = { 0xFF,0xFE };
				DWORD writeByte = sizeof(buff);
				DWORD wroteByte = 0;
				WriteFile(_DebugFileHandle, buff, writeByte, &wroteByte, NULL);
			}
		}
		DEBUG(_T("----- Apple-Keyboard-Bridge START -----\n"));
	}
	//2019.07.21:SUGIHARA:ADD <<<<<

	if ((wnd = FindWindow(App.Class, App.Title)) != NULL)
	{
		/* reload config */
		PostMessage(wnd, WM_APP_RELOAD, 0, 0);
		return 0;
	}

	CoInitializeEx(0, COINIT_MULTITHREADED);
	CoInitializeSecurity(NULL, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	WinAPI_Initialize();
	Global_Initialize();
	Status_Initialize();
	Config_Initialize();

	SpecialKey_Initialize();

	ExtractIconEx(AppIcon.File, IsVistaOrGreater() ? AppIcon.Index.Vista : AppIcon.Index.XP,
		&Global.hIconLarge, &Global.hIconSmall, 1);

	ZeroMemory(&wcx, sizeof wcx);
	wcx.cbSize        = sizeof wcx;
	wcx.style         = CS_NOCLOSE;
	wcx.lpfnWndProc   = WndProc;
	wcx.hInstance     = hInstance;
	wcx.hCursor       = (HCURSOR)LoadImage(NULL, IDC_ARROW,
	                    IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
	wcx.lpszClassName = App.Class;
	wcx.hIcon         = Global.hIconLarge;
	wcx.hIconSm       = Global.hIconSmall;
	RegisterClassEx(&wcx);
	CreateWindowEx(0, wcx.lpszClassName, App.Title, WS_POPUP,
		CW_USEDEFAULT, 0, 10, 10, NULL, NULL, wcx.hInstance, NULL);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	DestroyIcon(Global.hIconLarge);
	DestroyIcon(Global.hIconSmall);

	WinAPI_Uninitialize();
	CoUninitialize();

	//2019.07.21:SUGIHARA:ADD >>>>>
	{
		//開始ログ保存
		DEBUG(_T("----- Apple-Keyboard-Bridge END -----\n"));
		//デバッグログ終了
		EnterCriticalSection(&_DebugCriticalSection);
		HANDLE h = _DebugFileHandle;
		if (h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
		}
		_DebugFileHandle = INVALID_HANDLE_VALUE;
		LeaveCriticalSection(&_DebugCriticalSection);
		//クリティカルセクション終了
		DeleteCriticalSection(&_DebugCriticalSection);
	}
	//2019.07.21:SUGIHARA:ADD <<<<<

	return (int)msg.wParam;
}

#ifndef NDEBUG
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	return Main(hInstance);
}
#else
void Startup(void)
{
	ExitProcess(Main(GetModuleHandle(NULL)));
}
#endif
