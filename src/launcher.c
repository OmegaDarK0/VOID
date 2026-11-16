#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void extract_target_name(const char *exe_path, char *out_name, const size_t out_size) {
    if (!out_name || out_size == 0) return;
    const char *base = exe_path;
    for (const char *p = exe_path; *p; ++p) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    size_t len = strlen(base);
    if (len >= out_size) len = out_size - 1;
    memcpy(out_name, base, len);
    out_name[len] = '\0';
    if (len > 4 && strcmp(out_name + len - 4, ".exe") == 0) {
        out_name[len - 4] = '\0';
        len -= 4;
    }
    const char *suffix = "_Launcher";
    const size_t suffix_len = strlen(suffix);
    if (len > suffix_len && strcmp(out_name + len - suffix_len, suffix) == 0) {
        out_name[len - suffix_len] = '\0';
    }
}

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>

int main(void) {
    char root_dir[MAX_PATH];
    const DWORD len = GetModuleFileNameA(NULL, root_dir, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return 1;
    }
    char target_name[256];
    extract_target_name(root_dir, target_name, sizeof(target_name));
    char *last_sep = strrchr(root_dir, '\\');
    if (!last_sep) last_sep = strrchr(root_dir, '/');
    if (last_sep) *last_sep = '\0';
    char target_exe[MAX_PATH + sizeof(target_name) + 16];
    const int n = snprintf(target_exe, sizeof(target_exe), "%s\\bin\\%s.exe", root_dir, target_name);
    if (n < 0 || (size_t)n >= sizeof(target_exe)) {
        return 1;
    }
    const char *raw_cmd = GetCommandLineA();
    const char *args = raw_cmd;
    if (*args == '"') {
        args++;
        while (*args && *args != '"') args++;
        if (*args == '"') args++;
    } else {
        while (*args && *args != ' ' && *args != '\t') args++;
    }
    while (*args == ' ' || *args == '\t') args++;
    const size_t cmd_len = strlen(target_exe) + strlen(args) + 4;
    char *cmd_line = (char *)malloc(cmd_len);
    if (!cmd_line) return 1;
    if (*args) {
        snprintf(cmd_line, cmd_len, "\"%s\" %s", target_exe, args);
    } else {
        snprintf(cmd_line, cmd_len, "\"%s\"", target_exe);
    }
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    if (!CreateProcessA(target_exe, cmd_line, NULL, NULL, FALSE, 0, NULL, root_dir, &si, &pi)) {
        char err_msg[sizeof(target_exe) + 32];
        char err_title[300];
        snprintf(err_msg, sizeof(err_msg), "Failed to launch:\n%s", target_exe);
        snprintf(err_title, sizeof(err_title), "%s Launcher", target_name);
        MessageBoxA(NULL, err_msg, err_title, MB_OK | MB_ICONERROR);
        free(cmd_line);
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    free(cmd_line);
    return (int)exit_code;
}

#else
    #include <limits.h>
    #include <unistd.h>

int main(const int argc, char *argv[]) {
    (void)argc;
    char root_dir[PATH_MAX];
    const ssize_t len = readlink("/proc/self/exe", root_dir, sizeof(root_dir) - 1);
    if (len <= 0) {
        perror("readlink failed");
        return 1;
    }
    root_dir[len] = '\0';
    char target_name[256];
    extract_target_name(root_dir, target_name, sizeof(target_name));
    char *last_sep = strrchr(root_dir, '/');
    if (last_sep) *last_sep = '\0';
    if (chdir(root_dir) != 0) {
        perror("chdir failed");
        return 1;
    }
    char target_exe[PATH_MAX + sizeof(target_name) + 16];
    const int n = snprintf(target_exe, sizeof(target_exe), "%s/bin/%s", root_dir, target_name);
    if (n < 0 || (size_t)n >= sizeof(target_exe)) {
        return 1;
    }
    argv[0] = target_exe;
    execv(target_exe, argv);
    perror(target_exe);
    return 1;
}
#endif