# TINY Language Compiler - Assignment 2

## Overview
This project implements a compiler for the TINY language with additional data types and operators. The compiler supports integer, real, and boolean types, along with enhanced arithmetic and comparison operations.

## Changes Made

### 1. New Operators Added
- **Comparison Operators**: `>`, `>=`, `<=`
  - These operators compare two numeric values (int or real) and return a boolean result.
  - Example: `flag := 3 > 5;` assigns `false` to `flag`.

- **Arithmetic & Operator**: `&`
  - Computes `a^2 - b^2` where a and b are numeric values.
  - Example: `x := 3 & 2;` assigns `5` to `x` (9 - 4 = 5).

### 2. Removed Features
- **Logical OR Operator**: `|` has been completely removed from the language.
- **Boolean AND Operator**: The logical AND operation for booleans has been removed. The `&` operator now performs arithmetic operations only.

### 3. Type System Enhancements
- **Strict Type Rules**:
  - No arithmetic operations on boolean values.
  - No assignments between incompatible types (e.g., cannot assign boolean to integer variable).
  - Comparison operators return boolean values.
  - Arithmetic operators work on int and real types.

- **Error Handling**:
  - Compile-time type checking prevents invalid operations.
  - Descriptive error messages for type mismatches:
    - "Cannot perform arithmetic on boolean values"
    - "Type mismatch in assignment: expected [type], got [type]"

### 4. Code Changes

#### TokenType Enum Updates
```cpp
enum TokenType {
    // ... existing tokens ...
    GREATER_THAN,    // >
    GREATER_EQUAL,   // >=
    LESS_EQUAL,      // <=
    // Removed: OR_OP
};
```

#### Parser Updates
- `Expr()` function now handles comparison operations with proper precedence.
- Removed `OrExpr()` function.
- `Term()` uses `AndExpr()` for precedence handling.

#### Analyzer Updates
- Comparison operators (`>`, `>=`, `<=`) return `BOOLEAN` type.
- `&` operator is treated as arithmetic (returns numeric type).
- Added type checks to prevent boolean arithmetic and invalid assignments.

#### Evaluator Updates
- Implemented evaluation logic for new comparison operators.
- `&` operator computes `pow(a, 2) - pow(b, 2)`.

### 5. Test Program (input.txt)
The test program includes 20+ statements demonstrating:
- Variable declarations with different types
- Arithmetic operations (+, -, *, /, ^, &)
- Comparison operations (<, =, >, >=, <=)
- Conditional statements (if, repeat)
- Write statements with expressions
- Type safety enforcement

### 6. Sample Output
```
Start main()
Symbol Table:
[Var=x][Mem=0][Line=1]
[Var=y][Mem=1][Line=2]
[Var=flag][Mem=2][Line=3]
---------------------------------
Syntax Tree:
[Decl][x][Integer]
   [Num][5][Integer]
...
---------------------------------
Run Program:
Val: 115
Val: 3.500000
Val: false
Val: true
Val: true
Val: true
Val: 5
---------------------------------
End main()
```

## Usage
1. Compile the compiler: `g++ myfile.cpp -o myfile.exe`
2. Prepare your TINY program in `input.txt`
3. Run: `myfile.exe < input.txt > output.txt`

## Files
- `myfile.cpp`: Main compiler source code
- `input.txt`: Test program demonstrating all features
- `output.txt`: Compiler output (syntax tree and execution results)
- `README.md`: This documentation file

## Notes
- The language maintains backward compatibility with existing TINY programs.
- All type checking is performed at compile-time to ensure runtime safety.
- Boolean values cannot be used in arithmetic expressions.
- Mixed-type arithmetic (int + real) is supported with automatic promotion.