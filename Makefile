CC = gcc
CFLAGS = -s -Os -static -Iinclude -Ilibs/tJSON/include -flto -fno-exceptions -ffunction-sections -fdata-sections -Wl,--gc-sections
SRC = src/main.c
OUT = cmatrix.exe

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)