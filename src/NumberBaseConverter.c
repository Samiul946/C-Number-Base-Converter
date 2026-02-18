#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>

// ========================
//  Constants
// ========================
#define LINE_WIDTH 60
#define MAX_BASE 36
#define INITIAL_INPUT_BUFFER_SIZE 16
#define CONVERSION_BUFFER_SIZE (sizeof(unsigned long long) * CHAR_BIT + 2)

// ========================
//  Function Declarations
// ========================
void print_header(const char* title);
void print_menu(void);
void print_error(const char* message);
void print_success(const char* message);
void clear_input_buffer(void);
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
    size_t len = strlen(title);
    int pad_left = (LINE_WIDTH - (int)len) / 2;
    int pad_right = LINE_WIDTH - pad_left - (int)len;

    printf("\n");
    for (int i = 0; i < LINE_WIDTH; i++) putchar('=');
    printf("\n%*s%s%*s\n", pad_left, "", title, pad_right, "");
    for (int i = 0; i < LINE_WIDTH; i++) putchar('=');
    printf("\n");
}

void print_menu(void) {
    print_header("Universal Number Base Converter");
    printf(" This program converts numbers between any base from 2 to %d.\n", MAX_BASE);
    printf("  - Digits: 0–9, Letters: A–Z (case-insensitive)\n");
    printf("  - Enter 0 for any base to quit.\n");
}

void print_error(const char* message) {
    printf("\n! ERROR: %s\n", message);
}

void print_success(const char* message) {
    printf("\n%s\n", message);
}

void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ========================
//  Utility Functions
// ========================
char* trim_whitespace(char* str) {
    char* end;

    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;

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
    if (val >= 0 && val <= 9) return (char)(val + '0');
    if (val >= 10 && val < MAX_BASE) return (char)(val - 10 + 'A');

    fprintf(stderr, "Internal error: invalid digit %d\n", val);
    abort();
}

bool is_valid_input(const char* input, int base) {
    if (!input || !*input) return false;

    size_t start = (input[0] == '-' ? 1 : 0);
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

        if (val > ULLONG_MAX / (unsigned long long)base) return false;
        val *= (unsigned long long)base;

        if (val > ULLONG_MAX - (unsigned long long)digit) return false;
        val += (unsigned long long)digit;
    }

    if (neg) {
        if (val == (unsigned long long)LLONG_MAX + 1ULL) {
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
    unsigned long long abs_val = neg ? -(unsigned long long)val : (unsigned long long)val;

    if (abs_val == 0) {
        char* zero = malloc(2);
        if (!zero) return NULL;
        strcpy(zero, "0");
        return zero;
    }

    char buf[CONVERSION_BUFFER_SIZE];
    int i = 0;

    while (abs_val > 0) {
        buf[i++] = val_to_char((int)(abs_val % (unsigned)base));
        abs_val /= (unsigned)base;
    }

    if (neg) buf[i++] = '-';
    buf[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }

    char* result = malloc((size_t)i + 1);
    if (!result) return NULL;

    memcpy(result, buf, (size_t)i + 1);
    return result;
}

char* convert_base(const char* input, int src_base, int dst_base) {
    if (!input || !*input) {
        print_error("Input cannot be empty.");
        return NULL;
    }

    if (!is_valid_input(input, src_base)) {
        print_error("Input contains invalid characters for the source base.");
        return NULL;
    }

    long long dec_val;
    if (!from_any_to_dec(input, src_base, &dec_val)) {
        print_error("Number overflows signed 64-bit range.");
        return NULL;
    }

    char* result = from_dec_to_any(dec_val, dst_base);
    if (!result) {
        print_error("Memory allocation failed.");
        return NULL;
    }

    return result;
}

// ========================
//  Input Functions
// ========================
int get_numeric_base_input(const char* prompt) {
    int base;

    while (1) {
        printf("\n%s", prompt);

        if (scanf("%d", &base) != 1) {
            print_error("Invalid input. Enter a number.");
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
            if (!temp) {
                free(str);
                return NULL;
            }
            str = temp;
        }
        str[len++] = (char)ch;
    }

    str[len] = '\0';

    char* trimmed = trim_whitespace(str);
    if (trimmed != str) memmove(str, trimmed, strlen(trimmed) + 1);

    if (*str == '\0') {
        free(str);
        return NULL;
    }

    char* shrunk = realloc(str, strlen(str) + 1);
    if (shrunk) str = shrunk;

    return str;
}

// ========================
//  Main
// ========================
int main(void) {
    while (1) {
        print_menu();

        int src_base = get_numeric_base_input("Enter source base (2–36, or 0 to quit): ");
        if (src_base == 0) break;

        int dst_base = get_numeric_base_input("Enter destination base (2–36, or 0 to quit): ");
        if (dst_base == 0) break;

        char prompt[64];
        snprintf(prompt, sizeof(prompt), "\nEnter number in base %d: ", src_base);

        char* input = get_dynamic_input(prompt);
        if (!input) {
            print_error("Input cannot be empty.");
            continue;
        }

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
        int choice = getchar();
        clear_input_buffer();

        if (choice == EOF || toupper(choice) != 'Y') break;
    }

    print_header("Thank you for using the converter!");
    return 0;
}
