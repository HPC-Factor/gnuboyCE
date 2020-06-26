#include <windows.h>

extern "C" {

#include "defs.h"
#include "rc.h"
#include "input.h"

void die(char *fmt, ...);

/*
 * process keyboard
 */

extern int keymap[][2];

int keymap1[256];

void kb_init()
{
	int i;

	for (i = 0; i < 256; i++)
		keymap1[i] = 0;
	for (i = 0; keymap[i][0] != 0; i++)
		keymap1[keymap[i][0]] = keymap[i][1];
}

void kb_close()
{
}

int kb_process(MSG *msg)
{
	event_t ev;

	if (msg->message != WM_KEYDOWN &&
		msg->message != WM_KEYUP)
		return 0;

	if ((ev.code = keymap1[msg->wParam]) == 0)
		return 1;

	if (msg->message == WM_KEYDOWN)
		ev.type = EV_PRESS;
	else /* msg->message == WM_KEYUP */
		ev.type = EV_RELEASE;

	/* posting a key event here may be inpolite, but actually works */
	ev_postevent(&ev);

	return 1;
}

void kb_poll()
{
	/* do nothing here since key events have already been processed */
}

}
