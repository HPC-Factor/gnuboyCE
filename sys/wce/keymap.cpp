/*
 * keymap.cpp
 *
 * Mappings from Win CE virtual key codes to local key codes.
 */

#include <windows.h>

extern "C" {

#include "input.h"

int keymap[][2] =
{
	{ VK_ESCAPE, K_ESC },
	{ '1', '1' },
	{ '2', '2' },
	{ '3', '3' },
	{ '4', '4' },
	{ '5', '5' },
	{ '6', '6' },
	{ '7', '7' },
	{ '8', '8' },
	{ '9', '9' },
	{ '0', '0' },
	{ VK_HYPHEN, K_MINUS },
	{ VK_EQUAL, K_EQUALS },
	{ VK_BACK, K_BS },
	{ VK_TAB, K_TAB },
	
	{ 'Q', 'q' },
	{ 'W', 'w' },
	{ 'E', 'e' },
	{ 'R', 'r' },
	{ 'T', 't' },
	{ 'Y', 'y' },
	{ 'U', 'u' },
	{ 'I', 'i' },
	{ 'O', 'o' },
	{ 'P', 'p' },
	
	{ VK_LBRACKET, '[' },
	{ VK_RBRACKET, ']' },
	
	{ VK_RETURN, K_ENTER },
	{ VK_CONTROL, K_CTRL },
	
	{ 'A', 'a' },
	{ 'S', 's' },
	{ 'D', 'd' },
	{ 'F', 'f' },
	{ 'G', 'g' },
	{ 'H', 'h' },
	{ 'J', 'j' },
	{ 'K', 'k' },
	{ 'L', 'l' },
	
	{ VK_SEMICOLON, K_SEMI },
	{ VK_APOSTROPHE, '\'' },
	{ VK_BACKQUOTE, K_TILDE },
	{ VK_SHIFT, K_SHIFT },
	{ VK_BACKSLASH, K_BSLASH },
	
	{ 'Z', 'z' },
	{ 'X', 'x' },
	{ 'C', 'c' },
	{ 'V', 'v' },
	{ 'B', 'b' },
	{ 'N', 'n' },
	{ 'M', 'm' },
	
	{ VK_COMMA, ',' },
	{ VK_PERIOD, '.' },
	{ VK_SLASH, '/' },
	
	/*{ 54, K_SHIFT },*/
	
	{ VK_MULTIPLY, K_NUMMUL },
	
	/*{ VK_MENU, K_ALT },*/
	{ VK_SPACE, ' ' },
	{ VK_CAPITAL, K_CAPS },
	
	{ VK_F1, K_F1 },
	{ VK_F2, K_F2 },
	{ VK_F3, K_F3 },
	{ VK_F4, K_F4 },
	{ VK_F5, K_F5 },
	{ VK_F6, K_F6 },
	{ VK_F7, K_F7 },
	{ VK_F8, K_F8 },
	{ VK_F9, K_F9 },
	{ VK_F10, K_F10 },
	
	{ VK_NUMLOCK, K_NUMLOCK },
	{ VK_SCROLL, K_SCROLL },
	
	{ VK_NUMPAD7, K_NUM7 },
	{ VK_NUMPAD8, K_NUM8 },
	{ VK_NUMPAD9, K_NUM9 },
	{ VK_SUBTRACT, K_NUMMINUS },
	{ VK_NUMPAD4, K_NUM4 },
	{ VK_NUMPAD5, K_NUM5 },
	{ VK_NUMPAD6, K_NUM6 },
	{ VK_ADD, K_NUMPLUS },
	{ VK_NUMPAD1, K_NUM1 },
	{ VK_NUMPAD2, K_NUM2 },
	{ VK_NUMPAD3, K_NUM3 },
	{ VK_NUMPAD0, K_NUM0 },
	{ VK_DECIMAL, K_NUMDOT },
	
	{ VK_F11, K_F11 },
	{ VK_F12, K_F12 },
	
	{ VK_EXECUTE, K_NUMENTER },
	/*{ 97, K_CTRL },*/
	{ VK_DIVIDE, K_NUMDIV },
	{ VK_SNAPSHOT, K_SYSRQ },
	
	/*{ 100, K_ALT },*/
	{ VK_PAUSE, K_PAUSE },
	/*{ 119, K_PAUSE },*/
	
	{ VK_HOME, K_HOME },
	{ VK_UP, K_UP },
	{ VK_PRIOR, K_PRIOR },
	{ VK_LEFT, K_LEFT },
	{ VK_RIGHT, K_RIGHT },
	{ VK_END, K_END },
	{ VK_DOWN, K_DOWN },
	{ VK_NEXT, K_NEXT },
	{ VK_INSERT, K_INS },
	{ VK_DELETE, K_DEL },

	{ 0, 0 }
};

}
