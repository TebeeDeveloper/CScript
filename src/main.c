#ifndef CScript
#define CScript

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CS_LEX_H
    #include "cs_lex.h"
#endif

#ifndef CS_EMITTER_H
    #include "cs_emitter.h"
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
    if (argc < 3) {
        printf("How to use CScript: %s input.csr output [flags...]\n", argv[0]);
        printf("Example:\n");
        printf("  %s main.csr app\n", argv[0]);
        printf("  %s main.csr libm.so -shared\n", argv[0]);
        printf("  %s main.csr calc -lm -lpthread\n", argv[0]);
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


    char* bee_code = breadfile(input_path);
    if (!bee_code) {
        printf("Error: Cannot open file '%s'\n", input_path);
        return 2;
    }

    char c_code[65536] = {0};
    if (lex(bee_code, c_code, input_path) != 0) {
        free(bee_code);
        return 3;
    }
    free(bee_code);

    char* loi = compile_c(c_code, output_path, NULL, (flags_buf[0] != '\0') ? flags_buf : NULL, 1);
    if (loi) {
        printf("Compile Error:\n%s\n", loi);
        free(loi);
        return 4;
    }

    printf("Successful: '%s'\n", output_path);
    return 0;
}

#endif