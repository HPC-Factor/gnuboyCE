#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#ifndef WCE_HPC
#include <aygshell.h>
#endif /* !WCE_HPC */
#include <sipapi.h>
#include <afx.h>

#include "resource.h"

extern "C" {

#include "Version"
#include "defs.h"
#include "input.h"

int MAIN(int argc, char *argv[]);
void die(char *fmt, ...);
int rc_setvar(char *name, int c, char **v);
int rc_getint(char *name);
int rc_command(char *line);
void vid_setscreenmode(int mode);
void vid_setframeskip(int skip);
void vid_suspend();
void vid_resume();
#ifdef WCE_HPC
int kb_process(MSG *msg);
#else /* !WCE_HPC */
int joy_process(MSG *msg);
#endif /* WCE_HPC */

#ifdef CUSTOM_CONFIG
char *defaultconfig[] =
{
	"bind esc quit",
	"bind up +up",
	"bind down +down",
	"bind left +left",
	"bind right +right",
	/*"bind alt +a",*/
	"bind d +a",
	/*"bind ctrl +b",*/
	"bind s +b",
	"bind enter +start",
	"bind space +select",
	"bind joyup +up",
	"bind joydown +down",
	"bind joyleft +left",
	"bind joyright +right",
	"bind joy0 +b",
	"bind joy1 +a",
	"bind joy2 +select",
	"bind joy3 +start",
	"bind 1 \"set saveslot 1\"",
	"bind 2 \"set saveslot 2\"",
	"bind 3 \"set saveslot 3\"",
	"bind 4 \"set saveslot 4\"",
	"bind 5 \"set saveslot 5\"",
	"bind 6 \"set saveslot 6\"",
	"bind 7 \"set saveslot 7\"",
	"bind 8 \"set saveslot 8\"",
	"bind 9 \"set saveslot 9\"",
	"bind 0 \"set saveslot 0\"",
	/*"bind ins savestate",*/
	"bind bs savestate",
	"bind del loadstate",
	"source gnuboy.rc",
	NULL
};
#endif /* CUSTOM_CONFIG */

#define STR_LEN 100
#define DOTDIR "/My Documents/gnuboy"
#define MENU_HEIGHT 26

static int paused = 0;

static HINSTANCE inst = NULL;
static HWND wnd = NULL;
static HWND cbar = NULL;
static HACCEL accel = NULL;

/*
 * standard C functions missing in Win CE
 */

time_t time(time_t* t)
{
	CTime ctime = CTime::GetCurrentTime();
	return ctime.GetTime();
}

#ifdef WCE_HPC
int isdigit(int c)
{
	return c >= '0' && c <= '9';
}
#endif /* WCE_HPC */

#ifdef WCE_HPC
int isalnum(int c)
{
	return c >= 'A' && c <= 'Z' ||
		c >= 'a' && c <= 'z' ||
		c >= '0' && c <= '9';
}
#endif /* WCE_HPC */

char *strdup(const char *src)
{
#ifdef WCE_HPC
	int len;
	char *copy;

	len = strlen(src) + 1;
	if (!(copy = (char *) malloc(len)))
		return NULL;
	memcpy(copy, src, len);

	return copy;
#else /* !WCE_HPC */
	return _strdup(src);
#endif /* WCE_HPC */
}

int strcasecmp(const char *a, const char *b)
{
#ifdef WCE_HPC
	int ca, cb; 

	while (*a != '\0') { 
		ca = *a;
		cb = *b;
		if (ca != cb) {
			if (ca >= 'a' && ca <= 'z')
				ca += 'A' - 'a'; 
			if (cb >= 'a' && cb <= 'z')
				cb += 'A' - 'a'; 
		} 
		if (ca != cb)
			break;
		a++;
		b++;
	} 
	
	return ca - cb;
#else /* !WCE_HPC */
	return _stricmp(a, b);
#endif /* WCE_HPC */
}

#ifdef WCE_HPC
char *strpbrk(const char *s1, const char *s2)
{
	char c1, c2;
	const char *s;

	while ((c1 = *s1) != 0) {
		for (s = s2; (c2 = *s) != 0; s++)
			if (c2 == c1)
				return (char *) s1;
		s1++;
	}

	return NULL;
}
#endif /* WCE_HPC */

#ifdef WCE_HPC
char *strrchr(const char *string, int c)
{
	char *res;
	
	res = NULL;
	while (*string != '\0') {
		if (*string == c)
			res = (char *) string;
		string++;
	}

	return res;
}
#endif /* WCE_HPC */

static int _vprintf(const char *fmt, va_list ap)
{
	char buf[STR_LEN];
	wchar_t buf2[STR_LEN];
	int len;
	
	len = vsprintf(buf, fmt, ap);
	mbstowcs(buf2, buf, len + 1);
	OutputDebugString(buf2);

	return len;
}

#ifndef WCE_HPC
int printf(const char *fmt, ...)
{
	va_list ap;
	int len;
	
	va_start(ap, fmt);
	len = _vprintf(fmt, ap);
	va_end(ap);

	return len;
}
#endif /* !WCE_HPC */

/*
 * emulator code
 */

static void checksaveslot(int slot)
{
	HMENU menu;

#ifdef WCE_HPC
	menu = CommandBar_GetMenu(cbar, 0);
#else /* !WCE_HPC */
	menu = (HMENU) SendMessage(cbar, SHCMBM_GETSUBMENU, 0, IDM_FILE);
#endif /* WCE_HPC */
	CheckMenuRadioItem(menu, IDM_SAVE_SLOT_0, IDM_SAVE_SLOT_9,
					   IDM_SAVE_SLOT_0 + slot, MF_BYCOMMAND);
}

static void checkscreenmode(int mode)
{
	HMENU menu;

#ifdef WCE_HPC
	menu = CommandBar_GetMenu(cbar, 0);
#else /* !WCE_HPC */
	menu = (HMENU) SendMessage(cbar, SHCMBM_GETSUBMENU, 0, IDM_TOOLS);
#endif /* WCE_HPC */
	CheckMenuRadioItem(menu, IDM_SCREEN_MODE_160X144,
					   IDM_SCREEN_MODE_240X216LA,
					   IDM_SCREEN_MODE_160X144 + mode, MF_BYCOMMAND);
}

static void checkframeskip(int frameskip)
{
	HMENU menu;

#ifdef WCE_HPC
	menu = CommandBar_GetMenu(cbar, 0);
#else /* !WCE_HPC */
	menu = (HMENU) SendMessage(cbar, SHCMBM_GETSUBMENU, 0, IDM_TOOLS);
#endif /* WCE_HPC */
	CheckMenuRadioItem(menu, IDM_FRAME_RATE_60, IDM_FRAME_RATE_10,
					   IDM_FRAME_RATE_60 + frameskip, MF_BYCOMMAND);
}

void *sys_timer()
{
	SYSTEMTIME st;
	ULARGE_INTEGER *tm;

#if 0
	GetSystemTime(&st);
	tm = (ULARGE_INTEGER *) malloc(sizeof(ULARGE_INTEGER));
	SystemTimeToFileTime(&st, (FILETIME *) tm);
#else
	tm = NULL;
#endif

	return tm;
}

int sys_elapsed(void *prev)
{
	SYSTEMTIME st;
	ULARGE_INTEGER tm, *pr;
	int usecs;

#if 0
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, (FILETIME *) &tm);
	pr = (ULARGE_INTEGER *) prev;
	usecs = (int) (tm.QuadPart - pr->QuadPart) / 10;
	*pr = tm;
#else
	usecs = 0;
#endif

	return usecs;
}

static void processmessage(MSG *msg)
{
	if (!TranslateAccelerator(msg->hwnd, accel, msg)) {
		TranslateMessage(msg);
		DispatchMessage(msg);
	}
}

static void pause(MSG *bdownmsg)
{
	RECT rt;
	MSG msg;
	int cmd;
	int ret;
	int arg;
	char rccmd[64];

	vid_suspend();

	paused = 1;
#ifdef WCE_HPC
	CommandBar_Show(cbar, 1);
#endif /* WCE_HPC */
	GetClientRect(wnd, &rt);
	InvalidateRect(wnd, &rt, 1);
	UpdateWindow(wnd);

	checksaveslot(rc_getint("saveslot"));
	checkscreenmode(rc_getint("screenmode"));
	checkframeskip(rc_getint("frameskip"));
	processmessage(bdownmsg);

	while (GetMessage(&msg, NULL, 0, 0)) {
		processmessage(&msg);

		if (msg.message == WM_COMMAND) {
			cmd = LOWORD(msg.wParam);
			ret = 0;
			switch (cmd) {
			case IDM_SAVE_SLOT_0:
			case IDM_SAVE_SLOT_1:
			case IDM_SAVE_SLOT_2:
			case IDM_SAVE_SLOT_3:
			case IDM_SAVE_SLOT_4:
			case IDM_SAVE_SLOT_5:
			case IDM_SAVE_SLOT_6:
			case IDM_SAVE_SLOT_7:
			case IDM_SAVE_SLOT_8:
			case IDM_SAVE_SLOT_9:
				arg = cmd - IDM_SAVE_SLOT_0;
				sprintf(rccmd, "set saveslot %d", arg);
				rc_command(rccmd);
				checksaveslot(arg);
				break;
			case IDM_SAVE:
				rc_command("savestate");
				ret = 1;
				break;
			case IDM_LOAD:
				rc_command("loadstate");
				ret = 1;
				break;
			case IDM_RESUME:
				ret = 1;
				break;
			case IDM_EXIT:
				rc_command("quit");
				ret = 1;
				break;
			case IDM_SCREEN_MODE_160X144:
			case IDM_SCREEN_MODE_160X144L:
			case IDM_SCREEN_MODE_240X216:
			case IDM_SCREEN_MODE_240X216L:
			case IDM_SCREEN_MODE_240X216A:
			case IDM_SCREEN_MODE_240X216LA:
				arg = cmd - IDM_SCREEN_MODE_160X144;
				checkscreenmode(arg);
				vid_setscreenmode(arg);
				break;
			case IDM_FRAME_RATE_60:
			case IDM_FRAME_RATE_30:
			case IDM_FRAME_RATE_20:
			case IDM_FRAME_RATE_15:
			case IDM_FRAME_RATE_12:
			case IDM_FRAME_RATE_10:
				arg = cmd - IDM_FRAME_RATE_60;
				checkframeskip(arg);
				vid_setframeskip(arg);
				break;
			}
			if (ret) {
				vid_resume();
				break;
			}
		}
	}

	paused = 0;
#ifdef WCE_HPC
	CommandBar_Show(cbar, 0);
#endif /* WCE_HPC */
	GetClientRect(wnd, &rt);
	InvalidateRect(wnd, &rt, 1);
	UpdateWindow(wnd);
}

static void suspend(MSG *killfocusmsg)
{
	MSG msg;

	vid_suspend();
	processmessage(killfocusmsg);

	while (GetMessage(&msg, NULL, 0, 0)) {
		processmessage(&msg);

		if (msg.message == WM_SETFOCUS) {
			vid_resume();
			break;
		}
	}
}

void sys_sleep(int us)
{
	int ms;
	MSG msg;

	/* ignore sleep duration us */

	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
#ifdef WCE_HPC
		if (kb_process(&msg))
#else /* !WCE_HPC */
		if (joy_process(&msg))
#endif /* WCE_HPC */
			continue;

		switch (msg.message) {
		case WM_PAINT:
			processmessage(&msg);
			break;
		case WM_LBUTTONDOWN:
			pause(&msg);
			break;
		case WM_KILLFOCUS:
			suspend(&msg);
			break;
		default:
			/* dispose message; is it OK? */
			break;
		}
	}
}

void sys_checkdir(char *path)
{
	wchar_t path2[MAX_PATH];
	DWORD attr;
	char *p;
	
	mbstowcs(path2, path, strlen(path) + 1);
	attr = GetFileAttributes(path2);
    if (attr == 0xFFFFFFFF) {
		p = strrchr(path, '/');
		if (!p) die("descended to root trying to create dirs\n");
		*p = 0;
		sys_checkdir(path);
		*p = '/';
		if (!CreateDirectory(path2, NULL))
			die("cannot create %s: %lu\n", path, GetLastError());
	}
}

void sys_initpath(char *exe)
{
	char *var[1];
	char *rcpath;
	char *exepath;
	char *end;
	int len;

	exepath = (char *) malloc(sizeof(char) * strlen(exe));
	if ((end = strrchr(exe, '/')) == NULL)
		rcpath = DOTDIR;
	else {
		len = end - exe;
		strncpy(exepath, exe, len);
		exepath[len] = '\0';
		rcpath = (char *)
			malloc(sizeof(char) * (strlen(DOTDIR) + len + 2));
		sprintf(rcpath, "%s;" DOTDIR, exepath);
	}
	var[0] = rcpath;
	rc_setvar("rcpath", 1, var);

	var[0] = DOTDIR;
	rc_setvar("savedir", 1, var);
}

void sys_sanitize(char *s)
{
	int i;

	for (i = 0; s[i]; i++)
		if (s[i] == '\\')
			s[i] = '/';
}

#if 0
void die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_vprintf(fmt, ap);
	va_end(ap);

	exit(1);
}
#endif

/*
 * initialization
 */

static LRESULT CALLBACK aboutbox(HWND dlg, UINT message,
								 WPARAM wpar, LPARAM lpar)
{
#ifdef WCE_HPC
	RECT rt, rt0;
#else /* !WCE_HPC */
	SHINITDLGINFO info;
#endif /* WCE_HPC */
	char version[STR_LEN];
	wchar_t version2[STR_LEN];

	switch (message) {
	case WM_INITDIALOG:
		sprintf(version, "Version %s", VERSION);
		mbstowcs(version2, version, strlen(version) + 1);
		SetWindowText(GetDlgItem(dlg, IDC_VERSION), version2);
#ifdef WCE_HPC
		if (GetWindowRect(dlg, &rt)) {
			GetClientRect(GetParent(dlg), &rt0);
			SetWindowPos(dlg, 0,
						 (rt0.right - rt0.left - (rt.right - rt.left)) / 2,
						 (rt0.bottom - rt0.top - (rt.bottom - rt.top)) / 2,
						 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
#else /* !WCE_HPC */
		info.dwMask = SHIDIM_FLAGS;
		info.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN |
			SHIDIF_SIZEDLGFULLSCREEN;
		info.hDlg = dlg;
		SHInitDialog(&info);
#endif /* WCE_HPC */
		return 1; 
	case WM_COMMAND:
		if (LOWORD(wpar) == IDOK) {
			EndDialog(dlg, LOWORD(wpar));
			return 1;
		}
		break;
	}

    return 0;
}

static HWND createcommandbar(HWND wnd)
{
#ifdef WCE_HPC
	HWND cbar;

	cbar = CommandBar_Create(inst, wnd, 1);
	CommandBar_InsertMenubar(cbar, inst, IDM_MENU, 0);	

	return cbar;
#else /* !WCE_HPC */
	SHMENUBARINFO info;

	memset(&info, 0, sizeof(SHMENUBARINFO));
	info.cbSize = sizeof(SHMENUBARINFO);
	info.hwndParent = wnd;
	info.dwFlags = SHCMBF_HIDESIPBUTTON;
	info.nToolBarId = IDM_MENU;
	info.hInstRes = inst;
	info.nBmpId = 0;
	info.cBmpImages = 0;

	if (!SHCreateMenuBar(&info)) 
		return NULL;

	return info.hwndMB;
#endif /* WCE_HPC */
}

static LRESULT CALLBACK windowproc(HWND wnd, UINT message,
								   WPARAM wpar, LPARAM lpar)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
#ifndef WCE_HPC
	SHACTIVATEINFO actinfo;
#endif /* !WCE_HPC */

	switch (message) {
	case WM_COMMAND:
		switch (LOWORD(wpar)) {	
		case IDM_ABOUT:
			DialogBox(inst, (LPCTSTR) IDD_ABOUTBOX, wnd, (DLGPROC) aboutbox);
			break;
		case IDOK:
			SendMessage(wnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0),
						(LPARAM) wnd);
			SendMessage(wnd, WM_CLOSE, 0, 0);
			break;
		default:
			return DefWindowProc(wnd, message, wpar, lpar);
		}
		break;
	case WM_CREATE:
		cbar = createcommandbar(wnd);
		break;
	case WM_PAINT: 
		hdc = BeginPaint(wnd, &ps);
		if (paused) {
			wchar_t str[STR_LEN];
			LoadString(inst, IDS_PAUSE, str, STR_LEN);
			SetBkColor(hdc, RGB(0, 0, 0));
			SetTextColor(hdc, RGB(255, 255, 255));
			GetClientRect(wnd, &rect);
			DrawText(hdc, str, _tcslen(str), &rect, 
					 DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		}
		EndPaint(wnd, &ps);
		break; 
	case WM_DESTROY:
		CommandBar_Destroy(cbar);
		PostQuitMessage(0);
		break;
#ifndef WCE_HPC
	case WM_SETTINGCHANGE:
		SHHandleWMSettingChange(wnd, wpar, lpar, &actinfo);
		break;
#endif /* !WCE_HPC */
	default:
		return DefWindowProc(wnd, message, wpar, lpar);
   }

   return 0;
}

static ATOM registerclass(HINSTANCE inst, LPTSTR szWindowClass)
{
	WNDCLASS wclass;

    wclass.style = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc = (WNDPROC) windowproc;
    wclass.cbClsExtra = 0;
    wclass.cbWndExtra = 0;
    wclass.hInstance = inst;
    wclass.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_GNUBOY));
    wclass.hCursor = 0;
    wclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wclass.lpszMenuName = 0;
    wclass.lpszClassName = szWindowClass;

	return RegisterClass(&wclass);
}

static int initinstance(HINSTANCE inst1, int show)
{
	wchar_t	title[STR_LEN];
	wchar_t	wclass[STR_LEN];
	RECT rect;

	inst = inst1;
	LoadString(inst, IDC_GNUBOY, wclass, STR_LEN);
	LoadString(inst, IDS_APP_TITLE, title, STR_LEN);

	if (wnd = FindWindow(wclass, title)) {
		SetForegroundWindow((HWND) (((DWORD) wnd) | 0x01));    
		return 0;
	} 

	registerclass(inst, wclass);
	if (!(wnd = CreateWindow(wclass, title, WS_VISIBLE,
							 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
							 CW_USEDEFAULT, NULL, NULL, inst, NULL)))
		return 0;

	GetWindowRect(wnd, &rect);
#ifndef WCE_HPC
	rect.bottom -= MENU_HEIGHT;
#endif /* !WCE_HPC */
	if (cbar)
		MoveWindow(wnd, rect.left, rect.top, rect.right, rect.bottom, 0);

	ShowWindow(wnd, show);
	UpdateWindow(wnd);

	return 1;
}

static char *getromname()
{
	OPENFILENAME openname;
	wchar_t name2[MAX_PATH];
	char *name;
	int len;
	
	memset(&openname, 0, sizeof(openname));
	openname.lStructSize	= sizeof(openname);
	openname.hwndOwner = wnd;
	openname.lpstrTitle = TEXT("gnuboy CE");
	openname.lpstrFilter =
		TEXT("GB Roms (*.gb;*.gbc)\0*.gb;*.gbc\0All Files (*.*)\0*.*\0");	
	name2[0] = '\0';
	openname.lpstrFile = name2;
	openname.nMaxFile = MAX_PATH;	
	openname.Flags = OFN_EXPLORER;

	if (!GetOpenFileName(&openname))
		return NULL;

	len = wcslen(openname.lpstrFile);
	name = (char *) malloc(sizeof(char) * (len + 1));
	wcstombs(name, openname.lpstrFile, len + 1);

	return name;
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE previnst,
				   LPTSTR cmdline, int show)
{
	MSG msg;
	int argc;
	char **argv;

	if (!initinstance(inst, show))
		return 0;
	accel = LoadAccelerators(inst, (LPCTSTR)IDC_GNUBOY);

	argc = 2;
	argv = (char **) malloc(sizeof(char *) * 2);
	argv[0] = "gnuboy.exe";
	if ((argv[1] = getromname()) == NULL)
		return 0;

#ifdef WCE_HPC
	CommandBar_Show(cbar, 0);
#endif /* WCE_HPC */

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	MAIN(argc, argv);

	return 0;
}

}
