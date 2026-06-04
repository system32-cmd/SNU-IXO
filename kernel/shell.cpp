#include "shell.h"
#include "fs.h"
#include "screen.h"
#include "keyboard.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int strcmp(const char* a, const char* b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

size_t strlen(const char* s)
{
    size_t len = 0;
    while (*s++) {
        len++;
        s++;
    }
    return len;
}

void __chkstk_ms()
{
}

#ifdef __cplusplus
}
#endif

static bool handle_gcc(int argc, char* argv[]);
static bool handle_fasm(int argc, char* argv[]);
static bool handle_sh(int argc, char* argv[]);

static void print_line(const char* text)
{
    print(text);
    print("\r\n");
}

static void prompt()
{
    char cwd[FS_MAX_PATH];
    if (fs_get_cwd_path(cwd, sizeof(cwd))) {
        print(cwd);
    } else {
        print("/");
    }
    print(" $ ");
}

static int read_line(char* buffer, int max)
{
    int idx = 0;
    while (true) {
        char c = keyboard_wait_char();
        if (c == '\r' || c == '\n') {
            print("\r\n");
            break;
        }
        if (c == '\b') {
            if (idx > 0) {
                idx -= 1;
                print("\b \b");
            }
            continue;
        }
        if (c < 32) {
            continue;
        }
        if (idx < max - 1) {
            buffer[idx++] = c;
            char temp[2] = { c, 0 };
            print(temp);
        }
    }
    buffer[idx] = 0;
    return idx;
}

static int tokenize(char* line, char* argv[], int max)
{
    int argc = 0;
    char* p = line;
    while (*p && argc < max) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
    }
    return argc;
}

static bool starts_with(const char* str, const char* prefix)
{
    while (*prefix) {
        if (*str++ != *prefix++) {
            return false;
        }
    }
    return true;
}

static void shell_list(const char* path)
{
    int indices[FS_MAX_NODES];
    int count = fs_list(path, indices, FS_MAX_NODES);
    if (count < 0) {
        print_line("ls: path not found");
        return;
    }
    for (int i = 0; i < count; i++) {
        int idx = indices[i];
        print(fs_node_name(idx));
        if (fs_node_is_executable(idx)) {
            print("*");
        }
        print_line("");
    }
}

static void shell_cat(const char* path)
{
    int idx = fs_find_node(path);
    if (idx < 0) {
        print_line("cat: file not found");
        return;
    }
    if (fs_node_size(idx) == 0) {
        print_line("");
        return;
    }
    const uint8_t* data = fs_node_data(idx);
    if (!data) {
        print_line("cat: invalid file");
        return;
    }
    for (uint32_t i = 0; i < fs_node_size(idx); i++) {
        char c = (char)data[i];
        if (c == '\n') {
            print("\r\n");
        } else {
            char temp[2] = { c, 0 };
            print(temp);
        }
    }
}

static bool shell_change_dir(const char* path)
{
    return fs_set_cwd(path);
}

static bool shell_write_file(const char* path, const char* text)
{
    fs_write_file(path, (const uint8_t*)text, (uint32_t)strlen(text), FS_DATA);
    return true;
}

static bool shell_execute_command(char* line);

static bool shell_execute_binary(int idx)
{
    const uint8_t* data = fs_node_data(idx);
    uint32_t size = fs_node_size(idx);
    if (!data || size < 12) {
        print_line("exec: invalid binary");
        return false;
    }
    if (data[0] != 'S' || data[1] != 'N' || data[2] != 'U' || data[3] != 'B') {
        print_line("exec: unsupported binary format");
        return false;
    }
    uint32_t code_size = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];
    uint32_t data_size = (data[12] << 24) | (data[13] << 16) | (data[14] << 8) | data[15];
    const uint8_t* code = data + 16;
    const uint8_t* data_section = code + code_size;
    uint32_t ip = 0;
    while (ip < code_size) {
        uint8_t opcode = code[ip++];
        if (opcode == 0x01) {
            uint16_t off = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            uint16_t len = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            for (uint16_t j = 0; j < len && off + j < data_size; j++) {
                char c = (char)data_section[off + j];
                if (c == '\n') {
                    print("\r\n");
                } else {
                    char temp[2] = { c, 0 };
                    print(temp);
                }
            }
        } else if (opcode == 0x02) {
            uint16_t off = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            uint16_t len = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            char path[FS_MAX_PATH];
            uint32_t copy_len = len;
            if (copy_len >= FS_MAX_PATH) copy_len = FS_MAX_PATH - 1;
            for (uint32_t j = 0; j < copy_len && off + j < data_size; j++) {
                path[j] = (char)data_section[off + j];
            }
            path[copy_len] = 0;
            shell_execute_command(path);
        } else if (opcode == 0x03) {
            uint16_t path_off = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            uint16_t path_len = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            uint16_t data_off = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            uint16_t data_len = (uint16_t)((code[ip] << 8) | code[ip + 1]);
            ip += 2;
            char path[FS_MAX_PATH];
            uint32_t copy_len = path_len;
            if (copy_len >= FS_MAX_PATH) copy_len = FS_MAX_PATH - 1;
            for (uint32_t j = 0; j < copy_len && path_off + j < data_size; j++) {
                path[j] = (char)data_section[path_off + j];
            }
            path[copy_len] = 0;
            if (data_off + data_len <= data_size) {
                fs_write_file(path, data_section + data_off, data_len, FS_DATA);
            }
        } else if (opcode == 0xFF) {
            return true;
        } else {
            print_line("exec: unknown opcode");
            return false;
        }
    }
    return true;
}

static bool shell_execute_file(int idx, int argc, char* argv[])
{
    if (idx < 0) {
        return false;
    }
    if (fs_node_is_executable(idx)) {
        uint8_t file_type = fs_node_file_type(idx);
        if (file_type == FS_TOOL_GCC) {
            return handle_gcc(argc, argv);
        }
        if (file_type == FS_TOOL_FASM) {
            return handle_fasm(argc, argv);
        }
        if (file_type == FS_TOOL_SH) {
            return handle_sh(argc, argv);
        }
        if (file_type == FS_SCRIPT) {
            char script[256];
            uint32_t size = fs_node_size(idx);
            const uint8_t* data = fs_node_data(idx);
            uint32_t copy_len = size < sizeof(script) - 1 ? size : sizeof(script) - 1;
            for (uint32_t i = 0; i < copy_len; i++) {
                script[i] = (char)data[i];
            }
            script[copy_len] = 0;
            char* line = script;
            while (*line) {
                char* end = line;
                while (*end && *end != '\n' && *end != '\r') end++;
                char saved = *end;
                *end = 0;
                if (*line && *line != '#') {
                    shell_execute_command(line);
                }
                *end = saved;
                while (*end == '\n' || *end == '\r') end++;
                line = end;
            }
            return true;
        }
        const uint8_t* data = fs_node_data(idx);
        if (data && fs_node_size(idx) >= 16 && data[0] == 'S' && data[1] == 'N' && data[2] == 'U' && data[3] == 'B') {
            return shell_execute_binary(idx);
        }
    }
    print_line("exec: file is not executable");
    return false;
}

static bool shell_resolve_and_execute(const char* name, int argc, char* argv[])
{
    int idx = fs_find_node(name);
    if (idx >= 0) {
        return shell_execute_file(idx, argc, argv);
    }
    if (!starts_with(name, "/")) {
        char candidate[FS_MAX_PATH];
        int plen = 0;
        const char* p = "/sys/bin/";
        while (*p && plen < (int)sizeof(candidate) - 1) {
            candidate[plen++] = *p++;
        }
        for (const char* q = name; *q && plen < (int)sizeof(candidate) - 1; q++) {
            candidate[plen++] = *q;
        }
        candidate[plen] = 0;
        idx = fs_find_node(candidate);
        if (idx >= 0) {
            return shell_execute_file(idx, argc, argv);
        }
    }
    print_line("command not found");
    return false;
}

static bool compile_fasm_source(const char* source, const uint8_t* data, uint32_t size, uint8_t* out, uint32_t* out_size)
{
    (void)data;
    (void)size;
    uint8_t code[1024];
    uint8_t data_area[1024];
    uint32_t code_pos = 0;
    uint32_t data_pos = 0;
    const char* p = source;
    char line[FS_MAX_PATH];

    while (*p) {
        uint32_t len = 0;
        while (*p && *p != '\n' && len + 1 < sizeof(line)) {
            line[len++] = *p++;
        }
        if (*p == '\n') p++;
        while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
            len--;
        }
        line[len] = 0;
        if (len == 0 || line[0] == '#') continue;
        if (starts_with(line, "PRINT ")) {
            const char* text = line + 6;
            if (*text == '"') {
                text++;
                char string[FS_MAX_PATH];
                uint32_t out_len = 0;
                while (*text && *text != '"' && out_len + 1 < sizeof(string)) {
                    string[out_len++] = *text++;
                }
                string[out_len] = 0;
                if (data_pos + out_len > sizeof(data_area)) return false;
                uint16_t data_offset = (uint16_t)data_pos;
                for (uint32_t i = 0; i < out_len; i++) {
                    data_area[data_pos++] = (uint8_t)string[i];
                }
                code[code_pos++] = 0x01;
                code[code_pos++] = (data_offset >> 8) & 0xFF;
                code[code_pos++] = data_offset & 0xFF;
                code[code_pos++] = (out_len >> 8) & 0xFF;
                code[code_pos++] = out_len & 0xFF;
            }
        } else if (starts_with(line, "RUN ")) {
            const char* path = line + 4;
            while (*path == ' ') path++;
            if (*path) {
                uint32_t out_len = 0;
                uint16_t data_offset = (uint16_t)data_pos;
                while (*path && out_len + 1 < sizeof(data_area) && *path != ' ' && *path != '\r') {
                    data_area[data_pos++] = (uint8_t)*path++;
                    out_len++;
                }
                if (data_pos + 1 > sizeof(data_area)) return false;
                code[code_pos++] = 0x02;
                code[code_pos++] = (data_offset >> 8) & 0xFF;
                code[code_pos++] = data_offset & 0xFF;
                code[code_pos++] = (out_len >> 8) & 0xFF;
                code[code_pos++] = out_len & 0xFF;
            }
        } else if (starts_with(line, "WRITE ")) {
            const char* p2 = line + 6;
            while (*p2 == ' ') p2++;
            char target[FS_MAX_PATH];
            uint32_t target_len = 0;
            while (*p2 && *p2 != ' ' && target_len + 1 < sizeof(target)) {
                target[target_len++] = *p2++;
            }
            target[target_len] = 0;
            while (*p2 == ' ') p2++;
            if (*p2 == '"') {
                p2++;
                char content[FS_MAX_PATH];
                uint32_t content_len = 0;
                while (*p2 && *p2 != '"' && content_len + 1 < sizeof(content)) {
                    content[content_len++] = *p2++;
                }
                content[content_len] = 0;
                if (data_pos + target_len + content_len > sizeof(data_area)) return false;
                uint16_t target_offset = (uint16_t)data_pos;
                for (uint32_t i = 0; i < target_len; i++) {
                    data_area[data_pos++] = (uint8_t)target[i];
                }
                uint16_t content_offset = (uint16_t)data_pos;
                for (uint32_t i = 0; i < content_len; i++) {
                    data_area[data_pos++] = (uint8_t)content[i];
                }
                code[code_pos++] = 0x03;
                code[code_pos++] = (target_offset >> 8) & 0xFF;
                code[code_pos++] = target_offset & 0xFF;
                code[code_pos++] = (target_len >> 8) & 0xFF;
                code[code_pos++] = target_len & 0xFF;
                code[code_pos++] = (content_offset >> 8) & 0xFF;
                code[code_pos++] = content_offset & 0xFF;
                code[code_pos++] = (content_len >> 8) & 0xFF;
                code[code_pos++] = content_len & 0xFF;
            }
        } else if (starts_with(line, "EXIT")) {
            code[code_pos++] = 0xFF;
        }
    }
    if (code_pos + data_pos + 16 > *out_size) {
        return false;
    }
    out[0] = 'S'; out[1] = 'N'; out[2] = 'U'; out[3] = 'B';
    out[4] = 1;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = (code_pos >> 24) & 0xFF;
    out[9] = (code_pos >> 16) & 0xFF;
    out[10] = (code_pos >> 8) & 0xFF;
    out[11] = code_pos & 0xFF;
    out[12] = (data_pos >> 24) & 0xFF;
    out[13] = (data_pos >> 16) & 0xFF;
    out[14] = (data_pos >> 8) & 0xFF;
    out[15] = data_pos & 0xFF;
    for (uint32_t i = 0; i < code_pos; i++) out[16 + i] = code[i];
    for (uint32_t i = 0; i < data_pos; i++) out[16 + code_pos + i] = data_area[i];
    *out_size = 16 + code_pos + data_pos;
    return true;
}

static bool compile_gcc_source(const char* source, const uint8_t* data, uint32_t size, uint8_t* out, uint32_t* out_size)
{
    (void)data;
    (void)size;
    uint8_t code[1024];
    uint8_t data_area[1024];
    uint32_t code_pos = 0;
    uint32_t data_pos = 0;
    const char* p = source;
    bool in_main = false;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (starts_with(p, "int main")) {
            in_main = true;
            while (*p && *p != '{') p++;
            if (*p == '{') p++;
            continue;
        }
        if (!in_main) {
            p++;
            continue;
        }
        if (starts_with(p, "puts(")) {
            p += 5;
            if (*p == '"') {
                p++;
                uint16_t start = data_pos;
                while (*p && *p != '"' && data_pos + 1 < sizeof(data_area)) {
                    data_area[data_pos++] = (uint8_t)*p++;
                }
                uint16_t length = data_pos - start;
                if (*p == '"') p++;
                while (*p && *p != ';') p++;
                if (*p == ';') p++;
                code[code_pos++] = 0x01;
                code[code_pos++] = (start >> 8) & 0xFF;
                code[code_pos++] = start & 0xFF;
                code[code_pos++] = (length >> 8) & 0xFF;
                code[code_pos++] = length & 0xFF;
            }
            continue;
        }
        if (starts_with(p, "run(")) {
            p += 4;
            while (*p == ' ') p++;
            if (*p == '"') {
                p++;
                uint16_t start = data_pos;
                uint16_t length = 0;
                while (*p && *p != '"' && data_pos + 1 < sizeof(data_area)) {
                    data_area[data_pos++] = (uint8_t)*p++;
                    length++;
                }
                if (*p == '"') p++;
                while (*p && *p != ';') p++;
                if (*p == ';') p++;
                code[code_pos++] = 0x02;
                code[code_pos++] = (start >> 8) & 0xFF;
                code[code_pos++] = start & 0xFF;
                code[code_pos++] = (length >> 8) & 0xFF;
                code[code_pos++] = length & 0xFF;
            }
            continue;
        }
        if (starts_with(p, "return")) {
            while (*p && *p != ';') p++;
            if (*p == ';') p++;
            continue;
        }
        if (*p == '}') break;
        p++;
    }
    code[code_pos++] = 0xFF;
    if (16 + code_pos + data_pos > *out_size) return false;
    out[0] = 'S'; out[1] = 'N'; out[2] = 'U'; out[3] = 'B';
    out[4] = 1;
    out[5] = 0;
    out[6] = 0;
    out[7] = 0;
    out[8] = (code_pos >> 24) & 0xFF;
    out[9] = (code_pos >> 16) & 0xFF;
    out[10] = (code_pos >> 8) & 0xFF;
    out[11] = code_pos & 0xFF;
    out[12] = (data_pos >> 24) & 0xFF;
    out[13] = (data_pos >> 16) & 0xFF;
    out[14] = (data_pos >> 8) & 0xFF;
    out[15] = data_pos & 0xFF;
    for (uint32_t i = 0; i < code_pos; i++) out[16 + i] = code[i];
    for (uint32_t i = 0; i < data_pos; i++) out[16 + code_pos + i] = data_area[i];
    *out_size = 16 + code_pos + data_pos;
    return true;
}

static bool handle_gcc(int argc, char* argv[])
{
    if (argc < 4) {
        print_line("gcc: usage gcc -o <output> <source>");
        return false;
    }
    const char* output = NULL;
    const char* source = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (!source) {
            source = argv[i];
        }
    }
    if (!output || !source) {
        print_line("gcc: missing output or source");
        return false;
    }
    int src_idx = fs_find_node(source);
    if (src_idx < 0) {
        print_line("gcc: source file not found");
        return false;
    }
    uint32_t size = fs_node_size(src_idx);
    const uint8_t* src_data = fs_node_data(src_idx);
    uint8_t buffer[2048];
    uint32_t out_size = sizeof(buffer);
    if (!compile_gcc_source((const char*)src_data, src_data, size, buffer, &out_size)) {
        print_line("gcc: failed to compile source");
        return false;
    }
    fs_write_file(output, buffer, out_size, FS_BIN);
    print_line("gcc: compiled to output");
    return true;
}

static bool handle_fasm(int argc, char* argv[])
{
    if (argc < 4) {
        print_line("fasm: usage fasm -o <output> <source>");
        return false;
    }
    const char* output = NULL;
    const char* source = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (!source) {
            source = argv[i];
        }
    }
    if (!output || !source) {
        print_line("fasm: missing output or source");
        return false;
    }
    int src_idx = fs_find_node(source);
    if (src_idx < 0) {
        print_line("fasm: source file not found");
        return false;
    }
    uint32_t size = fs_node_size(src_idx);
    const uint8_t* src_data = fs_node_data(src_idx);
    uint8_t buffer[2048];
    uint32_t out_size = sizeof(buffer);
    if (!compile_fasm_source((const char*)src_data, src_data, size, buffer, &out_size)) {
        print_line("fasm: failed to assemble source");
        return false;
    }
    fs_write_file(output, buffer, out_size, FS_BIN);
    print_line("fasm: assembled to output");
    return true;
}

static bool handle_sh(int argc, char* argv[])
{
    if (argc < 2) {
        print_line("sh: usage sh <script>");
        return false;
    }
    int idx = fs_find_node(argv[1]);
    if (idx < 0) {
        print_line("sh: script not found");
        return false;
    }
    const uint8_t* data = fs_node_data(idx);
    uint32_t size = fs_node_size(idx);
    if (!data) {
        print_line("sh: invalid script");
        return false;
    }
    char script[256];
    uint32_t copy_len = size < sizeof(script) - 1 ? size : sizeof(script) - 1;
    for (uint32_t i = 0; i < copy_len; i++) {
        script[i] = (char)data[i];
    }
    script[copy_len] = 0;
    char* line = script;
    while (*line) {
        char* end = line;
        while (*end && *end != '\n' && *end != '\r') end++;
        char saved = *end;
        *end = 0;
        if (*line && *line != '#') {
            shell_execute_command(line);
        }
        *end = saved;
        while (*end == '\n' || *end == '\r') end++;
        line = end;
    }
    return true;
}

static bool shell_execute_command(char* line)
{
    char* argv[16];
    int argc = tokenize(line, argv, 16);
    if (argc == 0) return false;
    if (strcmp(argv[0], "help") == 0) {
        print_line("Built-in commands: ls cd pwd cat mkdir echo run gcc fasm sh help exit");
        return true;
    }
    if (strcmp(argv[0], "exit") == 0) {
        print_line("Exiting shell.");
        for (;;) asm volatile("hlt");
    }
    if (strcmp(argv[0], "pwd") == 0) {
        char cwd[FS_MAX_PATH];
        if (fs_get_cwd_path(cwd, sizeof(cwd))) {
            print_line(cwd);
        } else {
            print_line("/");
        }
        return true;
    }
    if (strcmp(argv[0], "ls") == 0) {
        if (argc == 1) {
            shell_list(".");
        } else {
            shell_list(argv[1]);
        }
        return true;
    }
    if (strcmp(argv[0], "cd") == 0) {
        if (argc < 2) {
            print_line("cd: missing path");
        } else if (!shell_change_dir(argv[1])) {
            print_line("cd: path not found");
        }
        return true;
    }
    if (strcmp(argv[0], "cat") == 0) {
        if (argc < 2) {
            print_line("cat: missing file");
        } else {
            shell_cat(argv[1]);
        }
        return true;
    }
    if (strcmp(argv[0], "mkdir") == 0) {
        if (argc < 2) {
            print_line("mkdir: missing path");
        } else {
            if (fs_make_dir(argv[1])) {
                print_line("directory created");
            } else {
                print_line("mkdir: failed");
            }
        }
        return true;
    }
    if (strcmp(argv[0], "echo") == 0) {
        if (argc >= 4 && strcmp(argv[argc - 2], ">") == 0) {
            int len = 0;
            for (int i = 1; i < argc - 2; i++) {
                len += strlen(argv[i]) + 1;
            }
            char buffer[256];
            int pos = 0;
            for (int i = 1; i < argc - 2; i++) {
                if (i > 1) buffer[pos++] = ' ';
                for (const char* p = argv[i]; *p && pos < (int)sizeof(buffer) - 1; p++) {
                    buffer[pos++] = *p;
                }
            }
            buffer[pos] = 0;
            shell_write_file(argv[argc - 1], buffer);
            return true;
        }
        for (int i = 1; i < argc; i++) {
            if (i > 1) {
                print(" ");
            }
            print(argv[i]);
        }
        print_line("");
        return true;
    }
    if (strcmp(argv[0], "run") == 0) {
        if (argc < 2) {
            print_line("run: missing executable");
        } else {
            int idx = fs_find_node(argv[1]);
            if (idx < 0) {
                print_line("run: executable not found");
            } else {
                if (!shell_execute_binary(idx)) {
                    print_line("run: failed to execute");
                }
            }
        }
        return true;
    }
    if (strcmp(argv[0], "gcc") == 0) {
        return handle_gcc(argc, argv);
    }
    if (strcmp(argv[0], "fasm") == 0) {
        return handle_fasm(argc, argv);
    }
    if (strcmp(argv[0], "sh") == 0) {
        return handle_sh(argc, argv);
    }
    return shell_resolve_and_execute(argv[0], argc, argv);
}

bool shell_run_init_service()
{
    int idx = fs_find_node("/sys/init.service");
    if (idx < 0) {
        return false;
    }
    const uint8_t* data = fs_node_data(idx);
    uint32_t size = fs_node_size(idx);
    if (!data) {
        return false;
    }
    char command[FS_MAX_PATH];
    command[0] = 0;
    char line[FS_MAX_PATH];
    uint32_t pos = 0;
    for (uint32_t i = 0; i <= size; i++) {
        char c = i < size ? (char)data[i] : '\n';
        if (c == '\r') continue;
        if (c == '\n' || i == size) {
            line[pos] = 0;
            if (starts_with(line, "ExecStart=")) {
                int j = 10;
                int out = 0;
                while (line[j] && out < FS_MAX_PATH - 1) {
                    command[out++] = line[j++];
                }
                command[out] = 0;
                break;
            }
            pos = 0;
            continue;
        }
        if (pos + 1 < sizeof(line)) {
            line[pos++] = c;
        }
    }
    if (!command[0]) {
        return false;
    }
    print_line("Starting init service...");
    shell_execute_command(command);
    return true;
}

void shell_main()
{
    print_line("SNU userland shell initialized.");
    if (!shell_run_init_service()) {
        print_line("No init.service found, starting interactive shell.");
    }
    while (true) {
        prompt();
        char line[128];
        read_line(line, sizeof(line));
        shell_execute_command(line);
    }
}
