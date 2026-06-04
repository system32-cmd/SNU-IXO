#include "keyboard.h"

static inline unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static char translate_scancode(unsigned char code)
{
    static const char map[128] = {
        0, 0, '1', '2', '3', '4', '5', '6',
        '7', '8', '9', '0', '-', '=', '\b', '\t',
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
        'o', 'p', '[', ']', '\n', 0, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', 0, '*',
        0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
    };
    if (code < 128) {
        return map[code];
    }
    return 0;
}

char keyboard_read_char()
{
    unsigned char status = inb(0x64);
    if (!(status & 1)) {
        return 0;
    }
    unsigned char code = inb(0x60);
    if (code & 0x80) {
        return 0;
    }
    return translate_scancode(code);
}

char keyboard_wait_char()
{
    char c = 0;
    while (!c) {
        c = keyboard_read_char();
    }
    return c;
}
