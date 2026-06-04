#include <stdbool.h>
#include "afs_set.h"
#include "screen.h"
#include "fs.h"

bool afs_mount()
{
    print("AFS: Initializing Apex Filesystem...\r\n");
    fs_init();
    print("AFS: Root volume mounted.\r\n");
    return true;
}
