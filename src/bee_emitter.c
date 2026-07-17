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
    printf("Lỗi: Không tìm thấy trình biên dịch C!\n");
    return NULL;
}

// Trả về NULL = thành công, chuỗi khác = nội dung lỗi
char* compile_c(const char* c_code, const char* out_name, const char* cc, const char* flags, int verbose) {
    // Kiểm tra tham số đầu vào
    if (!c_code || !*c_code || !out_name || !*out_name) {
        return strdup("Lỗi: Tham số đầu vào không hợp lệ");
    }

    // Chọn trình biên dịch
    if (cc && *cc) {
        strncpy(cc_path, cc, sizeof(cc_path)-1);
        cc_path[sizeof(cc_path)-1] = '\0';
        is_msvc = (strstr(cc_path, "cl") != NULL);
    } else {
        if (!detect_compiler()) {
            return strdup("Lỗi: Không tìm thấy trình biên dịch phù hợp");
        }
    }

    // Tạo lệnh biên dịch + chuyển stderr sang stdout để đọc lỗi chung
    char cmd[512];
    const char* def_flags = is_msvc ? "/std:c17 /O2 /W3" : "-std=c17 -O2 -Wall";
    const char* use_flags = flags ? flags : def_flags;

    if (is_msvc) {
        snprintf(cmd, sizeof(cmd), "%s %s /TC /Fe:\"%s\" - 2>&1", cc_path, use_flags, out_name);
    } else {
        snprintf(cmd, sizeof(cmd), "%s %s -xc - -o \"%s\" 2>&1", cc_path, use_flags, out_name);
    }

    if (verbose) printf("Lệnh biên dịch: %s\n", cmd);

    // Mở tiến trình: 1 lần duy nhất để ghi mã + đọc kết quả/lỗi
    FILE* proc = popen(cmd, "w+");
    if (!proc) return strdup("Lỗi: Không khởi động được trình biên dịch");

    // Gửi mã C vào trình biên dịch
    fputs(c_code, proc);
    fflush(proc); // Đẩy hết dữ liệu đi trước khi đọc

    // Đọc toàn bộ kết quả/lỗi
    static char err_buf[2048];
    err_buf[0] = '\0';
    char buf[512];
    int dong = 0;
    while (dong < 10 && fgets(buf, sizeof(buf), proc)) {
        strncat(err_buf, buf, sizeof(err_buf) - strlen(err_buf) - 1);
        dong++;
    }

    // Đóng tiến trình và lấy mã thoát
    int exit_code = pclose(proc);

    // Nếu mã thoát = 0 → thành công, ngược lại trả về lỗi
    if (exit_code == 0) {
        return NULL;
    } else {
        if (strlen(err_buf) == 0) {
            return strdup("Lỗi: Biên dịch thất bại nhưng không có thông báo chi tiết");
        }
        return strdup(err_buf);
    }
}