# Py2Cpp

**Py2Cpp** is a python compiler developed using C++. It focuses on implementing the **lexical analysis** and **syntax analysis** phases of a compiler. This project includes a fully functional GUI, a symbol table, error handling mechanisms, and parse tree generation.

> This project was created as part of the **Design of Compilers** course at Ain Shams University, Spring 2025.



## Features

- Lexical analysis with token generation
- Syntax analysis using recursive descent parser
- Symbol table with GUI view
- Error handling (lexical and syntactic)
- Parse tree visualization (Graphviz integration)
- Modern C++ with Qt-based GUI

## Prerequisites
Ensure the following dependencies are installed on your system:

- **C++ Compiler** supporting C++20
- **CMake** version 3.16 or higher
- **Qt** 5 or 6 development libraries
- **Graphviz** (for parse tree visualization)

> 🛠️ On Debian-based systems:
> ```bash
> sudo apt install build-essential cmake qtbase5-dev graphviz
> ```

> 🛠️ On Windows:
> - Install [Qt](https://www.qt.io/download)
> - Install [CMake](https://cmake.org/download/)
> - Install [Graphviz](https://graphviz.org/download/)
> - Use MSYS2/MinGW or integrate CMake with your IDE (e.g., CLion or Visual Studio)

## Installation

Clone the repository
```bash
git clone https://github.com/ysif9/Python-Compiler.git
cd Python-Compiler
```
Create a build directory
```bash
mkdir build && cd build
```
Configure the project
```bash
cmake ..
```
Build the executable
```bash
make
```
Run the application
```bash
./Python-Compiler
```

## Screenshots

- [ ] Add Screenshots

## Authors

- Yousif Abdulhafiz - [@ysif9](https://github.com/ysif9)
- Philopater Guirgis - [@Philodoescode](https://github.com/Philodoescode)
- Hams Hassan - [@Hams2305](https://github.com/Hams2305)
- Ahmed Lotfy - [@dark-hunter0](https://github.com/dark-hunter0)
- Jana Sameh - [@janasameh7](https://github.com/janasameh7)
- Ahmed Al-amin - [@Ahmed-Al-amin](https://github.com/Ahmed-Al-amin)

