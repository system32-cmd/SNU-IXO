#include "screen.h"

volatile unsigned short* vga =
    (unsigned short*)0xB8000;

static int pos = 0;

void print(const char* str)
{
    while(*str)
    {
        vga[pos++] =
            (0x0F << 8) | *str++;
    }
}

void clear_screen()
{
    for(int i=0;i<80*25;i++)
        vga[i] = 0x0720;

    pos = 0;
}