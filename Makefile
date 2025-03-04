CC = gcc
CFLAGS = -O2
SRC = src/main.c
OUT = cmatrix.exe

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)