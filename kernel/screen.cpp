#include "screen.h"

volatile unsigned short* vga =
    (unsigned short*)0xB8000;

static int pos = 0;

void print_char(char c)
{
    if (c == '\r') {
        pos = (pos / 80) * 80;
        return;
    }
    if (c == '\n') {
        pos = ((pos / 80) + 1) * 80;
        if (pos >= 80 * 25) {
            for (int row = 1; row < 25; row++) {
                for (int col = 0; col < 80; col++) {
                    vga[(row - 1) * 80 + col] = vga[row * 80 + col];
                }
            }
            for (int col = 0; col < 80; col++) {
                vga[24 * 80 + col] = 0x0720;
            }
            pos = 24 * 80;
        }
        return;
    }
    if (c == '\b') {
        if (pos > 0) {
            pos--;
            vga[pos] = 0x0720;
        }
        return;
    }
    vga[pos++] = (0x0F << 8) | c;
    if (pos >= 80 * 25) {
        for (int row = 1; row < 25; row++) {
            for (int col = 0; col < 80; col++) {
                vga[(row - 1) * 80 + col] = vga[row * 80 + col];
            }
        }
        for (int col = 0; col < 80; col++) {
            vga[24 * 80 + col] = 0x0720;
        }
        pos = 24 * 80;
    }
}

void print(const char* str)
{
    while (*str) {
        print_char(*str++);
    }
}

void clear_screen()
{
    for(int i=0;i<80*25;i++)
        vga[i] = 0x0720;

    pos = 0;
}