#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_MAX_NODES 128
#define FS_MAX_DATA_SIZE 32768
#define FS_MAX_NAME_SIZE 4096
#define FS_MAX_PATH 128

enum FsNodeType {
    FS_DIR = 0,
    FS_FILE = 1
};

enum FsFileType {
    FS_DATA = 0,
    FS_SCRIPT = 1,
    FS_BIN = 2,
    FS_TOOL_GCC = 3,
    FS_TOOL_FASM = 4,
    FS_TOOL_SH = 5
};

typedef struct FsNode {
    const char* name;
    uint8_t type;
    uint8_t file_type;
    const uint8_t* data;
    uint32_t size;
    int parent;
    int child;
    int sibling;
} FsNode;

void fs_init();
int fs_find_node(const char* path);
int fs_resolve_path(const char* path, char* resolved, uint32_t max);
bool fs_set_cwd(const char* path);
bool fs_make_dir(const char* path);
bool fs_write_file(const char* path, const uint8_t* data, uint32_t size, uint8_t file_type);
int fs_list(const char* path, int* out_indices, int max);
const char* fs_node_name(int index);
const uint8_t* fs_node_data(int index);
uint32_t fs_node_size(int index);
uint8_t fs_node_file_type(int index);
bool fs_node_is_executable(int index);
int fs_get_cwd_path(char* out, uint32_t max);

#ifdef __cplusplus
}
#endif
