#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h> // For LLONG_MAX, LLONG_MIN, ULLONG_MAX

// ========================
//  Constants
// ========================
#define LINE_WIDTH 60
#define MAX_BASE 36
#define INITIAL_INPUT_BUFFER_SIZE 16
#define CONVERSION_BUFFER_SIZE 70 // Sufficient for 64-bit binary + sign

// ========================
//  Function Declarations
// ========================
void print_header(const char* title);
void print_menu();
void print_error(const char* message);
void print_success(const char* message);
void clear_input_buffer();
char* trim_whitespace(char* str);
int char_to_val(char c);
char val_to_char(int val);
bool is_valid_input(const char* input, int base);
bool from_any_to_dec(const char* input, int base, long long* out_val);
char* from_dec_to_any(long long val, int base);
char* convert_base(const char* input, int src_base, int dst_base);
int get_numeric_base_input(const char* prompt);
char* get_dynamic_input(const char* prompt);

// ========================
//  UI Functions
// ========================
void print_header(const char* title) {
    printf("\n");
    for (int i = 0; i < LINE_WIDTH; i++) printf("=");
    int padding = (LINE_WIDTH - (int)strlen(title)) / 2;
    printf("\n%*s%s%*s\n", padding, "", title, padding, "");
    for (int i = 0; i < LINE_WIDTH; i++) printf("=");
    printf("\n");
}

void print_menu() {
    print_header("Universal Number Base Converter");
    printf(" This program can convert numbers between any base from 2 to %d.\n", MAX_BASE);
    printf("  - Use digits 0-9 and letters A-Z (case-insensitive).\n");
    printf("  - Enter '0' for a base to quit.\n");
}

void print_error(const char* message) {
    printf("\n! ERROR: %s\n", message);
}

void print_success(const char* message) {
    printf("\n%s\n", message);
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ========================
//  Utility Functions
// ========================
char* trim_whitespace(char* str) {
    char* end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    return str;
}

int char_to_val(char c) {
    c = tolower((unsigned char)c);
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

char val_to_char(int val) {
    if (val >= 0 && val <= 9) return val + '0';
    if (val >= 10 && val < MAX_BASE) return val - 10 + 'A';
    return '?';
}

bool is_valid_input(const char* input, int base) {
    if (!input) return false;
    size_t start = (input[0] == '-' ? 1 : 0);
    // OPTIMIZATION: Cache strlen result
    size_t len = strlen(input);
    if (start && len == 1) return false;
    for (size_t i = start; i < len; i++) {
        int val = char_to_val(input[i]);
        if (val < 0 || val >= base) return false;
    }
    return true;
}

// ========================
//  Conversion Logic
// ========================
bool from_any_to_dec(const char* input, int base, long long* out_val) {
    bool neg = (input[0] == '-');
    const char* num = neg ? input + 1 : input;
    unsigned long long val = 0;

    while (*num) {
        int digit = char_to_val(*num++);
        if (val > (ULLONG_MAX - digit) / base) return false;
        val = val * base + digit;
    }

    if (neg) {
        if (val == (unsigned long long)LLONG_MAX + 1) {
            *out_val = LLONG_MIN;
        } else if (val > LLONG_MAX) {
            return false;
        } else {
            *out_val = -(long long)val;
        }
    } else {
        if (val > LLONG_MAX) return false;
        *out_val = (long long)val;
    }
    return true;
}

char* from_dec_to_any(long long val, int base) {
    bool neg = (val < 0);
    unsigned long long abs_val = neg ? -(unsigned long long)val : val;

    if (abs_val == 0) {
        char* zero = malloc(2);
        if (!zero) return NULL;
        strcpy(zero, "0");
        return zero;
    }

    char buf[CONVERSION_BUFFER_SIZE];
    int i = 0;
    while (abs_val > 0) {
        buf[i++] = val_to_char(abs_val % base);
        abs_val /= base;
    }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }

    char* result = malloc(i + 1);
    if (!result) return NULL;
    strcpy(result, buf);
    return result;
}

char* convert_base(const char* input, int src_base, int dst_base) {
    if (!input || input[0] == '\0') {
        print_error("Input cannot be empty.");
        return NULL;
    }
    if (!is_valid_input(input, src_base)) {
        print_error("Input contains invalid characters for the source base.");
        return NULL;
    }
    long long dec_val;
    if (!from_any_to_dec(input, src_base, &dec_val)) {
        print_error("Number is too large for a 64-bit signed integer.");
        return NULL;
    }

    char* result = from_dec_to_any(dec_val, dst_base);
    if (!result) {
        print_error("Memory allocation failed during conversion.");
        return NULL;
    }
    return result;
}

// ========================
//  Input Functions
// ========================
int get_numeric_base_input(const char* prompt) {
    int base = 0;
    while (1) {
        printf("\n%s", prompt);
        if (scanf(" %d", &base) != 1) {
            print_error("Invalid input. Please enter a number.");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();
        if (base == 0 || (base >= 2 && base <= MAX_BASE)) return base;
        printf("! ERROR: Enter base between 2 and %d (or 0 to quit).\n", MAX_BASE);
    }
}

char* get_dynamic_input(const char* prompt) {
    printf("%s", prompt);
    size_t size = INITIAL_INPUT_BUFFER_SIZE, len = 0;
    char* str = malloc(size);
    if (!str) return NULL;

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (len + 1 >= size) {
            size *= 2;
            char* temp = realloc(str, size);
            if (!temp) { free(str); return NULL; }
            str = temp;
        }
        str[len++] = ch;
    }
    str[len] = '\0';

    char* trimmed_start = trim_whitespace(str);
    size_t trimmed_len = strlen(trimmed_start);

    if (trimmed_start != str) {
        memmove(str, trimmed_start, trimmed_len + 1);
    }

    if (trimmed_len + 1 < size) {
        char* shrunk = realloc(str, trimmed_len + 1);
        if (shrunk) {
            str = shrunk;
        }
    }

    return str;
}

// ========================
//  Main
// ========================
int main() {
    while (1) {
        print_menu();
        int src_base = get_numeric_base_input("Enter source base (2-36, or 0 to quit): ");
        if (src_base == 0) break;
        int dst_base = get_numeric_base_input("Enter destination base (2-36, or 0 to quit): ");
        if (dst_base == 0) break;

        char prompt[100];
        sprintf(prompt, "\nEnter number in base %d: ", src_base);
        char* input = get_dynamic_input(prompt);
        if (!input) { print_error("Memory allocation failed."); continue; }

        char* result = convert_base(input, src_base, dst_base);
        if (result) {
            print_success("Conversion Result:");
            printf("\n %s (base %d)\n", input, src_base);
            printf(" | converts to |\n");
            printf(" v             v\n");
            printf(" %s (base %d)\n", result, dst_base);
            free(result);
        }

        free(input);
        printf("\nConvert another number? (Y/N): ");
        char choice = getchar();
        if (choice == EOF) break;
        clear_input_buffer();
        if (toupper(choice) != 'Y') break;
    }
    print_header("Thank you for using the converter!");
    return 0;
}
