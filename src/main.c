// AidenDem, feel free to use & distribute

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h> 
#include <signal.h>

#define DEFAULT_DELAY 100
#define DEFAULT_COLOR_R 0
#define DEFAULT_COLOR_G 255
#define DEFAULT_COLOR_B 0
#define DEFAULT_STOPMIDWAY false
#define DEFAULT_MINTRAIL 3
#define DEFAULT_MAXTRAIL 8

HANDLE hConsole = NULL;

int cols, rows;
int delay = DEFAULT_DELAY;
int text_r = DEFAULT_COLOR_R, text_g = DEFAULT_COLOR_G, text_b = DEFAULT_COLOR_B;
int mintrail = DEFAULT_MINTRAIL, maxtrail = DEFAULT_MAXTRAIL;
bool stopmidway = DEFAULT_STOPMIDWAY;

void setCursorPosition(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(hConsole, coord);
}

void getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

char randChar() {
    return 33 + (rand() % 94);
}

void parseColor(char *arg) {
    sscanf(arg, "(%d,%d,%d)", &text_r, &text_g, &text_b);
}

bool strToBool(const char *str) {
    if (str == NULL) return false;
    if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0 || strcasecmp(str, "yes") == 0 || strcasecmp(str, "on") == 0) {
        return true;
    }
    return false;
}

void toggleCursor(bool toggle) {
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = toggle;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void handleSigint(int sig) {
    toggleCursor(true);
    system("cls");
    exit(0);
}

int main(int argc, char *argv[]) {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    srand(time(NULL));
    getConsoleSize();
    system("cls");
    toggleCursor(false);

    signal(SIGINT, handleSigint);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-delay") == 0 && i + 1 < argc) {
            delay = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-textcolor") == 0 && i + 1 < argc) {
            parseColor(argv[i + 1]);
        } else if (strcmp(argv[i], "-stopmidway") == 0 && i + 1 < argc) {
            stopmidway = strToBool(argv[i + 1]);
        } else if (strcmp(argv[i], "-mintrail") == 0 && i + 1 < argc) {
            mintrail = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-maxtrail") == 0 && i + 1 < argc) {
            maxtrail = atoi(argv[i + 1]);
        }
    }

    int *drops = (int *)malloc(cols * sizeof(int));
    int *trail_lengths = (int *)malloc(cols * sizeof(int));
    for (int i = 0; i < cols; i++) {
        drops[i] = rand() % rows;
        trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail;
    }

    DWORD written;
    char buf[128];

    while (1) {
        for (int i = 0; i < cols; i++) {
            int drop = drops[i];
            int trail_length = trail_lengths[i];

            for (int j = 0; j < trail_length; j++) {
                int pos = (drop - j + rows) % rows;
                double factor = 1.0 - ((double)j / trail_length);
                int r = (int)(text_r * factor);
                int g = (int)(text_g * factor);
                int b = (int)(text_b * factor);

                setCursorPosition(i, pos);

                if (j == 0) {
                    int len = sprintf(buf, "\033[1;38;2;%d;%d;%dm%c\033[0m", r, g, b, randChar());
                    WriteConsole(hConsole, buf, len, &written, NULL);
                } else {
                    int len = sprintf(buf, "\033[0;38;2;%d;%d;%dm%c\033[0m", r, g, b, randChar());
                    WriteConsole(hConsole, buf, len, &written, NULL);
                }
            }

            int clearPos = (drop - trail_length + rows) % rows;
            setCursorPosition(i, clearPos);
            int len = sprintf(buf, " ");
            WriteConsole(hConsole, buf, len, &written, NULL);

            drops[i] = (drop + 1) % rows;
            if (stopmidway && (rand() % 10 > 8)) {
                drops[i] = rand() % rows;
                trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail;
            }
        }
        Sleep(delay);
    }

    free(drops);
    free(trail_lengths);
    return 0;
}