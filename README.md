# WinCMatrix
Remade version of CMatrix for Windows in C.  
## Dependencies
- GCC
- Makefile (Or just compile directly using GCC)
## Usage
### Compiling
- Using makefile
```bash
make all
```
- Using GCC
```bash
gcc -O2 src/main.c -o cmatrix.exe
```
### Using the program
- Format
```bash
cmatrix.exe -delay (delay in ms) -textcolor (r,g,b)
```
- Example
```bash
cmatrix.exe -delay 100 -textcolor "(0,255,0)"
```