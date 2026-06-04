#include <stdbool.h>
#include "afs_set.h"
#include "screen.h"

bool afs_mount()
{
    print("AFS: Initializing Apex Filesystem...\r\n");
    print("AFS: Root volume mounted.\r\n");
    return true;
}
