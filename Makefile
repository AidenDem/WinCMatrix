CC := gcc
CFLAGS := -s -Os -static -Iinclude -Ilibs/tJSON/include -flto -fno-exceptions -ffunction-sections -fdata-sections -Wl,--gc-sections

SRC_DIR := src
SRC := $(SRC_DIR)/main.c
OUT := cmatrix.exe

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	$(RM) $(OUT)
