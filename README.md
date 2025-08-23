# C Universal Number Base Converter

A robust, command-line number base converter written in C. This program converts 64-bit signed integers between any base from 2 to 36, using digits `0-9` and letters `A-Z`.



## Features

- **Universal Base Support**: Converts numbers between any base from 2 to 36.
- **64-Bit Signed Integers**: Handles the full range of `long long`, from `LLONG_MIN` to `LLONG_MAX`.
- **Robust Error Handling**: Gracefully manages invalid input, empty strings, and numbers that are too large, with clear error messages.
- **Dynamic Memory Management**: Safely handles input of any length, preventing buffer overflows.
- **Overflow Protection**: Includes explicit checks to detect and prevent integer overflows during conversion.

## Getting Started

### Prerequisites

- A C compiler (like GCC or Clang)
- `make` (optional, but recommended)

### Compilation & Execution

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/Samiul946/C-Number-Base-Converter.git](https://github.com/Samiul946/C-Number-Base-Converter.git)
    cd C-Number-Base-Converter
    ```

2.  **Compile the program:**
    *Using Makefile (recommended):*
    ```bash
    make
    ```
    *Using GCC manually:*
    ```bash
    gcc -Wall -Wextra -std=c11 -o converter converter.c
    ```

3.  **Run the converter:**
    ```bash
    ./converter
    ```

## Technical Challenges

A key challenge in this project was ensuring mathematical correctness at the limits of the 64-bit integer range. Specifically, handling `LLONG_MIN` required careful attention, as negating it (`-LLONG_MIN`) causes a signed integer overflow, which is undefined behavior in C.

The solution was to perform all intermediate calculations on `unsigned long long` and then explicitly check if the final value corresponded to `LLONG_MIN` before applying the negative sign, thus avoiding the undefined operation entirely.

## License

This project is licensed under the MIT License - see the `LICENSE` file for details.
