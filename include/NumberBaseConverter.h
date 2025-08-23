#ifndef NUMBER_BASE_CONVERTER_H
#define NUMBER_BASE_CONVERTER_H

#include <stdbool.h>

// ========================
// Function Declarations
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
static char* get_dynamic_input(const char* prompt);

#endif // NUMBER_BASE_CONVERTER_H
