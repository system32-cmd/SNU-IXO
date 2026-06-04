#include "screen.h"
#include "afs_set.h"
#include "disk.h"

extern "C" void kernel_main();
extern "C" void _start() { kernel_main(); }

extern "C" void kernel_main()
{
    clear_screen();
    print("SNU Apex Filesystem OS\r\n");
    print("Kernel startup...\r\n");

    if (!disk_init()) {
        print("Disk init failed\r\n");
        for (;;) asm volatile("hlt");
    }

    if (afs_mount()) {
        print("AFS mount OK\r\n");
    } else {
        print("AFS mount failed\r\n");
    }

    print("SNU is ready. Press any key in example to continue...\r\n");
    for (;;) asm volatile("hlt");
}
