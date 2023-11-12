#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "vm/vm.h"

static void parse_prompt();
static void parse_file(const char *path);

int main(int argc, const char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [path]\n", argv[0]);
        // 64 stands for command line usage error
        exit(64);
    } else if (argc == 2) parse_file(argv[1]);
    else parse_prompt();
    exit(0);
}

static void parse_prompt() {
    char line[1024];
    for (;;) {
        fprintf(stdout, "> ");
        if (fgets(line, sizeof(line), stdin) != NULL) interpret(line);
        else {
            // ctrl + D =>  EOF
            fprintf(stdout, "\n");
            break;
        }
    }
}

static void parse_file(const char *path) {
    // open in binary mode
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        // 66 stands for input file is not exist or readable
        exit(66);
    }

    // set file position to end of file
    fseek(file, 0L, SEEK_END);
    // acquire current file position
    long size = ftell(file);
    // set file position to start of file
    fseek(file, 0L, SEEK_SET);

    // +1 for null terminator
    char *content = (char*)malloc(size + 1);
    if (content == NULL) {
        fclose(file);
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        // 71 stands for system error => out of memory
        exit(71);
    }

    // load file into memory
    size_t read_bytes = fread(content, sizeof(char), size, file);
    content[read_bytes] = '\0';
    if (read_bytes != size) {
        fclose(file);
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        // 74 stands for input/output error
        exit(74);
    }
    
    fclose(file);

    InterpreterResult rst = interpret(content);
    free(content);

    // 65 stands for data format error
    if (rst == INTERPRET_COMPLIE_ERROR) exit(65);
    // 70 stands for software error => in this case, user lox program error
    if (rst == INTERPRET_RUNTIME_ERROR) exit(70);
}