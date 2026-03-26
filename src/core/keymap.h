#ifndef KEYMAP_H
#define KEYMAP_H

#include <stddef.h>
#include <string.h>

/*
 * macOS virtual key codes (from Events.h / Carbon HIToolbox)
 * N_VIRTUAL_KEY covers all possible key codes we track.
 */

#define N_VIRTUAL_KEY 146

/* Letters */
#define KBF_KEY_A           0x00
#define KBF_KEY_S           0x01
#define KBF_KEY_D           0x02
#define KBF_KEY_F           0x03
#define KBF_KEY_H           0x04
#define KBF_KEY_G           0x05
#define KBF_KEY_Z           0x06
#define KBF_KEY_X           0x07
#define KBF_KEY_C           0x08
#define KBF_KEY_V           0x09
#define KBF_KEY_B           0x0B
#define KBF_KEY_Q           0x0C
#define KBF_KEY_W           0x0D
#define KBF_KEY_E           0x0E
#define KBF_KEY_R           0x0F
#define KBF_KEY_Y           0x10
#define KBF_KEY_T           0x11
#define KBF_KEY_1           0x12
#define KBF_KEY_2           0x13
#define KBF_KEY_3           0x14
#define KBF_KEY_4           0x15
#define KBF_KEY_6           0x16
#define KBF_KEY_5           0x17
#define KBF_KEY_EQUAL       0x18
#define KBF_KEY_9           0x19
#define KBF_KEY_7           0x1A
#define KBF_KEY_MINUS       0x1B
#define KBF_KEY_8           0x1C
#define KBF_KEY_0           0x1D
#define KBF_KEY_RBRACKET    0x1E
#define KBF_KEY_O           0x1F
#define KBF_KEY_U           0x20
#define KBF_KEY_LBRACKET    0x21
#define KBF_KEY_I           0x22
#define KBF_KEY_P           0x23
#define KBF_KEY_RETURN      0x24
#define KBF_KEY_L           0x25
#define KBF_KEY_J           0x26
#define KBF_KEY_QUOTE       0x27
#define KBF_KEY_K           0x28
#define KBF_KEY_SEMICOLON   0x29
#define KBF_KEY_BACKSLASH   0x2A
#define KBF_KEY_COMMA       0x2B
#define KBF_KEY_SLASH       0x2C
#define KBF_KEY_N           0x2D
#define KBF_KEY_M           0x2E
#define KBF_KEY_PERIOD      0x2F
#define KBF_KEY_TAB         0x30
#define KBF_KEY_SPACE       0x31
#define KBF_KEY_GRAVE       0x32
#define KBF_KEY_DELETE      0x33
#define KBF_KEY_ESCAPE      0x35
#define KBF_KEY_COMMAND     0x37
#define KBF_KEY_LSHIFT      0x38
#define KBF_KEY_CAPSLOCK    0x39
#define KBF_KEY_LOPTION     0x3A
#define KBF_KEY_LCONTROL    0x3B
#define KBF_KEY_RSHIFT      0x3C
#define KBF_KEY_ROPTION     0x3D
#define KBF_KEY_RCONTROL    0x3E
#define KBF_KEY_FN          0x3F

/* Function keys */
#define KBF_KEY_F17         0x40
#define KBF_KEY_KP_DECIMAL  0x41
#define KBF_KEY_KP_MULTIPLY 0x43
#define KBF_KEY_KP_PLUS     0x45
#define KBF_KEY_KP_CLEAR    0x47
#define KBF_KEY_VOLUME_UP   0x48
#define KBF_KEY_VOLUME_DOWN 0x49
#define KBF_KEY_MUTE        0x4A
#define KBF_KEY_KP_DIVIDE   0x4B
#define KBF_KEY_KP_ENTER    0x4C
#define KBF_KEY_KP_MINUS    0x4E
#define KBF_KEY_F18         0x4F
#define KBF_KEY_F19         0x50
#define KBF_KEY_KP_EQUALS   0x51
#define KBF_KEY_KP_0        0x52
#define KBF_KEY_KP_1        0x53
#define KBF_KEY_KP_2        0x54
#define KBF_KEY_KP_3        0x55
#define KBF_KEY_KP_4        0x56
#define KBF_KEY_KP_5        0x57
#define KBF_KEY_KP_6        0x58
#define KBF_KEY_KP_7        0x59
#define KBF_KEY_KP_8        0x5B
#define KBF_KEY_KP_9        0x5C
#define KBF_KEY_F5          0x60
#define KBF_KEY_F6          0x61
#define KBF_KEY_F7          0x62
#define KBF_KEY_F3          0x63
#define KBF_KEY_F8          0x64
#define KBF_KEY_F9          0x65
#define KBF_KEY_F11         0x67
#define KBF_KEY_F13         0x69
#define KBF_KEY_F16         0x6A
#define KBF_KEY_F14         0x6B
#define KBF_KEY_F10         0x6D
#define KBF_KEY_F12         0x6F
#define KBF_KEY_F15         0x71
#define KBF_KEY_HELP        0x72
#define KBF_KEY_HOME        0x73
#define KBF_KEY_PAGEUP      0x74
#define KBF_KEY_FWD_DELETE  0x75
#define KBF_KEY_F4          0x76
#define KBF_KEY_END         0x77
#define KBF_KEY_F2          0x78
#define KBF_KEY_PAGEDOWN    0x79
#define KBF_KEY_F1          0x7A
#define KBF_KEY_LEFT        0x7B
#define KBF_KEY_RIGHT       0x7C
#define KBF_KEY_DOWN        0x7D
#define KBF_KEY_UP          0x7E

/* Map key code to human-readable name */
static const char* keycode_names[N_VIRTUAL_KEY] = {
    [0x00] = "a",
    [0x01] = "s",
    [0x02] = "d",
    [0x03] = "f",
    [0x04] = "h",
    [0x05] = "g",
    [0x06] = "z",
    [0x07] = "x",
    [0x08] = "c",
    [0x09] = "v",
    [0x0A] = "section",
    [0x0B] = "b",
    [0x0C] = "q",
    [0x0D] = "w",
    [0x0E] = "e",
    [0x0F] = "r",
    [0x10] = "y",
    [0x11] = "t",
    [0x12] = "1",
    [0x13] = "2",
    [0x14] = "3",
    [0x15] = "4",
    [0x16] = "6",
    [0x17] = "5",
    [0x18] = "equal",
    [0x19] = "9",
    [0x1A] = "7",
    [0x1B] = "minus",
    [0x1C] = "8",
    [0x1D] = "0",
    [0x1E] = "rbracket",
    [0x1F] = "o",
    [0x20] = "u",
    [0x21] = "lbracket",
    [0x22] = "i",
    [0x23] = "p",
    [0x24] = "return",
    [0x25] = "l",
    [0x26] = "j",
    [0x27] = "quote",
    [0x28] = "k",
    [0x29] = "semicolon",
    [0x2A] = "backslash",
    [0x2B] = "comma",
    [0x2C] = "slash",
    [0x2D] = "n",
    [0x2E] = "m",
    [0x2F] = "period",
    [0x30] = "tab",
    [0x31] = "space",
    [0x32] = "grave",
    [0x33] = "delete",
    [0x34] = NULL,
    [0x35] = "escape",
    [0x36] = "rcommand",
    [0x37] = "command",
    [0x38] = "lshift",
    [0x39] = "capslock",
    [0x3A] = "loption",
    [0x3B] = "lcontrol",
    [0x3C] = "rshift",
    [0x3D] = "roption",
    [0x3E] = "rcontrol",
    [0x3F] = "fn",
    [0x40] = "f17",
    [0x41] = "kp_decimal",
    [0x42] = NULL,
    [0x43] = "kp_multiply",
    [0x44] = NULL,
    [0x45] = "kp_plus",
    [0x46] = NULL,
    [0x47] = "kp_clear",
    [0x48] = "volume_up",
    [0x49] = "volume_down",
    [0x4A] = "mute",
    [0x4B] = "kp_divide",
    [0x4C] = "kp_enter",
    [0x4D] = NULL,
    [0x4E] = "kp_minus",
    [0x4F] = "f18",
    [0x50] = "f19",
    [0x51] = "kp_equals",
    [0x52] = "kp_0",
    [0x53] = "kp_1",
    [0x54] = "kp_2",
    [0x55] = "kp_3",
    [0x56] = "kp_4",
    [0x57] = "kp_5",
    [0x58] = "kp_6",
    [0x59] = "kp_7",
    [0x5A] = NULL,
    [0x5B] = "kp_8",
    [0x5C] = "kp_9",
    [0x5D] = NULL,
    [0x5E] = NULL,
    [0x5F] = NULL,
    [0x60] = "f5",
    [0x61] = "f6",
    [0x62] = "f7",
    [0x63] = "f3",
    [0x64] = "f8",
    [0x65] = "f9",
    [0x66] = NULL,
    [0x67] = "f11",
    [0x68] = NULL,
    [0x69] = "f13",
    [0x6A] = "f16",
    [0x6B] = "f14",
    [0x6C] = NULL,
    [0x6D] = "f10",
    [0x6E] = NULL,
    [0x6F] = "f12",
    [0x70] = NULL,
    [0x71] = "f15",
    [0x72] = "help",
    [0x73] = "home",
    [0x74] = "pageup",
    [0x75] = "fwd_delete",
    [0x76] = "f4",
    [0x77] = "end",
    [0x78] = "f2",
    [0x79] = "pagedown",
    [0x7A] = "f1",
    [0x7B] = "left",
    [0x7C] = "right",
    [0x7D] = "down",
    [0x7E] = "up",
};

/* Resolve key name string to key code. Returns -1 if not found. */
static inline int keycode_from_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; i < N_VIRTUAL_KEY; i++) {
        if (keycode_names[i] && strcmp(keycode_names[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Get human-readable name for a key code. Returns "unknown" if not mapped. */
static inline const char* keycode_to_name(int keycode) {
    if (keycode < 0 || keycode >= N_VIRTUAL_KEY) return "unknown";
    const char *name = keycode_names[keycode];
    return name ? name : "unknown";
}

#endif /* KEYMAP_H */
