#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bee_emitter.h"

#ifdef _WIN32
    #include <windows.h>
    #define popen _popen
    #define pclose _pclose
#else
    #include <unistd.h>
#endif

static char cc_path[64] = "gcc";
static int is_msvc = 0;

char* detect_compiler() {
    #ifdef _WIN32
        if (system("where gcc >nul 2>&1") == 0)      { strcpy(cc_path, "gcc");      is_msvc = 0; return cc_path; }
        if (system("where clang >nul 2>&1") == 0)    { strcpy(cc_path, "clang");     is_msvc = 0; return cc_path; }
        if (system("where cl >nul 2>&1") == 0)       { strcpy(cc_path, "cl");        is_msvc = 1; return cc_path; }
    #else
        if (system("command -v gcc >/dev/null 2>&1") == 0)     { strcpy(cc_path, "gcc");      is_msvc = 0; return cc_path; }
        if (system("command -v clang >/dev/null 2>&1") == 0)   { strcpy(cc_path, "clang");     is_msvc = 0; return cc_path; }
    #endif
    return NULL;
}

char* compile_c(const char* c_code, const char* out_name, const char* cc, const char* flags, int verbose) {
    
    if (!c_code || !*c_code || !out_name || !*out_name) {
        return strdup("Error: Mismatch.");
    }

    
    if (cc && *cc) {
        strncpy(cc_path, cc, sizeof(cc_path)-1);
        cc_path[sizeof(cc_path)-1] = '\0';
        is_msvc = (strstr(cc_path, "cl") != NULL);
    } else {
        if (!detect_compiler()) {
            return strdup("Error: Cannot find compiler.");
        }
    }

    
    char cmd[512];
    const char* def_flags = is_msvc ? "/std:c17 /O2 /W3" : "-std=c17 -O2 -Wall";
    const char* use_flags = flags ? flags : def_flags;

    if (is_msvc) {
        snprintf(cmd, sizeof(cmd), "%s %s /TC /Fe:\"%s\" - 2>&1", cc_path, use_flags, out_name);
    } else {
        snprintf(cmd, sizeof(cmd), "%s %s -xc - -o \"%s\" 2>&1", cc_path, use_flags, out_name);
    }

    if (verbose) printf("Lệnh biên dịch: %s\n", cmd);

    
    FILE* proc = popen(cmd, "w+");
    if (!proc) return strdup("Error: Cannot use compiler.");

    
    fputs(c_code, proc);
    fflush(proc); 

    
    static char err_buf[2048];
    err_buf[0] = '\0';
    char buf[512];
    int dong = 0;
    while (dong < 10 && fgets(buf, sizeof(buf), proc)) {
        strncat(err_buf, buf, sizeof(err_buf) - strlen(err_buf) - 1);
        dong++;
    }

    
    int exit_code = pclose(proc);

    
    if (exit_code == 0) {
        return NULL;
    } else {
        if (strlen(err_buf) == 0) {
            return strdup("Error: Compile Fail.");
        }
        return strdup(err_buf);
    }
}