#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

// Syscall table
typedef struct {
    const char *name;
    int number;
} Syscall;

Syscall syscalls[] = {
    {"exit", 0x00},
    {"exec", 0x01},
    {"open", 0x02},
    {"read", 0x03},
    {"write", 0x04},
    {"create", 0x05},
    {"delete", 0x06},
    {"cap_request", 0x07},
    {"cap_spawn", 0x08},
    {"drv_call", 0x09},
    {"msg_send", 0x0A},
    {"msg_recieve", 0x0B},
    {"inb", 0x0C},
    {"outb", 0x0D},
    {NULL, 0}
};

// Structure for labels
typedef struct {
    char *name;
    uint32_t address;
} Label;

Label *labels = NULL;
int label_count = 0;
uint32_t current_address = 4; // Start after signature

int find_syscall(const char *name) {
    for (int i = 0; syscalls[i].name != NULL; i++) {
        if (strcmp(syscalls[i].name, name) == 0) {
            return syscalls[i].number;
        }
    }
    return -1;
}

uint32_t parse_number(const char *str) {
    if (str[0] == '\'' && str[1] != '\0' && str[2] == '\'') {
        return (uint32_t)str[1];
    }
    else if (strncmp(str, "0x", 2) == 0) {
        return (uint32_t)strtol(str + 2, NULL, 16);
    } 
    else {
        return (uint32_t)atoi(str);
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

void add_label(const char *name, uint32_t address) {
    labels = realloc(labels, (label_count + 1) * sizeof(Label));
    labels[label_count].name = strdup(name);
    labels[label_count].address = address;
    label_count++;
}

int find_label(const char *name, uint32_t *address) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0) {
            *address = labels[i].address;
            return 1;
        }
    }
    return 0;
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
    if (token_count == 0) return 0;
    
    if (strcasecmp(tokens[0], ".NVM0") == 0) return 0;
    else if (strcasecmp(tokens[0], "hlt") == 0 || strcasecmp(tokens[0], "halt") == 0) return 1;
    else if (strcasecmp(tokens[0], "nop") == 0) return 1;
    else if (strcasecmp(tokens[0], "push") == 0 && token_count >= 2) return 5; // PUSH32: 1 byte opcode + 4 bytes value
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
    // Flow control (32-bit)
    else if (strcasecmp(tokens[0], "jmp") == 0) return 5;    // JMP32: 1 + 4
    else if (strcasecmp(tokens[0], "jz") == 0) return 5;     // JZ32: 1 + 4
    else if (strcasecmp(tokens[0], "jnz") == 0) return 5;    // JNZ32: 1 + 4
    else if (strcasecmp(tokens[0], "call") == 0) return 5;   // CALL32: 1 + 4
    else if (strcasecmp(tokens[0], "ret") == 0) return 1;
    // Memory
    else if (strcasecmp(tokens[0], "load") == 0) return 2;   // 1 + 1 byte index
    else if (strcasecmp(tokens[0], "store") == 0) return 2;  // 1 + 1 byte index
    else if (strcasecmp(tokens[0], "load_abs") == 0) return 1; // LOAD_ABS: 1 byte
    else if (strcasecmp(tokens[0], "store_abs") == 0) return 1; // STORE_ABS: 1 byte
    // System calls
    else if (strcasecmp(tokens[0], "syscall") == 0) return 2; // 1 + 1 byte syscall ID
    else if (strcasecmp(tokens[0], "break") == 0) return 1;
    
    return 0;
}

void write_32bit_value(FILE *output, uint32_t value) {
    fputc((value >> 24) & 0xFF, output);
    fputc((value >> 16) & 0xFF, output);
    fputc((value >> 8) & 0xFF, output);
    fputc(value & 0xFF, output);
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
        else if (strcasecmp(tokens[0], "push") == 0 && token_count >= 2) {
            fputc(0x02, output); // PUSH32 opcode
            uint32_t value = parse_number(tokens[1]);
            write_32bit_value(output, value);
            current_address += 5;
            
            // Debug output
            printf("DEBUG: PUSH32 0x%08X (%d) at address %d\n", value, value, current_address - 5);
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
        // Flow control (32-bit)
        else if (strcasecmp(tokens[0], "jmp") == 0 && token_count >= 2) {
            fputc(0x30, output); // JMP32
            uint32_t addr;
            if (find_label(tokens[1], &addr)) {
                write_32bit_value(output, addr);
            } else {
                addr = parse_number(tokens[1]);
                write_32bit_value(output, addr);
            }
            current_address += 5;
        }
        else if (strcasecmp(tokens[0], "jz") == 0 && token_count >= 2) {
            fputc(0x31, output); // JZ32
            uint32_t addr;
            if (find_label(tokens[1], &addr)) {
                write_32bit_value(output, addr);
            } else {
                addr = parse_number(tokens[1]);
                write_32bit_value(output, addr);
            }
            current_address += 5;
        }
        else if (strcasecmp(tokens[0], "jnz") == 0 && token_count >= 2) {
            fputc(0x32, output); // JNZ32
            uint32_t addr;
            if (find_label(tokens[1], &addr)) {
                write_32bit_value(output, addr);
            } else {
                addr = parse_number(tokens[1]);
                write_32bit_value(output, addr);
            }
            current_address += 5;
        }
        else if (strcasecmp(tokens[0], "call") == 0 && token_count >= 2) {
            fputc(0x33, output); // CALL32
            uint32_t addr;
            if (find_label(tokens[1], &addr)) {
                write_32bit_value(output, addr);
            } else {
                addr = parse_number(tokens[1]);
                write_32bit_value(output, addr);
            }
            current_address += 5;
        }
        else if (strcasecmp(tokens[0], "ret") == 0) {
            fputc(0x34, output);
            current_address += 1;
        }
        // Memory
        else if (strcasecmp(tokens[0], "load") == 0 && token_count >= 2) {
            fputc(0x40, output);
            uint8_t var_index = (uint8_t)parse_number(tokens[1]);
            fputc(var_index, output);
            current_address += 2;
        }
        else if (strcasecmp(tokens[0], "store") == 0 && token_count >= 2) {
            fputc(0x41, output);
            uint8_t var_index = (uint8_t)parse_number(tokens[1]);
            fputc(var_index, output);
            current_address += 2;
        }
        // New absolute memory access instructions
        else if (strcasecmp(tokens[0], "load_abs") == 0) {
            fputc(0x44, output); // LOAD_ABS opcode
            current_address += 1;
        }
        else if (strcasecmp(tokens[0], "store_abs") == 0) {
            fputc(0x45, output); // STORE_ABS opcode
            current_address += 1;
        }
        // System calls
        else if (strcasecmp(tokens[0], "syscall") == 0 && token_count >= 2) {
            fputc(0x50, output);
            int syscall_num = find_syscall(tokens[1]);
            if (syscall_num == -1) {
                syscall_num = (int)parse_number(tokens[1]);
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
    
    printf("Compilation successful! Output size: %d bytes\n", current_address);
    return 0;
}