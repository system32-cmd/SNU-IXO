#include "fs.h"
#include "screen.h"
#include <cstddef>

static FsNode fs_nodes[FS_MAX_NODES];
static uint8_t fs_data_pool[FS_MAX_DATA_SIZE];
static char fs_name_pool[FS_MAX_NAME_SIZE];
static uint32_t fs_data_top = 0;
static uint32_t fs_name_top = 0;
static int fs_node_count = 0;
static int fs_cwd = 0;

static uint32_t fs_strlen_internal(const char* text)
{
    uint32_t len = 0;
    while (text[len]) {
        len++;
    }
    return len;
}

static void fs_memcpy(void* dest, const void* src, uint32_t count)
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (count--) {
        *d++ = *s++;
    }
}

static char* fs_strdup(const char* text)
{
    uint32_t len = fs_strlen_internal(text) + 1;
    if (fs_name_top + len > FS_MAX_NAME_SIZE) {
        return NULL;
    }
    char* dst = fs_name_pool + fs_name_top;
    fs_memcpy(dst, text, len);
    fs_name_top += len;
    return dst;
}

static uint8_t* fs_alloc_data(const uint8_t* data, uint32_t size)
{
    if (fs_data_top + size > FS_MAX_DATA_SIZE) {
        return NULL;
    }
    uint8_t* dst = fs_data_pool + fs_data_top;
    fs_memcpy(dst, data, size);
    fs_data_top += size;
    return dst;
}

static int fs_add_node(const char* name, uint8_t type, uint8_t file_type, const uint8_t* data, uint32_t size, int parent)
{
    if (fs_node_count >= FS_MAX_NODES) {
        return -1;
    }
    int index = fs_node_count++;
    FsNode* node = &fs_nodes[index];
    node->name = fs_strdup(name);
    node->type = type;
    node->file_type = file_type;
    node->size = size;
    node->parent = parent;
    node->child = -1;
    node->sibling = -1;
    if (data && size > 0) {
        node->data = fs_alloc_data(data, size);
    } else {
        node->data = NULL;
    }
    if (parent >= 0) {
        FsNode* parent_node = &fs_nodes[parent];
        if (parent_node->child < 0) {
            parent_node->child = index;
        } else {
            int sibling = parent_node->child;
            while (fs_nodes[sibling].sibling >= 0) {
                sibling = fs_nodes[sibling].sibling;
            }
            fs_nodes[sibling].sibling = index;
        }
    }
    return index;
}

static int fs_find_child(int parent, const char* name)
{
    if (parent < 0 || parent >= fs_node_count) {
        return -1;
    }
    for (int child = fs_nodes[parent].child; child >= 0; child = fs_nodes[child].sibling) {
        const char* child_name = fs_nodes[child].name;
        bool equal = true;
        int i = 0;
        while (child_name[i] || name[i]) {
            if (child_name[i] != name[i]) {
                equal = false;
                break;
            }
            i++;
        }
        if (equal) {
            return child;
        }
    }
    return -1;
}

static int fs_find_name_in_path(const char* path, char* segment, int max_len)
{
    int i = 0;
    while (*path && *path != '/' && *path != '\0' && i < max_len - 1) {
        segment[i++] = *path++;
    }
    segment[i] = 0;
    return i;
}

int fs_find_node(const char* path)
{
    if (!path || !*path) {
        return fs_cwd;
    }
    int current = 0;
    const char* ptr = path;
    if (*ptr == '/') {
        current = 0;
        while (*ptr == '/') ptr++;
    } else {
        current = fs_cwd;
    }
    char segment[FS_MAX_PATH];
    while (*ptr) {
        if (*ptr == '/') {
            ptr++;
            continue;
        }
        fs_find_name_in_path(ptr, segment, sizeof(segment));
        while (*ptr && *ptr != '/') ptr++;
        if (segment[0] == '.' && segment[1] == 0) {
            continue;
        }
        if (segment[0] == '.' && segment[1] == '.' && segment[2] == 0) {
            if (current > 0) {
                current = fs_nodes[current].parent;
            }
            continue;
        }
        int child = fs_find_child(current, segment);
        if (child < 0) {
            return -1;
        }
        current = child;
    }
    return current;
}

static bool fs_split_parent(const char* path, char* parent, char* name)
{
    int len = fs_strlen_internal(path);
    if (len == 0) return false;
    int cut = len;
    while (cut > 0 && path[cut - 1] == '/') {
        cut--;
    }
    while (cut > 0 && path[cut - 1] != '/') {
        cut--;
    }
    if (cut == 0) {
        parent[0] = '/';
        parent[1] = 0;
        int j = 0;
        for (int i = 0; i < len; i++) {
            if (path[i] != '/') {
                name[j++] = path[i];
            }
        }
        name[j] = 0;
        return true;
    }
    int j = 0;
    for (int i = 0; i < cut; i++) {
        parent[j++] = path[i];
    }
    parent[j] = 0;
    while (cut < len && path[cut] == '/') {
        cut++;
    }
    j = 0;
    for (int i = cut; i < len; i++) {
        name[j++] = path[i];
    }
    name[j] = 0;
    return true;
}

bool fs_make_dir(const char* path)
{
    if (!path || !*path) {
        return false;
    }
    int current = (*path == '/') ? 0 : fs_cwd;
    const char* ptr = path;
    if (*ptr == '/') {
        while (*ptr == '/') ptr++;
    }
    char segment[FS_MAX_PATH];
    while (*ptr) {
        fs_find_name_in_path(ptr, segment, sizeof(segment));
        while (*ptr && *ptr != '/') ptr++;
        if (segment[0] == 0 || (segment[0] == '.' && segment[1] == 0)) {
            continue;
        }
        if (segment[0] == '.' && segment[1] == '.' && segment[2] == 0) {
            if (current > 0) {
                current = fs_nodes[current].parent;
            }
            continue;
        }
        int child = fs_find_child(current, segment);
        if (child < 0) {
            child = fs_add_node(segment, FS_DIR, FS_DATA, NULL, 0, current);
            if (child < 0) {
                return false;
            }
        }
        current = child;
    }
    return true;
}

int fs_resolve_path(const char* path, char* resolved, uint32_t max)
{
    int node = fs_find_node(path);
    if (node < 0) {
        return -1;
    }
    if (resolved) {
        char temp[FS_MAX_PATH];
        int length = 0;
        int current = node;
        while (current > 0) {
            const char* name = fs_nodes[current].name;
            int name_len = fs_strlen_internal(name);
            if (length + name_len + 1 >= (int)max) {
                return -1;
            }
            for (int i = name_len - 1; i >= 0; i--) {
                temp[length++] = name[i];
            }
            temp[length++] = '/';
            current = fs_nodes[current].parent;
        }
        if (length == 0) {
            if (max > 1) {
                resolved[0] = '/';
                resolved[1] = 0;
            }
            return 0;
        }
        int out_len = 0;
        while (length > 0) {
            resolved[out_len++] = temp[--length];
        }
        resolved[out_len] = 0;
    }
    return node;
}

int fs_list(const char* path, int* out_indices, int max)
{
    int idx = fs_find_node(path);
    if (idx < 0) {
        return -1;
    }
    if (fs_nodes[idx].type != FS_DIR) {
        return -1;
    }
    int count = 0;
    for (int child = fs_nodes[idx].child; child >= 0; child = fs_nodes[child].sibling) {
        if (count < max) {
            out_indices[count] = child;
        }
        count++;
    }
    return count;
}

const char* fs_node_name(int index)
{
    if (index < 0 || index >= fs_node_count) {
        return "";
    }
    return fs_nodes[index].name;
}

const uint8_t* fs_node_data(int index)
{
    if (index < 0 || index >= fs_node_count) {
        return NULL;
    }
    return fs_nodes[index].data;
}

uint32_t fs_node_size(int index)
{
    if (index < 0 || index >= fs_node_count) {
        return 0;
    }
    return fs_nodes[index].size;
}

uint8_t fs_node_file_type(int index)
{
    if (index < 0 || index >= fs_node_count) {
        return FS_DATA;
    }
    return fs_nodes[index].file_type;
}

bool fs_node_is_executable(int index)
{
    if (index < 0 || index >= fs_node_count) {
        return false;
    }
    uint8_t type = fs_nodes[index].file_type;
    return type == FS_SCRIPT || type == FS_BIN || type == FS_TOOL_GCC || type == FS_TOOL_FASM || type == FS_TOOL_SH;
}

bool fs_write_file(const char* path, const uint8_t* data, uint32_t size, uint8_t file_type)
{
    char parent_path[FS_MAX_PATH];
    char name[FS_MAX_PATH];
    if (!fs_split_parent(path, parent_path, name)) {
        return false;
    }
    int parent = fs_find_node(parent_path);
    if (parent < 0 || fs_nodes[parent].type != FS_DIR) {
        return false;
    }
    int existing = fs_find_child(parent, name);
    if (existing >= 0) {
        if (fs_nodes[existing].data && fs_nodes[existing].size > 0) {
            // new data may reuse pool, but old data remains valid; we ignore freeing.
        }
        fs_nodes[existing].data = fs_alloc_data(data, size);
        if (!fs_nodes[existing].data) {
            return false;
        }
        fs_nodes[existing].size = size;
        fs_nodes[existing].file_type = file_type;
        return true;
    }
    int node = fs_add_node(name, FS_FILE, file_type, data, size, parent);
    return node >= 0;
}

int fs_get_cwd_path(char* out, uint32_t max)
{
    if (!out || max == 0) {
        return -1;
    }
    int node = fs_cwd;
    if (node < 0 || node >= fs_node_count) {
        return -1;
    }
    char temp[FS_MAX_PATH];
    int pos = 0;
    while (node > 0) {
        const char* name = fs_nodes[node].name;
        uint32_t len = fs_strlen_internal(name);
        if (pos + len + 1 >= FS_MAX_PATH) {
            return -1;
        }
        for (uint32_t i = 0; i < len; i++) {
            temp[pos + len - 1 - i] = name[i];
        }
        pos += len;
        temp[pos++] = '/';
        node = fs_nodes[node].parent;
    }
    if (pos == 0) {
        if (max > 1) {
            out[0] = '/';
            out[1] = 0;
            return 1;
        }
        return -1;
    }
    if ((uint32_t)pos >= max) {
        return -1;
    }
    for (int i = 0; i < pos; i++) {
        out[i] = temp[pos - 1 - i];
    }
    out[pos] = 0;
    return 0;
}

bool fs_set_cwd(const char* path)
{
    int idx = fs_find_node(path);
    if (idx < 0 || fs_nodes[idx].type != FS_DIR) {
        return false;
    }
    fs_cwd = idx;
    return true;
}

void fs_init()
{
    fs_data_top = 0;
    fs_name_top = 0;
    fs_node_count = 0;
    fs_cwd = 0;
    int root = fs_add_node("", FS_DIR, FS_DATA, NULL, 0, -1);
    int root_dir = fs_add_node("root", FS_DIR, FS_DATA, NULL, 0, root);
    int home = fs_add_node("home", FS_DIR, FS_DATA, NULL, 0, root);
    int sys = fs_add_node("sys", FS_DIR, FS_DATA, NULL, 0, root);
    int pkgs = fs_add_node("pkgs", FS_DIR, FS_DATA, NULL, 0, root);
    int sys_bin = fs_add_node("bin", FS_DIR, FS_DATA, NULL, 0, sys);
    fs_add_node("src", FS_DIR, FS_DATA, NULL, 0, sys);
    fs_add_node("gcc", FS_FILE, FS_TOOL_GCC, NULL, 0, sys_bin);
    fs_add_node("fasm", FS_FILE, FS_TOOL_FASM, NULL, 0, sys_bin);
    fs_add_node("sh", FS_FILE, FS_TOOL_SH, NULL, 0, sys_bin);

    static const uint8_t init_service[] =
        "[Unit]\n"
        "Description=SNU init service\n"
        "[Service]\n"
        "ExecStart=/sys/bin/sh /sys/init.sh\n";
    static const uint8_t init_script[] =
        "# SNU init script\n"
        "echo Starting SNU init sequence\n"
        "mkdir /home/projects\n"
        "gcc -o /home/projects/hello.bin /pkgs/hello.c\n"
        "fasm -o /home/projects/build.bin /pkgs/build.asm\n"
        "run /home/projects/hello.bin\n"
        "run /home/projects/build.bin\n";
    static const uint8_t hello_c[] =
        "int main() {\n"
        "    puts(\"Hello from GCC tool!\");\n"
        "    return 0;\n"
        "}\n";
    static const uint8_t build_asm[] =
        "PRINT \"Hello from FASM tool!\"\n"
        "EXIT\n";
    static const uint8_t welcome[] =
        "Welcome to SNU userland. Use ls, cd, cat, mkdir, gcc, fasm, run, sh or help.\n";
    static const uint8_t user_txt[] =
        "This is /home/user.txt inside the SNU virtual filesystem.\n";
    fs_add_node("init.service", FS_FILE, FS_DATA, init_service, sizeof(init_service) - 1, sys);
    fs_add_node("init.sh", FS_FILE, FS_SCRIPT, init_script, sizeof(init_script) - 1, sys);
    fs_add_node("hello.c", FS_FILE, FS_DATA, hello_c, sizeof(hello_c) - 1, pkgs);
    fs_add_node("build.asm", FS_FILE, FS_DATA, build_asm, sizeof(build_asm) - 1, pkgs);
    fs_add_node("welcome.txt", FS_FILE, FS_DATA, welcome, sizeof(welcome) - 1, root_dir);
    fs_add_node("user.txt", FS_FILE, FS_DATA, user_txt, sizeof(user_txt) - 1, home);
    fs_cwd = home;
}
