#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h> 
#include <signal.h>

#define DEFAULT_SPEED 100
#define DEFAULT_COLOR 0, 255, 0

int cols, rows;
int speed = DEFAULT_SPEED;
int text_r = 0, text_g = 255, text_b = 0;

void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

char randChar() {
    return 33 + (rand() % 94);
}

void parseColor(char *arg) {
    sscanf(arg, "(%d,%d,%d)", &text_r, &text_g, &text_b);
}

void toggleCursor(bool toggle) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
    cursorInfo.bVisible = toggle;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

void handleSigint(int sig) {
    toggleCursor(true);
    system("cls");
    exit(0);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    getConsoleSize();
    system("cls");
    toggleCursor(false);

    signal(SIGINT, handleSigint);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-speed") == 0 && i + 1 < argc) {
            speed = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-textcolor") == 0 && i + 1 < argc) {
            parseColor(argv[i + 1]);
        }
    }

    int *drops = (int *)malloc(cols * sizeof(int));
    for (int i = 0; i < cols; i++) {
        drops[i] = rand() % rows;
    }

    while (1) {
        for (int i = 0; i < cols; i++) {
            int drop = drops[i];

            int head  = drop;
            int trail = (drop - 1 + rows) % rows;
            int fade  = (drop - 2 + rows) % rows;

            setCursorPosition(i, head);
            printf("\033[1;38;2;%d;%d;%dm%c\033[0m", text_r, text_g, text_b, randChar());

            setCursorPosition(i, trail);
            printf("\033[0;38;2;%d;%d;%dm%c\033[0m", text_r / 2, text_g / 2, text_b / 2, randChar());

            setCursorPosition(i, fade);
            printf(" ");

            drops[i] = (drop + 1) % rows;
            if (rand() % 10 > 8) {
                drops[i] = rand() % rows;
            }
        }
        fflush(stdout);
        Sleep(speed);
    }

    free(drops);
    return 0;
}
