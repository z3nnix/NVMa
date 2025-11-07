#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Syscall table
typedef struct {
    const char *name;
    int number;
} Syscall;

Syscall syscalls[] = {
    {"exit", 0x00},
    {"exec", 0x01},
    {"read", 0x02},
    {"write", 0x03},
    {"create", 0x04},
    {"delete", 0x05},
    {"cap_request", 0x06},
    {"cap_spawn", 0x07},
    {"drv_call", 0x08},
    {"msg_send", 0x09},
    {"msg_recieve", 0x0A},
    {NULL, 0}
};

int find_syscall(const char *name) {
    for (int i = 0; syscalls[i].name != NULL; i++) {
        if (strcmp(syscalls[i].name, name) == 0) {
            return syscalls[i].number;
        }
    }
    return -1;
}

int parse_number(const char *str) {
    if (strncmp(str, "0x", 2) == 0) {
        return (int)strtol(str + 2, NULL, 16);
    } else {
        return atoi(str);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s input.asm output.bin\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "wb");
    
    if (!input || !output) {
        printf("Error opening files\n");
        return 1;
    }

    // Write signature
    fputc(0x4E, output); // 'N'
    fputc(0x56, output); // 'V'
    fputc(0x4D, output); // 'M'
    fputc(0x30, output); // '0'

    char line[256];
    
    while (fgets(line, sizeof(line), input)) {
        // Remove comments
        char *comment = strchr(line, ';');
        if (comment) *comment = '\0';
        
        // Trim whitespace
        char *start = line;
        while (isspace(*start)) start++;
        
        char *end = start + strlen(start) - 1;
        while (end > start && isspace(*end)) end--;
        end[1] = '\0';
        
        if (strlen(start) == 0) continue;
        
        // Tokenize
        char *tokens[4];
        int token_count = 0;
        char *token = strtok(start, " \t,");
        
        while (token && token_count < 4) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t,");
        }
        
        // Process instructions
        if (strcasecmp(tokens[0], ".NVM0") == 0) {
            // Already handled - signature
        }
        else if (strcasecmp(tokens[0], "hlt") == 0 || strcasecmp(tokens[0], "halt") == 0) {
            fputc(0x00, output);
        }
        else if (strcasecmp(tokens[0], "nop") == 0) {
            fputc(0x01, output);
        }
        else if (strcasecmp(tokens[0], "push") == 0 && token_count >= 3) {
            if (strcasecmp(tokens[1], "byte") == 0) {
                fputc(0x02, output);
                fputc(parse_number(tokens[2]) & 0xFF, output);
            }
            else if (strcasecmp(tokens[1], "short") == 0) {
                fputc(0x03, output);
                int value = parse_number(tokens[2]);
                fputc(value & 0xFF, output);
                fputc((value >> 8) & 0xFF, output);
            }
        }
        else if (strcasecmp(tokens[0], "pop") == 0) {
            fputc(0x04, output);
        }
        else if (strcasecmp(tokens[0], "dup") == 0) {
            fputc(0x05, output);
        }
        else if (strcasecmp(tokens[0], "swap") == 0) {
            fputc(0x06, output);
        }
        // Arithmetic
        else if (strcasecmp(tokens[0], "add") == 0) {
            fputc(0x10, output);
        }
        else if (strcasecmp(tokens[0], "sub") == 0) {
            fputc(0x11, output);
        }
        else if (strcasecmp(tokens[0], "mul") == 0) {
            fputc(0x12, output);
        }
        else if (strcasecmp(tokens[0], "div") == 0) {
            fputc(0x13, output);
        }
        else if (strcasecmp(tokens[0], "mod") == 0) {
            fputc(0x14, output);
        }
        // Comparisons
        else if (strcasecmp(tokens[0], "cmp") == 0) {
            fputc(0x20, output);
        }
        else if (strcasecmp(tokens[0], "eq") == 0) {
            fputc(0x21, output);
        }
        else if (strcasecmp(tokens[0], "neq") == 0) {
            fputc(0x22, output);
        }
        else if (strcasecmp(tokens[0], "gt") == 0) {
            fputc(0x23, output);
        }
        else if (strcasecmp(tokens[0], "lt") == 0) {
            fputc(0x24, output);
        }
        // Flow control
        else if (strcasecmp(tokens[0], "jmp") == 0 && token_count >= 2) {
            fputc(0x30, output);
            int addr = parse_number(tokens[1]);
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "jz") == 0 && token_count >= 2) {
            fputc(0x31, output);
            int addr = parse_number(tokens[1]);
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "jnz") == 0 && token_count >= 2) {
            fputc(0x32, output);
            int addr = parse_number(tokens[1]);
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "call") == 0 && token_count >= 2) {
            fputc(0x33, output);
            int addr = parse_number(tokens[1]);
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "ret") == 0) {
            fputc(0x34, output);
        }
        // Memory
        else if (strcasecmp(tokens[0], "load") == 0 && token_count >= 2) {
            fputc(0x40, output);
            fputc(parse_number(tokens[1]) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "store") == 0 && token_count >= 2) {
            fputc(0x41, output);
            fputc(parse_number(tokens[1]) & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "load_abs") == 0) {
            fputc(0x42, output);
        }
        else if (strcasecmp(tokens[0], "store_abs") == 0) {
            fputc(0x43, output);
        }
        // System calls
        else if (strcasecmp(tokens[0], "syscall") == 0 && token_count >= 2) {
            fputc(0x50, output);
            int syscall_num = find_syscall(tokens[1]);
            if (syscall_num == -1) {
                syscall_num = parse_number(tokens[1]);
            }
            fputc(syscall_num & 0xFF, output);
        }
        else if (strcasecmp(tokens[0], "break") == 0) {
            fputc(0x51, output);
        }
        else {
            printf("Warning: Unknown instruction '%s'\n", tokens[0]);
        }
    }

    fclose(input);
    fclose(output);
    printf("Compilation successful!\n");
    return 0;
}