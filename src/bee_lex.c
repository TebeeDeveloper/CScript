#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bee_lex.h"

// 1. XÓA TOÀN BỘ chú thích -- và cắt khoảng trắng cuối dòng
void trans_comment(char* line) {
    char* p = strstr(line, "--");
    if (p) *p = '\0';

    int len = strlen(line);
    while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\t' || line[len-1] == '\r'))
        line[--len] = '\0';
}

// 2. Dịch khai báo biến: let ten kieu = gia tri → kieu ten = gia tri
char* trans_let(char* line) {
    char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "let ", 4) != 0) return NULL;

    p += 4;
    while (*p == ' ' || *p == '\t') p++;

    char ten[128], kieu[128], gia_tri[512];
    int so_phan = sscanf(p, "%127s %127s = %511[^\n;]", ten, kieu, gia_tri);

    char ket_qua[1024];
    if (so_phan == 3) {
        snprintf(ket_qua, sizeof(ket_qua), "%s %s = %s", kieu, ten, gia_tri);
    } else if (so_phan == 2) {
        snprintf(ket_qua, sizeof(ket_qua), "%s %s", kieu, ten);
    } else {
        return strdup(line);
    }

    return strdup(ket_qua);
}

// 3. Dịch hàm main: fn main() { → int main() {
char* trans_main(char* line) {
    char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "fn main()", 8) == 0) {
        return strdup("int main() {");
    }
    return NULL;
}

// 4. Thêm dấu ; cho các dòng cần thiết, trừ #include và ngoặc { }
char* trans_semicolon(char* line) {
    if (!line || !*line) return strdup("");

    char* p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '\0') return strdup("");

    // KHÔNG thêm ; cho các dòng cấu trúc
    if (*p == '{' || *p == '}' ||
        strncmp(p, "#include", 8) == 0 ||
        strncmp(p, "fn ", 3) == 0 ||
        strncmp(p, "if ", 3) == 0 ||
        strncmp(p, "else", 4) == 0 ||
        strncmp(p, "for ", 4) == 0 ||
        strncmp(p, "while ", 6) == 0 ||
        strncmp(p, "extern ", 7) == 0) {
        return strdup(line);
    }

    // Giữ nguyên nếu đã có ;
    size_t len = strlen(line);
    if (len > 0 && line[len-1] == ';')
        return strdup(line);

    char* res = malloc(len + 2);
    if (!res) return NULL;
    strncpy(res, line, len);
    res[len] = ';';
    res[len+1] = '\0';
    return res;
}

#define BUF_SIZE 131072

int lex(char* code, char* output, const char* filename) {
    if (!code || !*code || !output || !filename) return -1;

    // ==== Thư viện chuẩn đầy đủ ====
    const char* lib_headers =
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <stdbool.h>\n"
        "#include <stdarg.h>\n"
        "#include <stddef.h>\n"
        "#include <ctype.h>\n"
        "#include <math.h>\n\n";

    char sharpline[512];
    snprintf(sharpline, sizeof(sharpline), "#line 1 \"%s\"\n\n", filename);

    char buf[BUF_SIZE] = {0};
    char line[1024];
    char* src = code;
    int found_main = 0;
    int has_closing_brace = 0;

    while (*src) {
        int i = 0;
        while (*src && *src != '\n' && i < sizeof(line)-1)
            line[i++] = *src++;
        line[i] = '\0';
        if (*src == '\n') src++;

        trans_comment(line);
        char* p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') continue;

        char* converted = NULL;

        converted = trans_main(line);
        if (converted) {
            found_main = 1;
        } else {
            converted = trans_let(line);
            if (converted) {
                char* tmp = converted;
                converted = trans_semicolon(tmp);
                free(tmp);
            } else {
                converted = trans_semicolon(line);
            }
        }

        if (converted) {
            strncat(buf, converted, BUF_SIZE - strlen(buf) - 1);
            strncat(buf, "\n", BUF_SIZE - strlen(buf) - 1);
            free(converted);
            if (strcmp(p, "}") == 0) has_closing_brace = 1;
        }
    }

    if (!found_main) {
        fprintf(stderr, "❌ Thiếu hàm fn main()!\n");
        return -1;
    }

    if (!has_closing_brace) {
        strncat(buf, "}\n", BUF_SIZE - strlen(buf) - 1);
    }

    // ==== Ghép đúng thứ tự: Thư viện chuẩn → #line → Nội dung file nguồn ====
    snprintf(output, BUF_SIZE, "%s%s%s", lib_headers, sharpline, buf);
    printf("%s", output);

    return 0;
}