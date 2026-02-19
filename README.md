# 🔢 Universal Number Base Converter in C

A robust, **command-line number base converter** written in C.  
This program converts **64-bit signed integers** between any base from **2 to 36**, using digits `0-9` and letters `A-Z`.  

Built with a strong focus on **memory safety, efficiency, and robust error handling**. 

---

## ✨ Features
- 🔄 **Universal Base Support** → Convert numbers between **any base 2–36**  
- 🖥️ **64-Bit Integer Handling** → Supports the full range of `long long` (`LLONG_MIN` to `LLONG_MAX`)  
- 🚫 **Robust Error Handling** → Detects invalid inputs, empty strings, out-of-range bases, and overflow  
- 📦 **Dynamic Memory Management** → No fixed-size buffers; input grows dynamically to any length  
- 🛡️ **Overflow Protection** → Explicit checks prevent undefined behavior on large conversions  
- ✅ **Memory Safe** → Every `malloc` / `realloc` has a matching `free`, with **zero leaks**  

---

## 🚀 Getting Started

### 📋 Prerequisites
- A C compiler (e.g., **GCC** or **Clang**)  
- The `make` build tool (recommended for simplicity)  

### ⚡ Build & Run

1. **Clone the repository**:
```bash
   git clone https://github.com/Samiul946/C-Number-Base-Converter.git
   cd C-Number-Base-Converter


**Run the interactive converter**:
```bash
./build/converter
```

## 🧠 Technical Highlights

### 🔒 Memory Safety by Design
This project takes a **defensive approach** to memory:

- **Dynamic Input Buffers** → Input grows automatically with `realloc` as the user types, preventing overflow. After input, the buffer is shrunk to the exact required size.  
- **Precise Output Allocation** → Results are first calculated in a temporary stack buffer, then copied into a perfectly-sized heap allocation. No wasted memory, no risk of writing past bounds.  
- **Strict Ownership Model** → Each heap allocation has a clear owner. `main()` always frees what it allocates, ensuring **zero leaks**.  

---

### ⚖️ Correctness at Integer Limits
Special care is taken with **edge cases like `LLONG_MIN`**:

- Direct negation of `LLONG_MIN` is **undefined behavior in C**.  
- To avoid this, conversions use `unsigned long long` intermediates.  
- This ensures correctness across the full signed 64-bit range.  

---

### 🛡️ Robust Error Handling
- Invalid bases (`<2` or `>36`) are rejected immediately  
- Non-digit/letter characters in input cause a graceful error  
- Overflow during conversion is detected explicitly and reported

---

### 📜 License

This project is licensed under the GNU Affero General Public License v3.0 - see the `LICENSE` file for details.

---
