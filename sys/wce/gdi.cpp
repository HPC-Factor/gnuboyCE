#include <windows.h>

extern "C" {

#include "defs.h"
#include "rc.h"
#include "fb.h"

void die(char *fmt, ...);
void kb_init();
void kb_close();
void kb_poll();
void joy_init();
void joy_close();
void joy_poll();

#define landscapemode(screenmode) (screenmode & 0x1)

/*
 * process graphics
 */

static int screenmode = 4;
static int frameskip = 3;

static HDC offscreen = NULL;
static HBITMAP bitmap = NULL;
static word *vram = NULL;

rcvar_t vid_exports[] =
{
	RCV_INT("screenmode", &screenmode),
	RCV_INT("frameskip", &frameskip),
	RCV_END
};

struct fb fb;

static int initok = 0;

void vid_preinit()
{
	/* do nothing; only needed on systems where we must drop perms */
}

void vid_init()
{
	if (initok) return;
	
	kb_init();
	joy_init();

	fb.ptr = (byte *) malloc(161 * 145 * 2);
	memset(fb.ptr, 0, 161 * 145 * 2);
	fb.w = 160;
	fb.h = 144;
	fb.pelsize = 2;
	fb.pitch = 322;
	fb.indexed = 0;
	fb.cc[0].r = 3;
	fb.cc[0].l = 11;
	fb.cc[1].r = 2;
	fb.cc[1].l = 5;
	fb.cc[2].r = 3;
	fb.cc[2].l = 0;
	fb.cc[3].r = 0;
	fb.cc[3].l = 0;
	fb.yuv = 0;
	fb.enabled = 1;
	fb.dirty = 0;

	initok = 1;
}

void vid_close()
{
	if (!initok) return;

	kb_close();
	joy_close();

	if (offscreen != NULL) {
		DeleteDC(offscreen);
		DeleteObject(bitmap);
	}

	initok = 0;
}

void ev_poll()
{
	kb_poll();
	joy_poll();
}

void vid_settitle(char *title)
{
	int len;
	wchar_t *title2;
	HWND wnd;

	len = strlen(title);
	title2 = (wchar_t *) malloc(sizeof(wchar_t) * (len + 1));
	mbstowcs(title2, title, len + 1);
	wnd = GetActiveWindow();
	SetWindowText(wnd, title2);
	free(title2);
}

void vid_setpal(int i, int r, int g, int b)
{
}

void vid_begin()
{
	static int skip = 0;

	skip = (skip + 1) % (frameskip > 0 ? frameskip + 1 : 1);
	fb.enabled = skip == 0;
}

void vid_setscreenmode(int mode)
{
	screenmode = mode;
}

void vid_setframeskip(int skip)
{
	frameskip = skip;
}

static void initoffscreen(HWND wnd, HDC dc)
{
	LPBITMAPINFO bmi;

	offscreen = CreateCompatibleDC(dc);

	bmi = (LPBITMAPINFO)
		malloc(sizeof BITMAPINFOHEADER + sizeof RGBQUAD * 3);
	bmi->bmiHeader.biSize = sizeof BITMAPINFOHEADER;
	bmi->bmiHeader.biWidth = 240;
	bmi->bmiHeader.biHeight = -216;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 16;
	bmi->bmiHeader.biCompression = BI_BITFIELDS;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;
	*((DWORD*) bmi->bmiColors) = 0xF800;
	*((DWORD*) bmi->bmiColors + 1) = 0x07E0;
	*((DWORD*) bmi->bmiColors + 2) = 0x001F;

	bitmap = CreateDIBSection(offscreen, bmi, DIB_RGB_COLORS,
							  (void**) &vram, NULL, 0);
	SelectObject(offscreen, bitmap);
}

static void paint160x144(word *vram, int landscape)
{
	word *fbptr;
	int fbpitch;
	int xpitch, ypitch;
	word *line, *pixel;
	int x, y;
	word *p;

	fbptr = (word *) fb.ptr;
	fbpitch = fb.pitch / 2;

	xpitch = 1;
	ypitch = 240;
	line = vram;

	for (y = 0; y < 144; y++) {
		pixel = line;
		p = fbptr;
		for (x = 0; x < 160; x++) {
			*pixel = *(p++);
			pixel += xpitch;
		}
		line += ypitch;
		fbptr += fbpitch;
	}
}

static void paint240x216(word *vram, int landscape)
{
	word *fbptr;
	int fbpitch;
	int xpitch, ypitch;
	word *line, *pixel0, *pixel1;
	int x, y;
	word *p, p0, p1;

	fbptr = (word *) fb.ptr;
	fbpitch = fb.pitch / 2;

	xpitch = 1;
	ypitch = 240;
	line = vram;

	for (y = 0; y < 72; y++) {
		pixel0 = line;
		pixel1 = line + ypitch;
		p = fbptr;
		for (x = 0; x < 80; x++) {
			p0 = *(p++);
			p1 = *(p++);

			*pixel0 = p0;
			pixel0 += xpitch;
			*pixel0 = p0;
			pixel0 += xpitch;
			*pixel0 = p1;
			pixel0 += xpitch;

			*pixel1 = p0;
			pixel1 += xpitch;
			*pixel1 = p0;
			pixel1 += xpitch;
			*pixel1 = p1;
			pixel1 += xpitch;
		}

		pixel0 = line + ypitch * 2;
		p = fbptr + fbpitch;
		for (x = 0; x < 80; x++) {
			p0 = *(p++);
			p1 = *(p++);

			*pixel0 = p0;
			pixel0 += xpitch;
			*pixel0 = p0;
			pixel0 += xpitch;
			*pixel0 = p1;
			pixel0 += xpitch;
		}

		line += ypitch * 3;
		fbptr += fbpitch * 2;
	}
}

static void paint240x216a(word *vram, int landscape)
{
	word *fbptr;
	int fbpitch;
	int xpitch, ypitch;
	word *line, *pixel0, *pixel1, *pixel2;
	int x, y;
	word *p0, *p1, *p2; 
	word p00, p00r, p00g, p00b;
	word p01, p01r, p01g, p01b;
	word p02, p02r, p02g, p02b;
	word p10, p10r, p10g, p10b;
	word p11, p11r, p11g, p11b;
	word p12, p12r, p12g, p12b;
	word p20, p20r, p20g, p20b;
	word p21, p21r, p21g, p21b;
	word p22, p22r, p22g, p22b;
	word p01r3, p01g3, p01b3;
	word p10r3, p10g3, p10b3;
	word p11r9, p11g9, p11b9;
	word p12r3, p12g3, p12b3;
	word p21r3, p21g3, p21b3;
	word p00r_p01r3, p00g_p01g3, p00b_p01b3;
	word p01r3_p02r, p01g3_p02g, p01b3_p02b;
	word p10r3_p11r9, p10g3_p11g9, p10b3_p11b9;
	word p11r9_p12r3, p11g9_p12g3, p11b9_p12b3;

	fbptr = (word *) fb.ptr;
	fbpitch = fb.pitch / 2;

	xpitch = 1;
	ypitch = 240;
	line = vram;

	for (y = 0; y < 72; y++) {
		pixel0 = line;
		pixel1 = line + ypitch;
		pixel2 = line + ypitch * 2;

		p0 = fbptr;
		p00 = *(p0++);
		p00r = (p00 & 0xf800) >> 11;
		p00g = (p00 & 0x07e0) >> 5;
		p00b = p00 & 0x001f;
		p1 = fbptr + fbpitch;
		p10 = *(p1++);
		p10r = (p10 & 0xf800) >> 11;
		p10g = (p10 & 0x07e0) >> 5;
		p10b = p10 & 0x001f;
		p2 = fbptr + fbpitch * 2;
		p20 = *(p2++);
		p20r = (p20 & 0xf800) >> 11;
		p20g = (p20 & 0x07e0) >> 5;
		p20b = p20 & 0x001f;
		p10r3 = p10r * 3;
		p10g3 = p10g * 3;
		p10b3 = p10b * 3;
		for (x = 0; x < 80; x++) {
			p01 = *(p0++);
			p01r = (p01 & 0xf800) >> 11;
			p01g = (p01 & 0x07e0) >> 5;
			p01b = p01 & 0x001f;
			p02 = *(p0++);
			p02r = (p02 & 0xf800) >> 11;
			p02g = (p02 & 0x07e0) >> 5;
			p02b = p02 & 0x001f;
			p11 = *(p1++);
			p11r = (p11 & 0xf800) >> 11;
			p11g = (p11 & 0x07e0) >> 5;
			p11b = p11 & 0x001f;
			p12 = *(p1++);
			p12r = (p12 & 0xf800) >> 11;
			p12g = (p12 & 0x07e0) >> 5;
			p12b = p12 & 0x001f;
			p21 = *(p2++);
			p21r = (p21 & 0xf800) >> 11;
			p21g = (p21 & 0x07e0) >> 5;
			p21b = p21 & 0x001f;
			p22 = *(p2++);
			p22r = (p22 & 0xf800) >> 11;
			p22g = (p22 & 0x07e0) >> 5;
			p22b = p22 & 0x001f;

			p01r3 = p01r * 3;
			p01g3 = p01g * 3;
			p01b3 = p01b * 3;
			p11r9 = p11r * 9;
			p11g9 = p11g * 9;
			p11b9 = p11b * 9;
			p12r3 = p12r * 3;
			p12g3 = p12g * 3;
			p12b3 = p12b * 3;
			p21r3 = p21r * 3;
			p21g3 = p21g * 3;
			p21b3 = p21b * 3;	

			p00r_p01r3 = p00r + p01r3;
			p00g_p01g3 = p00g + p01g3;
			p00b_p01b3 = p00b + p01b3;
			p01r3_p02r = p01r3 + p02r;
			p01g3_p02g = p01g3 + p02g;
			p01b3_p02b = p01b3 + p02b;
			p10r3_p11r9 = p10r3 + p11r9;
			p10g3_p11g9 = p10g3 + p11g9;
			p10b3_p11b9 = p10b3 + p11b9;
			p11r9_p12r3 = p11r9 + p12r3;
			p11g9_p12g3 = p11g9 + p12g3;
			p11b9_p12b3 = p11b9 + p12b3;
			
			*pixel0 = p00;
			pixel0 += xpitch;
			
			*pixel0 = (p00r_p01r3 >> 2 << 11) |
				(p00g_p01g3 >> 2 << 5) |
				(p00b_p01b3 >> 2);
			pixel0 += xpitch;
			
			*pixel0 = (p01r3_p02r >> 2 << 11) |
				(p01g3_p02g >> 2 << 5) |
				(p01b3_p02b >> 2);
			pixel0 += xpitch;

			*pixel1 = ((p00r + p10r3) >> 2 << 11) |
				((p00g + p10g3) >> 2 << 5) |
				((p00b + p10b3) >> 2);
			pixel1 += xpitch;

			*pixel1 = ((p00r_p01r3 + p10r3_p11r9) >> 4 << 11) |
				((p00g_p01g3 + p10g3_p11g9) >> 4 << 5) |
				((p00b_p01b3 + p10b3_p11b9) >> 4);
			pixel1 += xpitch;
			
			*pixel1 = ((p01r3_p02r + p11r9_p12r3) >> 4 << 11) |
				((p01g3_p02g + p11g9_p12g3) >> 4 << 5) |
				((p01b3_p02b + p11b9_p12b3) >> 4);
			pixel1 += xpitch;

			*pixel2 = ((p10r3 + p20r) >> 2 << 11) |
				((p10g3 + p20g) >> 2 << 5) |
				((p10b3 + p20b) >> 2);
			pixel2 += xpitch;

			*pixel2 = ((p10r3_p11r9 + p20r + p21r3) >> 4 << 11) |
				((p10g3_p11g9 + p20g + p21g3) >> 4 << 5) |
				((p10b3_p11b9 + p20b + p21b3) >> 4);
			pixel2 += xpitch;
			
			*pixel2 = ((p11r9_p12r3 + p21r3 + p22r) >> 4 << 11) |
				((p11g9_p12g3 + p21g3 + p22g) >> 4 << 5) |
				((p11b9_p12b3 + p21b3 + p22b) >> 4);
			pixel2 += xpitch;

			p00 = p02;
			p00r = p02r;
			p00g = p02g;
			p00b = p02b;
			p10r3 = p12r3;
			p10g3 = p12g3;
			p10b3 = p12b3;
			p20r = p22r;
			p20g = p22g;
			p20b = p22b;
		}

		line += ypitch * 3;
		fbptr += fbpitch * 2;
	}
}

static void paintonscreen(HWND wnd, HDC dc, int width, int height)
{
	RECT rt;

	GetClientRect(wnd, &rt);
	BitBlt(dc,
		   rt.left + (rt.right - rt.left - width) / 2,
		   rt.top + (rt.bottom - rt.top - height) / 2,
		   width, height, offscreen, 0, 0, SRCCOPY); 
}

void vid_end()
{
	int landscape;
	HWND wnd;
    HDC dc;

	if (!initok || !fb.enabled) return;

	wnd = GetActiveWindow();
	dc = GetDC(wnd);
	if (offscreen == NULL)
		initoffscreen(wnd, dc);

	landscape = landscapemode(screenmode);
	switch (screenmode) {
	case 2:
	case 3:
		paint240x216(vram, landscape);
		paintonscreen(wnd, dc, 240, 216);
		break;
	case 4:
	case 5:
		paint240x216a(vram, landscape);
		paintonscreen(wnd, dc, 240, 216);
		break;
	default:
		paint160x144(vram, landscape);
		paintonscreen(wnd, dc, 160, 144);
		break;
	}

	ReleaseDC(wnd, dc);
}

void vid_suspend()
{
}

void vid_resume()
{
}

}
