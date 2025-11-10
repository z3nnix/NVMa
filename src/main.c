#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

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

// Structure for labels
typedef struct {
    char *name;
    int address;
} Label;

Label *labels = NULL;
int label_count = 0;
int current_address = 4; // Start after signature

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

char* generate_output_filename(const char *input_filename) {
    static char output[256];
    char *last_dot = strrchr(input_filename, '.');
    
    if (last_dot != NULL) {
        size_t base_len = last_dot - input_filename;
        strncpy(output, input_filename, base_len);
        output[base_len] = '\0';
        strcat(output, ".bin");
    } else {
        strcpy(output, input_filename);
        strcat(output, ".bin");
    }
    
    return output;
}

void add_label(const char *name, int address) {
    labels = realloc(labels, (label_count + 1) * sizeof(Label));
    labels[label_count].name = strdup(name);
    labels[label_count].address = address;
    label_count++;
}

int find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            return labels[i].address;
        }
    }
    return -1;
}

void free_labels() {
    for (int i = 0; i < label_count; i++) {
        free(labels[i].name);
    }
    free(labels);
    labels = NULL;
    label_count = 0;
}

int get_instruction_size(char *tokens[], int token_count) {
    if (strcasecmp(tokens[0], ".NVM0") == 0) return 0;
    else if (strcasecmp(tokens[0], "hlt") == 0 || strcasecmp(tokens[0], "halt") == 0) return 1;
    else if (strcasecmp(tokens[0], "nop") == 0) return 1;
    else if (strcasecmp(tokens[0], "push") == 0 && token_count >= 3) {
        if (strcasecmp(tokens[1], "byte") == 0) return 2;
        else if (strcasecmp(tokens[1], "short") == 0) return 3;
    }
    else if (strcasecmp(tokens[0], "pop") == 0) return 1;
    else if (strcasecmp(tokens[0], "dup") == 0) return 1;
    else if (strcasecmp(tokens[0], "swap") == 0) return 1;
    // Arithmetic
    else if (strcasecmp(tokens[0], "add") == 0) return 1;
    else if (strcasecmp(tokens[0], "sub") == 0) return 1;
    else if (strcasecmp(tokens[0], "mul") == 0) return 1;
    else if (strcasecmp(tokens[0], "div") == 0) return 1;
    else if (strcasecmp(tokens[0], "mod") == 0) return 1;
    // Comparisons
    else if (strcasecmp(tokens[0], "cmp") == 0) return 1;
    else if (strcasecmp(tokens[0], "eq") == 0) return 1;
    else if (strcasecmp(tokens[0], "neq") == 0) return 1;
    else if (strcasecmp(tokens[0], "gt") == 0) return 1;
    else if (strcasecmp(tokens[0], "lt") == 0) return 1;
    // Flow control
    else if (strcasecmp(tokens[0], "jmp") == 0 && token_count >= 2) return 3;
    else if (strcasecmp(tokens[0], "jz") == 0 && token_count >= 2) return 3;
    else if (strcasecmp(tokens[0], "jnz") == 0 && token_count >= 2) return 3;
    else if (strcasecmp(tokens[0], "call") == 0 && token_count >= 2) return 3;
    else if (strcasecmp(tokens[0], "ret") == 0) return 1;
    // Memory
    else if (strcasecmp(tokens[0], "load") == 0 && token_count >= 2) return 2;
    else if (strcasecmp(tokens[0], "store") == 0 && token_count >= 2) return 2;
    else if (strcasecmp(tokens[0], "load_abs") == 0) return 1;
    else if (strcasecmp(tokens[0], "store_abs") == 0) return 1;
    // System calls
    else if (strcasecmp(tokens[0], "syscall") == 0 && token_count >= 2) return 2;
    else if (strcasecmp(tokens[0], "break") == 0) return 1;
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s input.asm [output.bin]\n", argv[0]);
        printf("If output.bin is omitted, it will be generated from input filename\n");
        return 1;
    }

    char *output_filename;
    if (argc >= 3) {
        output_filename = argv[2];
    } else {
        output_filename = generate_output_filename(argv[1]);
    }

    // First pass: collect labels
    FILE *input = fopen(argv[1], "r");
    if (!input) {
        printf("Error opening input file '%s': %s\n", argv[1], strerror(errno));
        return 1;
    }

    char line[256];
    current_address = 4; // Start after signature
    
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
        
        // Check if it's a label (ends with colon)
        if (start[strlen(start) - 1] == ':') {
            char label_name[256];
            strncpy(label_name, start, strlen(start) - 1);
            label_name[strlen(start) - 1] = '\0';
            
            add_label(label_name, current_address);
            continue;
        }
        
        // Tokenize
        char *tokens[4];
        int token_count = 0;
        char *token = strtok(start, " \t,");
        
        while (token && token_count < 4) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t,");
        }
        
        if (token_count == 0) continue;
        
        int size = get_instruction_size(tokens, token_count);
        current_address += size;
    }
    
    fclose(input);
    
    // Second pass: actual compilation
    input = fopen(argv[1], "r");
    FILE *output = fopen(output_filename, "wb");
    
    if (!input || !output) {
        printf("Error opening files: %s\n", strerror(errno));
        free_labels();
        return 1;
    }

    printf("Input: %s\n", argv[1]);
    printf("Output: %s\n", output_filename);

    // Write signature
    fputc(0x4E, output); // 'N'
    fputc(0x56, output); // 'V'
    fputc(0x4D, output); // 'M'
    fputc(0x30, output); // '0'

    current_address = 4;
    int line_num = 0;
    
    while (fgets(line, sizeof(line), input)) {
        line_num++;
        
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
        
        // Skip labels in second pass (they don't generate code)
        if (start[strlen(start) - 1] == ':') {
            continue;
        }
        
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
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "nop") == 0) {
            fputc(0x01, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "push") == 0 && token_count >= 3) {
            if (strcasecmp(tokens[1], "byte") == 0) {
                fputc(0x02, output);
                fputc(parse_number(tokens[2]) & 0xFF, output);
                current_address += 2;
            }
            else if (strcasecmp(tokens[1], "short") == 0) {
                fputc(0x03, output);
                int value = parse_number(tokens[2]);
                fputc(value & 0xFF, output);
                fputc((value >> 8) & 0xFF, output);
                current_address += 3;
            }
        }
        else if (strcasecmp(tokens[0], "pop") == 0) {
            fputc(0x04, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "dup") == 0) {
            fputc(0x05, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "swap") == 0) {
            fputc(0x06, output);
            current_address += 1;
        }
        // Arithmetic
        else if (strcasecmp(tokens[0], "add") == 0) {
            fputc(0x10, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "sub") == 0) {
            fputc(0x11, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "mul") == 0) {
            fputc(0x12, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "div") == 0) {
            fputc(0x13, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "mod") == 0) {
            fputc(0x14, output);
            current_address += 1;
        }
        // Comparisons
        else if (strcasecmp(tokens[0], "cmp") == 0) {
            fputc(0x20, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "eq") == 0) {
            fputc(0x21, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "neq") == 0) {
            fputc(0x22, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "gt") == 0) {
            fputc(0x23, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "lt") == 0) {
            fputc(0x24, output);
            current_address += 1;
        }
        // Flow control
        else if (strcasecmp(tokens[0], "jmp") == 0 && token_count >= 2) {
            fputc(0x30, output);
            int addr;
            // Check if it's a label
            int label_addr = find_label(tokens[1]);
            if (label_addr != -1) {
                addr = label_addr;
            } else {
                addr = parse_number(tokens[1]);
            }
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
            current_address += 3;
        }
        else if (strcasecmp(tokens[0], "jz") == 0 && token_count >= 2) {
            fputc(0x31, output);
            int addr;
            int label_addr = find_label(tokens[1]);
            if (label_addr != -1) {
                addr = label_addr;
            } else {
                addr = parse_number(tokens[1]);
            }
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
            current_address += 3;
        }
        else if (strcasecmp(tokens[0], "jnz") == 0 && token_count >= 2) {
            fputc(0x32, output);
            int addr;
            int label_addr = find_label(tokens[1]);
            if (label_addr != -1) {
                addr = label_addr;
            } else {
                addr = parse_number(tokens[1]);
            }
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
            current_address += 3;
        }
        else if (strcasecmp(tokens[0], "call") == 0 && token_count >= 2) {
            fputc(0x33, output);
            int addr;
            int label_addr = find_label(tokens[1]);
            if (label_addr != -1) {
                addr = label_addr;
            } else {
                addr = parse_number(tokens[1]);
            }
            fputc(addr & 0xFF, output);
            fputc((addr >> 8) & 0xFF, output);
            current_address += 3;
        }
        else if (strcasecmp(tokens[0], "ret") == 0) {
            fputc(0x34, output);
            current_address += 1;
        }
        // Memory
        else if (strcasecmp(tokens[0], "load") == 0 && token_count >= 2) {
            fputc(0x40, output);
            fputc(parse_number(tokens[1]) & 0xFF, output);
            current_address += 2;
        }
        else if (strcasecmp(tokens[0], "store") == 0 && token_count >= 2) {
            fputc(0x41, output);
            fputc(parse_number(tokens[1]) & 0xFF, output);
            current_address += 2;
        }
        else if (strcasecmp(tokens[0], "load_abs") == 0) {
            fputc(0x42, output);
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "store_abs") == 0) {
            fputc(0x43, output);
            current_address += 1;
        }
        // System calls
        else if (strcasecmp(tokens[0], "syscall") == 0 && token_count >= 2) {
            fputc(0x50, output);
            int syscall_num = find_syscall(tokens[1]);
            if (syscall_num == -1) {
                syscall_num = parse_number(tokens[1]);
            }
            fputc(syscall_num & 0xFF, output);
            current_address += 2;
        }
        else if (strcasecmp(tokens[0], "break") == 0) {
            fputc(0x51, output);
            current_address += 1;
        }
        else {
            printf("Warning at line %d: Unknown instruction '%s'\n", line_num, tokens[0]);
        }
    }

    fclose(input);
    fclose(output);
    free_labels();
    
    printf("Compilation successful! \n");
    return 0;
}