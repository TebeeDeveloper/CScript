#ifndef BeeScript
#define BeeScript

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BEE_LEX_H
    #include "bee_lex.h"
#endif

#ifndef BEE_EMITTER_H
    #include "bee_emitter.h"
#endif

char* breadfile(const char* file_name) {
    if (!file_name || !*file_name) return NULL;
    FILE* f = fopen(file_name, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char* buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    long read = fread(buf, 1, size, f);
    buf[read] = '\0';

    fclose(f);
    return buf;
}

int main(int argc, char* argv[]) {
    // Cách dùng đầy đủ: bee <tệp_nhập.bee> <tệp_đầu_ra> [cờ_biên_dịch_tùy_chọn]
    if (argc < 3) {
        printf("Cách dùng: %s input.bee output [cờ...]\n", argv[0]);
        printf("Ví dụ:\n");
        printf("  %s main.bee app               → Tạo tệp thực thi app\n", argv[0]);
        printf("  %s main.bee libm.so -shared   → Tạo thư viện động libm.so\n", argv[0]);
        printf("  %s main.bee calc -lm -lpthread → Liên kết thư viện toán + luồng\n", argv[0]);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];
    char flags_buf[1024] = {0};
    if (argc >= 4) {
        for (int i = 3; i < argc; i++) {
            strncat(flags_buf, argv[i], sizeof(flags_buf) - strlen(flags_buf) - 2);
            strncat(flags_buf, " ", sizeof(flags_buf) - strlen(flags_buf) - 1);
        }
        size_t len = strlen(flags_buf);
        if (len > 0) flags_buf[len - 1] = '\0';
    }


    // 1. Đọc tệp BeeScript
    char* bee_code = breadfile(input_path);
    if (!bee_code) {
        printf("❌ Lỗi: Không mở hoặc đọc được tệp '%s'\n", input_path);
        return 2;
    }

    // 2. Dịch sang mã C
    char c_code[65536] = {0};
    if (lex(bee_code, c_code, input_path) != 0) {
        free(bee_code);
        return 3;
    }
    free(bee_code);

    // 3. Biên dịch với cờ người dùng cung cấp
    char* loi = compile_c(c_code, output_path, NULL, (flags_buf[0] != '\0') ? flags_buf : NULL, 1);
    if (loi) {
        printf("❌ Lỗi biên dịch:\n%s\n", loi);
        free(loi);
        return 4;
    }

    printf("✅ Hoàn tất! Tạo thành công: '%s'\n", output_path);
    return 0;
}

#endif