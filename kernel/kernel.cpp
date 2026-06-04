#include "screen.h"
#include "afs_set.h"
#include "disk.h"
#include "shell.h"

extern "C" void kernel_main();
extern "C" void start() { kernel_main(); }

extern "C" void kernel_main()
{
    clear_screen();
    print("SNU Apex Filesystem OS\r\n");
    print("Kernel startup...\r\n");

    if (!disk_init()) {
        print("Disk init failed\r\n");
        for (;;) asm volatile("hlt");
    }

    if (!afs_mount()) {
        print("AFS mount failed\r\n");
        for (;;) asm volatile("hlt");
    }

    shell_main();
}
