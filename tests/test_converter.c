#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>

// Include implementation directly since we aren’t splitting yet
#include "../src/NumberBaseConverter.c"

#define RUN_TEST(desc, expr) \
    do { \
        if (expr) { \
            printf("✅ PASS: %s\n", desc); \
        } else { \
            printf("❌ FAIL: %s\n", desc); \
            failed++; \
        } \
        total++; \
    } while (0)

int main(void) {
    int total = 0, failed = 0;
    char *res;
    long long num;
    int ok;

    // === Basic round-trips across bases ===
    for (int base = 2; base <= 36; base++) {
        res = from_dec_to_any(12345, base);
        ok = from_any_to_dec(res, base, &num);
        RUN_TEST("Round-trip 12345 across base", ok && num == 12345);
        free(res);
    }

    // === Specific conversions ===
    res = convert_base("1010", 2, 10); // binary → decimal
    RUN_TEST("Binary 1010 → Decimal 10", strcmp(res, "10") == 0);
    free(res);

    res = convert_base("FF", 16, 2); // hex → binary
    RUN_TEST("Hex FF → Binary 11111111", strcmp(res, "11111111") == 0);
    free(res);

    res = convert_base("Z", 36, 10); // max digit in base36
    RUN_TEST("Base36 Z → Decimal 35", strcmp(res, "35") == 0);
    free(res);

    res = convert_base("12345", 10, 36);
    RUN_TEST("Decimal 12345 → Base36 9IX", strcmp(res, "9IX") == 0);
    free(res);

    // === Edge cases ===
    res = convert_base("0", 10, 2);
    RUN_TEST("Zero stays zero", strcmp(res, "0") == 0);
    free(res);

    res = from_dec_to_any(LLONG_MAX, 36);
    ok = from_any_to_dec(res, 36, &num);
    RUN_TEST("LLONG_MAX round-trips in base36", ok && num == LLONG_MAX);
    free(res);

    res = from_dec_to_any(LLONG_MIN, 2);
    ok = from_any_to_dec(res, 2, &num);
    RUN_TEST("LLONG_MIN round-trips in base2", ok && num == LLONG_MIN);
    free(res);

    // === Invalid inputs ===
    ok = from_any_to_dec("123", 1, &num);
    RUN_TEST("Reject base < 2", !ok);

    ok = from_any_to_dec("123", 37, &num);
    RUN_TEST("Reject base > 36", !ok);

    ok = from_any_to_dec("XYZ", 10, &num);
    RUN_TEST("Reject invalid digit in base10", !ok);

    ok = from_any_to_dec("999999999999999999999999999999", 10, &num);
    RUN_TEST("Reject overflow input", !ok);

    // === Summary ===
    printf("\n===== TEST SUMMARY =====\n");
    printf("Total: %d | Passed: %d | Failed: %d\n",
           total, total - failed, failed);

    return failed ? 1 : 0;
}
