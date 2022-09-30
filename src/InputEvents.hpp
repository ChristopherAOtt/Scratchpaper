#pragma once

#include <math.h>  // For std::is_pod and std::is_trivially_copyable
#include <cassert>  // For static_assert
#include <string.h>  // For memcmp

#include "Primitives.hpp"

/*
9/26/2021: At the moment these are all supposed to be POD
*/

/*
FORMATTING NOTE:
Any enum specifying a type should end in the word Type

CODE NOTE:
Each enum should have an INVALID state in case we get bad input
*/

enum Key{
	/*
	These are intended to roughly follow ASCII values. Above the ASCII printable range
	are other keycodes such as SHIFT, CONTROL, ALT, etc. These are set to the GLFW keycode values
	to make the conversion process more straightforward. For systems not following the GLFW values
	a more unpleasant lookup table will be required.
	*/

	KEY_INVALID = 0,               // INVALID KEY VALUE
	
	KEY_SPACE = 32,                //  
	KEY_EXCLAMATION = 33,          // !
	KEY_DOUBLE_QUOTE = 34,         // "
	KEY_POUND_SIGN = 35,           // #
	KEY_DOLLAR_SIGN = 36,          // $
	KEY_PERCENTAGE = 37,           // %
	KEY_AMPERSAND = 38,            // &
	KEY_SINGLE_QUOTE = 39,         // '
	KEY_OPEN_PARENTHESES = 40,     // (
	KEY_CLOSE_PARENTHESES = 41,    // )
	KEY_ASTERIX = 42,              // *
	KEY_PLUS_SIGN = 43,            // +
	KEY_COMMA = 44,                // ,
	KEY_MINUS_SIGN = 45,           // -
	KEY_PERIOD = 46,               // .
	KEY_FORWARD_SLASH = 47,        // /
	KEY_0 = 48,                    // 0
	KEY_1 = 49,                    // 1
	KEY_2 = 50,                    // 2
	KEY_3 = 51,                    // 3
	KEY_4 = 52,                    // 4
	KEY_5 = 53,                    // 5
	KEY_6 = 54,                    // 6
	KEY_7 = 55,                    // 7
	KEY_8 = 56,                    // 8
	KEY_9 = 57,                    // 9
	KEY_COLON = 58,                // :
	KEY_SEMICOLON = 59,            // ;
	KEY_LESS_THAN_SIGN = 60,       // <
	KEY_EQUALS_SIGN = 61,          // =
	KEY_GREATER_THAN_SIGN = 62,    // >
	KEY_QUESTION_MARK = 63,        // ?
	KEY_AT_SIGN = 64,              // @
	KEY_UPPERCASE_A = 65,          // A
	KEY_UPPERCASE_B = 66,          // B
	KEY_UPPERCASE_C = 67,          // C
	KEY_UPPERCASE_D = 68,          // D
	KEY_UPPERCASE_E = 69,          // E
	KEY_UPPERCASE_F = 70,          // F
	KEY_UPPERCASE_G = 71,          // G
	KEY_UPPERCASE_H = 72,          // H
	KEY_UPPERCASE_I = 73,          // I
	KEY_UPPERCASE_J = 74,          // J
	KEY_UPPERCASE_K = 75,          // K
	KEY_UPPERCASE_L = 76,          // L
	KEY_UPPERCASE_M = 77,          // M
	KEY_UPPERCASE_N = 78,          // N
	KEY_UPPERCASE_O = 79,          // O
	KEY_UPPERCASE_P = 80,          // P
	KEY_UPPERCASE_Q = 81,          // Q
	KEY_UPPERCASE_R = 82,          // R
	KEY_UPPERCASE_S = 83,          // S
	KEY_UPPERCASE_T = 84,          // T
	KEY_UPPERCASE_U = 85,          // U
	KEY_UPPERCASE_V = 86,          // V
	KEY_UPPERCASE_W = 87,          // W
	KEY_UPPERCASE_X = 88,          // X
	KEY_UPPERCASE_Y = 89,          // Y
	KEY_UPPERCASE_Z = 90,          // Z

	// NOTE: For GLFW keys are listed as uppercase regardless of whether shift is pressed
	// or not. 
	KEY_A = KEY_UPPERCASE_A,       // A
	KEY_B = KEY_UPPERCASE_B,       // B
	KEY_C = KEY_UPPERCASE_C,       // C
	KEY_D = KEY_UPPERCASE_D,       // D
	KEY_E = KEY_UPPERCASE_E,       // E
	KEY_F = KEY_UPPERCASE_F,       // F
	KEY_G = KEY_UPPERCASE_G,       // G
	KEY_H = KEY_UPPERCASE_H,       // H
	KEY_I = KEY_UPPERCASE_I,       // I
	KEY_J = KEY_UPPERCASE_J,       // J
	KEY_K = KEY_UPPERCASE_K,       // K
	KEY_L = KEY_UPPERCASE_L,       // L
	KEY_M = KEY_UPPERCASE_M,       // M
	KEY_N = KEY_UPPERCASE_N,       // N
	KEY_O = KEY_UPPERCASE_O,       // O
	KEY_P = KEY_UPPERCASE_P,       // P
	KEY_Q = KEY_UPPERCASE_Q,       // Q
	KEY_R = KEY_UPPERCASE_R,       // R
	KEY_S = KEY_UPPERCASE_S,       // S
	KEY_T = KEY_UPPERCASE_T,       // T
	KEY_U = KEY_UPPERCASE_U,       // U
	KEY_V = KEY_UPPERCASE_V,       // V
	KEY_W = KEY_UPPERCASE_W,       // W
	KEY_X = KEY_UPPERCASE_X,       // X
	KEY_Y = KEY_UPPERCASE_Y,       // Y
	KEY_Z = KEY_UPPERCASE_Z,       // Z

	KEY_OPEN_SQUARE_BRACKET = 91,  // [
	KEY_BACKSLASH = 92,            /* \ */
	KEY_CLOSE_SQUARE_BRACKET = 93, // ]
	KEY_CIRCUMFLEX_ACCENT = 94,    // ^
	KEY_UNDERSCORE = 95,           // _
	KEY_BACK_APOSTROPHE = 96,      // `

	KEY_LOWERCASE_A = 97,          // a
	KEY_LOWERCASE_B = 98,          // b
	KEY_LOWERCASE_C = 99,          // c
	KEY_LOWERCASE_D = 100,         // d
	KEY_LOWERCASE_E = 101,         // e
	KEY_LOWERCASE_F = 102,         // f
	KEY_LOWERCASE_G = 103,         // g
	KEY_LOWERCASE_H = 104,         // h
	KEY_LOWERCASE_I = 105,         // i
	KEY_LOWERCASE_J = 106,         // j
	KEY_LOWERCASE_K = 107,         // k
	KEY_LOWERCASE_L = 108,         // l
	KEY_LOWERCASE_M = 109,         // m
	KEY_LOWERCASE_N = 110,         // n
	KEY_LOWERCASE_O = 111,         // o
	KEY_LOWERCASE_P = 112,         // p
	KEY_LOWERCASE_Q = 113,         // q
	KEY_LOWERCASE_R = 114,         // r
	KEY_LOWERCASE_S = 115,         // s
	KEY_LOWERCASE_T = 116,         // t
	KEY_LOWERCASE_U = 117,         // u
	KEY_LOWERCASE_V = 118,         // v
	KEY_LOWERCASE_W = 119,         // w
	KEY_LOWERCASE_X = 120,         // x
	KEY_LOWERCASE_Y = 121,         // y
	KEY_LOWERCASE_Z = 122,         // z
	KEY_OPEN_CURLY_BRACKET = 123,  // {
	KEY_VERTICAL_BAR = 124,        // |
	KEY_CLOSE_CURLY_BRACKET = 125, // }
	KEY_TILDE = 126,               // ~
	//KEY_DELETE = 127,            // NOTE: This is removed to avoid conflict with code 261

	// Non-printable keycodes designed to mirror GLFW keycodes. 
	KEY_ESCAPE = 256,
	KEY_ENTER = 257,
	KEY_TAB = 258,
	KEY_BACKSPACE = 259,
	KEY_INSERT = 260,
	KEY_DELETE = 261,
	KEY_RIGHT = 262,
	KEY_LEFT = 263,
	KEY_DOWN = 264,
	KEY_UP = 265,
	KEY_PAGE_UP = 266,
	KEY_PAGE_DOWN = 267,
	KEY_HOME = 268,
	KEY_END = 269,
	KEY_CAPS_LOCK = 280,
	KEY_SCROLL_LOCK = 281,
	KEY_NUM_LOCK = 282,
	KEY_PRINT_SCREEN = 283,
	KEY_PAUSE = 284,
	KEY_F1 = 290,
	KEY_F2 = 291,
	KEY_F3 = 292,
	KEY_F4 = 293,
	KEY_F5 = 294,
	KEY_F6 = 295,
	KEY_F7 = 296,
	KEY_F8 = 297,
	KEY_F9 = 298,
	KEY_F10 = 299,
	KEY_F11 = 300,
	KEY_F12 = 301,
	KEY_F13 = 302,
	KEY_F14 = 303,
	KEY_F15 = 304,
	KEY_F16 = 305,
	KEY_F17 = 306,
	KEY_F18 = 307,
	KEY_F19 = 308,
	KEY_F20 = 309,
	KEY_F21 = 310,
	KEY_F22 = 311,
	KEY_F23 = 312,
	KEY_F24 = 313,
	KEY_F25 = 314,

	// NOTE: KP = KeyPad. Not available on all keyboards.
	KEY_KP_0 = 320,
	KEY_KP_1 = 321,
	KEY_KP_2 = 322,
	KEY_KP_3 = 323,
	KEY_KP_4 = 324,
	KEY_KP_5 = 325,
	KEY_KP_6 = 326,
	KEY_KP_7 = 327,
	KEY_KP_8 = 328,
	KEY_KP_9 = 329,
	KEY_KP_DECIMAL = 330,
	KEY_KP_DIVIDE = 331,
	KEY_KP_MULTIPLY = 332,
	KEY_KP_SUBTRACT = 333,
	KEY_KP_ADD = 334,
	KEY_KP_ENTER = 335,
	KEY_KP_EQUAL = 336,
	KEY_LEFT_SHIFT = 340,
	KEY_LEFT_CONTROL = 341,
	KEY_LEFT_ALT = 342,
	KEY_LEFT_SUPER = 343,
	KEY_RIGHT_SHIFT = 344,
	KEY_RIGHT_CONTROL = 345,
	KEY_RIGHT_ALT = 346,
	KEY_RIGHT_SUPER = 347,
	KEY_MENU = 348,
};

enum class InputSourceType{
	DEVICE_INVALID = 0,
	
	DEVICE_UNRECOGNIZED,  // Device exists, we just don't recognize it
	DEVICE_KEYBOARD,
	DEVICE_MOUSE,
	DEVICE_CONTROLLER_TYPE_1,
};

enum class KeyEventType{
	KEY_INVALID = 0,

	KEY_PRESSED,
	KEY_HELD,
	KEY_RELEASED,
};

enum class MouseEventType{
	MOUSE_INVALID = 0,

	MOUSE_MOVED,
	MOUSE_PRESSED_RIGHT,
	MOUSE_PRESSED_LEFT,
	MOUSE_RELEASED_RIGHT,
	MOUSE_RELEASED_LEFT,
	MOUSE_ENTERED_WINDOW,
	MOUSE_LEFT_WINDOW,
};

struct KeyboardEvent{
	KeyEventType event_type;
	int keycode;
};

struct MouseEvent{
	MouseEventType event_type;
	FVec2 mouse_pos;
};

struct ControllerEvent{
	float placeholder;
};

struct InputEvent{
	/*
	Input event that packages info from any kind of external input device.
	Since querying for this information can be platform-specific, there needs
	to be a way to standardize it into one format.
	*/

	InputSourceType source_type;
	union{
		KeyboardEvent keyboard;
		MouseEvent mouse;
		ControllerEvent controller;
	};
};

bool operator==(InputEvent e1, InputEvent e2);


//------------------------------------------------------------------------------
// Assertions
// TODO: Fix the awful formatting here. Figure out proper template usage.
//------------------------------------------------------------------------------
static_assert(std::is_pod<InputEvent>::value, "InputEvent must be a POD type");
static_assert(
	std::is_trivially_copyable<InputEvent>::value,
	"InputEvent must be trivially copyable");
