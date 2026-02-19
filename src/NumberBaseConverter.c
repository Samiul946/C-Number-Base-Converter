#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>     /* static_assert (C11) */

/* =========================================================
 *  Constants
 * ========================================================= */

#define LINE_WIDTH               60
#define MAX_BASE                 36
#define MIN_BASE                  2
#define INITIAL_INPUT_BUFFER_SIZE 16

/*
 * Hard cap on number input length.  LLONG_MIN in base 2 is 65 characters
 * (sign + 64 digits), or 66 bytes including NUL.  4096 is ~62× that; anything
 * longer cannot possibly be a valid in-range value and is rejected immediately
 * so the allocator never sees a multi-megabyte paste.
 */
#define MAX_INPUT_BYTES          4096

/*
 * Maximum characters needed to represent any unsigned 64-bit integer
 * in base 2 (64 digits for LLONG_MIN magnitude: 1 followed by 63 zeros)
 * plus one byte for an optional '-' sign and one byte for the NUL terminator.
 * Worst case: 64 + 1 + 1 = 66 bytes; buffer is exactly that.
 */
#define CONVERSION_BUFFER_SIZE  (sizeof(unsigned long long) * CHAR_BIT + 2)


/* =========================================================
 *  Conversion result codes
 *  Returned by convert_base() so callers decide how to report errors.
 * ========================================================= */

typedef enum {
    CONV_OK = 0,
    CONV_BAD_BASE,       /* src or dst base out of [MIN_BASE, MAX_BASE]   */
    CONV_INVALID_CHARS,  /* input contains chars illegal for the src base  */
    CONV_OVERFLOW,       /* value does not fit in a signed 64-bit integer  */
    CONV_OOM,            /* malloc/realloc returned NULL                   */
    CONV_INTERNAL,       /* unreachable logic fault (e.g. val_to_char bug) */
    CONV_COUNT           /* sentinel — must remain last; do not use directly */
} ConvResult;

/* Human-readable messages matching ConvResult codes (index == code). */
static const char* const CONV_MESSAGES[] = {
    "OK",
    "Base must be between 2 and 36.",
    "Input contains invalid characters for the source base.",
    "Number overflows the signed 64-bit range.",
    "Memory allocation failed.",
    "Internal error (this is a bug — please report it)."
};

/*
 * Compile-time guard: CONV_COUNT always equals the number of enum members
 * (excluding itself).  If a new ConvResult value is added without a
 * matching CONV_MESSAGES entry the array size will no longer equal
 * CONV_COUNT and the build fails immediately.
 */
static_assert(
    sizeof(CONV_MESSAGES) / sizeof(CONV_MESSAGES[0]) == CONV_COUNT,
    "CONV_MESSAGES is out of sync with ConvResult enum — add a message entry"
);


/* =========================================================
 *  Function declarations
 * ========================================================= */

/* UI */
static void print_header(const char *title);
static void print_menu(void);
static void print_error(const char *message);
static void print_success(const char *message);
static void clear_input_buffer(void);

/* Utilities */
static char *trim_whitespace(char *str);
static int   char_to_val(unsigned char c);
static int   val_to_char(int val);          /* returns -1 on bad input */
static bool  is_valid_input(const char *input, int base);

/* Core conversion */
static bool  from_any_to_dec(const char *input, int base, long long *out_val);
static char *from_dec_to_any(long long val, int base, ConvResult *out_result);
char        *convert_base(const char *input, int src_base, int dst_base,
                          ConvResult *out_result);

/* Interactive input */
static int   get_numeric_base_input(const char *prompt);

/*
 * Return codes from get_dynamic_input().
 *
 * GDI_OK         — heap string returned; caller must free()
 * GDI_EMPTY      — trimmed input was empty (or whitespace-only)
 * GDI_TOO_LONG   — line exceeded MAX_INPUT_BYTES; line was drained
 * GDI_OOM        — malloc/realloc failed
 * GDI_EOF        — stdin reached EOF
 */
typedef enum {
    GDI_OK = 0,
    GDI_EMPTY,
    GDI_TOO_LONG,
    GDI_OOM,
    GDI_EOF
} GdiStatus;

static GdiStatus get_dynamic_input(const char *prompt, char **out_str);


/* =========================================================
 *  UI helpers
 * ========================================================= */

static void print_header(const char *title)
{
    int len     = (int)strlen(title);
    int pad     = (len >= LINE_WIDTH) ? 0 : (LINE_WIDTH - len) / 2;

    printf("\n");
    for (int i = 0; i < LINE_WIDTH; i++) putchar('=');
    printf("\n%*s%s\n", pad, "", title);
    for (int i = 0; i < LINE_WIDTH; i++) putchar('=');
    printf("\n");
}

static void print_menu(void)
{
    print_header("Universal Number Base Converter");
    printf(" Converts numbers between any base from %d to %d.\n",
           MIN_BASE, MAX_BASE);
    printf("  Digits: 0-9, Letters: A-Z (case-insensitive).\n");
    printf("  Enter 0 for any base to quit.\n");
}

static void print_error(const char *message)
{
    printf("\n! ERROR: %s\n", message);
}

static void print_success(const char *message)
{
    printf("\n%s\n", message);
}

static void clear_input_buffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}


/* =========================================================
 *  Utility functions
 * ========================================================= */

/*
 * Trim leading and trailing ASCII whitespace in-place.
 * Returns a pointer into str (not necessarily str itself).
 */
static char *trim_whitespace(char *str)
{
    char *end;

    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    return str;
}

/* Map an ASCII digit or letter to its integer value, or -1 on error. */
static int char_to_val(unsigned char c)
{
    c = (unsigned char)tolower(c);
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

/*
 * Map an integer value [0, MAX_BASE) to the corresponding ASCII character.
 * Returns -1 for out-of-range values so callers can propagate failure
 * cleanly rather than embedding a silent sentinel character in output.
 * This path is unreachable in correct usage.
 */
static int val_to_char(int val)
{
    if (val < 0 || val >= MAX_BASE) return -1; /* unreachable in correct code */
    return (val <= 9) ? (val + '0') : (val - 10 + 'A');
}

/*
 * Return true if every character in input is a valid digit for the given
 * base, and the value represented is not negative zero ("-0...0").
 *
 * Leading zeros are accepted (e.g. "007" → 7) and are not considered
 * errors, since the converter operates on mathematical values rather than
 * language literals.
 *
 * Negative zero ("-0", "-00", etc.) is rejected: it is not a distinct
 * integer value and its display would be misleading.
 */
static bool is_valid_input(const char *input, int base)
{
    if (!input || !*input) return false;
    if (base < MIN_BASE || base > MAX_BASE) return false;

    bool neg   = (input[0] == '-');
    size_t start = neg ? 1u : 0u;

    /* A bare '-' with no digits is invalid. */
    if (neg && input[1] == '\0') return false;

    /* Validate every digit character. */
    for (size_t i = start; input[i]; i++) {
        int val = char_to_val((unsigned char)input[i]);
        if (val < 0 || val >= base) return false;
    }

    /*
     * Reject negative zero: a '-' sign followed exclusively by '0' digits.
     * This catches "-0", "-00", "-000", etc. in any base.
     */
    if (neg) {
        bool all_zero = true;
        for (size_t i = 1; input[i]; i++) {
            if (input[i] != '0') { all_zero = false; break; }
        }
        if (all_zero) return false;
    }

    return true;
}


/* =========================================================
 *  Core conversion
 * ========================================================= */

/*
 * Parse input (a string of digits in the given base, optionally prefixed
 * with '-') into a signed 64-bit integer.  Returns false on overflow or
 * invalid characters.
 */
static bool from_any_to_dec(const char *input, int base, long long *out_val)
{
    if (!input || !out_val) return false;
    if (base < MIN_BASE || base > MAX_BASE) return false;
    if (*input == '\0') return false;

    bool neg = (input[0] == '-');
    const char *num = neg ? input + 1 : input;
    if (*num == '\0') return false;

    unsigned long long val = 0;

    while (*num) {
        int digit = char_to_val((unsigned char)*num++);
        if (digit < 0 || digit >= base) return false;

        /* Overflow guard: pre-multiply check. */
        if (val > ULLONG_MAX / (unsigned long long)base) return false;
        val *= (unsigned long long)base;

        /* Overflow guard: pre-add check. */
        if (val > ULLONG_MAX - (unsigned long long)digit) return false;
        val += (unsigned long long)digit;
    }

    if (neg) {
        /* The only magnitude that maps to LLONG_MIN is 2^63. */
        if (val == (unsigned long long)LLONG_MAX + 1ULL)
            *out_val = LLONG_MIN;
        else if (val > (unsigned long long)LLONG_MAX)
            return false;
        else
            *out_val = -(long long)val;
    } else {
        if (val > (unsigned long long)LLONG_MAX) return false;
        *out_val = (long long)val;
    }

    return true;
}

/*
 * Convert a signed 64-bit integer to a NUL-terminated string in the given
 * base.  The caller owns the returned allocation and must free() it.
 *
 * On failure, returns NULL and sets *out_result to:
 *   CONV_BAD_BASE  — base is outside [MIN_BASE, MAX_BASE]
 *                    (unreachable when called via convert_base, which
 *                     pre-validates both bases before calling this function)
 *   CONV_OOM       — malloc returned NULL
 *   CONV_INTERNAL  — val_to_char returned -1 (unreachable in correct code)
 */
static char *from_dec_to_any(long long val, int base, ConvResult *out_result)
{
    if (base < MIN_BASE || base > MAX_BASE) {
        *out_result = CONV_BAD_BASE;
        return NULL;
    }

    bool neg = (val < 0);
    unsigned long long abs_val;

    if (val == LLONG_MIN)
        abs_val = (unsigned long long)LLONG_MAX + 1ULL;
    else
        abs_val = neg ? (unsigned long long)(-val) : (unsigned long long)val;

    /* Special-case zero so the loop below never runs with abs_val == 0. */
    if (abs_val == 0) {
        char *zero = malloc(2);
        if (!zero) { *out_result = CONV_OOM; return NULL; }
        zero[0] = '0';
        zero[1] = '\0';
        return zero;
    }

    /*
     * Build digits in reverse order into a stack buffer, then reverse.
     * CONVERSION_BUFFER_SIZE guarantees this never overflows for any
     * 64-bit value in any base >= 2.
     */
    char buf[CONVERSION_BUFFER_SIZE];
    int  i = 0;

    while (abs_val > 0) {
        int ch = val_to_char((int)(abs_val % (unsigned long long)base));
        if (ch < 0) { *out_result = CONV_INTERNAL; return NULL; } /* unreachable */
        buf[i++] = (char)ch;
        abs_val  /= (unsigned long long)base;
    }
    if (neg) buf[i++] = '-';
    buf[i] = '\0';

    /* Reverse the buffer in-place. */
    for (int j = 0; j < i / 2; j++) {
        char tmp       = buf[j];
        buf[j]         = buf[i - j - 1];
        buf[i - j - 1] = tmp;
    }

    char *result = malloc((size_t)i + 1);
    if (!result) { *out_result = CONV_OOM; return NULL; }
    memcpy(result, buf, (size_t)i + 1);
    return result;
}

/*
 * Full pipeline: validate, parse, and convert.
 *
 * On success  : returns a heap-allocated result string; *out_result = CONV_OK.
 * On failure  : returns NULL; *out_result indicates the reason.
 *
 * out_result may be NULL if the caller does not need the error code.
 *
 * Error reporting is intentionally left to the caller; this function
 * performs no I/O so it can be embedded in non-interactive contexts.
 *
 * Note: src_base undergoes the same explicit range check as dst_base.
 * is_valid_input() would catch a bad src_base implicitly, but the explicit
 * check here makes the two paths symmetric and produces the correct error code.
 */
char *convert_base(const char *input, int src_base, int dst_base,
                   ConvResult *out_result)
{
    ConvResult dummy;
    if (!out_result) out_result = &dummy;

    if (src_base < MIN_BASE || src_base > MAX_BASE ||
        dst_base < MIN_BASE || dst_base > MAX_BASE) {
        *out_result = CONV_BAD_BASE;
        return NULL;
    }

    if (!input || !is_valid_input(input, src_base)) {
        *out_result = CONV_INVALID_CHARS;
        return NULL;
    }

    long long dec_val;
    if (!from_any_to_dec(input, src_base, &dec_val)) {
        *out_result = CONV_OVERFLOW;
        return NULL;
    }

    char *result = from_dec_to_any(dec_val, dst_base, out_result);
    if (!result) return NULL;   /* out_result already set by from_dec_to_any */

    *out_result = CONV_OK;
    return result;
}


/* =========================================================
 *  Interactive input helpers
 * ========================================================= */

/*
 * Prompt for a base number in [MIN_BASE, MAX_BASE] or 0 (quit).
 * Loops until valid input is provided or EOF is encountered.
 */
static int get_numeric_base_input(const char *prompt)
{
    char buf[32];

    while (1) {
        printf("\n%s", prompt);
        fflush(stdout); /* ensure prompt is visible before blocking on read */

        if (!fgets(buf, sizeof(buf), stdin)) {
            if (feof(stdin) || ferror(stdin)) return 0;
            print_error("Failed to read input.");
            continue;
        }

        /* Reject if the line did not fit (no newline and not at EOF). */
        if (!strchr(buf, '\n') && !feof(stdin)) {
            print_error("Input too long. Enter a number.");
            clear_input_buffer();
            continue;
        }

        char *end;
        long parsed = strtol(buf, &end, 10);

        /*
         * raw_end == buf  → strtol consumed no digits (blank/whitespace line).
         * *end != '\0'    → trailing garbage after the number.
         */
        const char *raw_end = end;
        while (isspace((unsigned char)*end)) end++;
        if (raw_end == buf || *end != '\0') {
            print_error("Invalid input. Enter a number.");
            continue;
        }

        /* strtol saturation at LONG_MAX/LONG_MIN is also caught here. */
        if (parsed < 0 || parsed > MAX_BASE) {
            printf("! ERROR: Enter base between %d and %d (or 0 to quit).\n",
                   MIN_BASE, MAX_BASE);
            continue;
        }

        int base = (int)parsed;
        if (base == 0 || (base >= MIN_BASE && base <= MAX_BASE)) return base;

        printf("! ERROR: Enter base between %d and %d (or 0 to quit).\n",
               MIN_BASE, MAX_BASE);
    }
}

/*
 * Read one line from stdin, trim whitespace, and return it via *out_str.
 * The caller must free() *out_str when GDI_OK is returned.
 * *out_str is set to NULL for all non-OK statuses.
 */
static GdiStatus get_dynamic_input(const char *prompt, char **out_str)
{
    *out_str = NULL;

    printf("%s", prompt);
    fflush(stdout); /* ensure prompt is visible before blocking on read */

    size_t size = INITIAL_INPUT_BUFFER_SIZE;
    size_t len  = 0;
    char  *str  = malloc(size);
    if (!str) return GDI_OOM;

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (len + 1 >= size) {
            /*
             * Hard cap: any number valid in the signed 64-bit range fits
             * comfortably within MAX_INPUT_BYTES.  Reject and drain the
             * rest of the line rather than allocating without bound.
             */
            if (size >= MAX_INPUT_BYTES) {
                free(str);
                while ((ch = getchar()) != '\n' && ch != EOF)
                    ;
                return GDI_TOO_LONG;
            }
            size *= 2;
            if (size > MAX_INPUT_BYTES) size = MAX_INPUT_BYTES;
            char *temp = realloc(str, size);
            if (!temp) { free(str); return GDI_OOM; }
            str = temp;
        }
        str[len++] = (char)ch;
    }

    /* EOF terminates the session regardless of how many chars were buffered. */
    if (ch == EOF) {
        free(str);
        return GDI_EOF;
    }

    str[len] = '\0';

    /* Shift trimmed content to the front of the allocation. */
    char  *trimmed     = trim_whitespace(str);
    size_t trimmed_len = strlen(trimmed);
    if (trimmed != str) memmove(str, trimmed, trimmed_len + 1);

    if (trimmed_len == 0) { free(str); return GDI_EMPTY; }

    /* Shrink to exact size; keep the larger allocation if realloc fails. */
    char *shrunk = realloc(str, trimmed_len + 1);
    if (shrunk) str = shrunk;

    *out_str = str;
    return GDI_OK;
}


/* =========================================================
 *  Main
 * ========================================================= */

int main(void)
{
    while (1) {
        print_menu();

        int src_base = get_numeric_base_input(
            "Enter source base (2-36, or 0 to quit): ");
        if (src_base == 0) break;

        int dst_base = get_numeric_base_input(
            "Enter destination base (2-36, or 0 to quit): ");
        if (dst_base == 0) break;

        char prompt[64];
        snprintf(prompt, sizeof(prompt),
                 "\nEnter number in base %d: ", src_base);

        bool converted = false;
        while (!converted) {
            char *input  = NULL;
            GdiStatus gs = get_dynamic_input(prompt, &input);

            if (gs != GDI_OK) {
                if (gs == GDI_EOF)      goto quit;
                if (gs == GDI_TOO_LONG) print_error("Input too long to be a valid number.");
                else if (gs == GDI_OOM) print_error("Out of memory reading input.");
                else                    print_error("Input cannot be empty.");
                continue;
            }

            ConvResult res;
            char *result = convert_base(input, src_base, dst_base, &res);
            if (result) {
                converted = true;
                print_success("Conversion Result:");
                printf("\n %s (base %d)\n", input, src_base);
                printf(" | converts to |\n");
                printf(" v             v\n");
                printf(" %s (base %d)\n", result, dst_base);
                free(result);
            } else {
                print_error(CONV_MESSAGES[res]);
            }

            free(input);
        }

        printf("\nConvert another number? (Y/N): ");
        fflush(stdout);
        int choice = getchar();
        /*
         * Only drain the rest of the line when getchar() did NOT already
         * consume the newline.  Without this guard, a bare Enter press
         * consumes '\n' here, then clear_input_buffer blocks waiting for
         * a second newline that never arrives in interactive use.
         */
        if (choice != '\n' && choice != EOF) clear_input_buffer();

        if (choice == EOF || toupper(choice) != 'Y') break;
    }

quit:
    print_header("Thank you for using the converter!");
    return 0;
}
