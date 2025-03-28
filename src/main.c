// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h> 
#include <signal.h>

// Constants
// Default Settings
#define DEFAULT_DELAY 100
#define DEFAULT_COLOR_R 0
#define DEFAULT_COLOR_G 255
#define DEFAULT_COLOR_B 0
#define DEFAULT_STOPMIDWAY false
#define DEFAULT_MINTRAIL 3
#define DEFAULT_MAXTRAIL 8
#define DEFAULT_SIDEWAY false

// Console Handle
HANDLE hConsole = NULL;

// Variables
int cols, rows;
int delay = DEFAULT_DELAY;
int text_r = DEFAULT_COLOR_R, text_g = DEFAULT_COLOR_G, text_b = DEFAULT_COLOR_B;
int mintrail = DEFAULT_MINTRAIL, maxtrail = DEFAULT_MAXTRAIL;
bool stopmidway = DEFAULT_STOPMIDWAY, sideway = DEFAULT_SIDEWAY, help = false;
int i,j;

// Functions
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
    if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 || strcmp(str, "yes") == 0 || strcmp(str, "on") == 0) {
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
    printf("\033[2J\033[H");
    exit(0);
}

void parseParameters(int argc, char *argv[]) {
    for (i = 1; i < argc; i++) {
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
        } else if (strcmp(argv[i], "-sideway") == 0 && i + 1 < argc) {
            sideway = strToBool(argv[i + 1]);
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: cmatrix.exe [options]\n"
                "Options:\n"
                "  -delay <value>               Set the speed of the animation (lower = faster).\n"
                "  -textcolor (r,g,b)           Set the text color using RGB values.\n"
                "  -stopmidway <true/false>     Enable or disable text stopping midway.\n"
                "  -mintrail <value>            Set the minimum length of falling trails.\n"
                "  -maxtrail <value>            Set the maximum length of falling trails.\n"
                "  -sideway <true/false>        Enable or disable sideways text movement.\n"
                "  --help                       Display this help message.\n");         
            help = true;
        }
    }
}

// Main Program
int main(int argc, char *argv[]) {
    parseParameters(argc, argv);
    if (help) return 0;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    srand(time(NULL));
    getConsoleSize();
    system("cls");
    toggleCursor(false);

    signal(SIGINT, handleSigint);

    int drops[cols];
    int trail_lengths[cols];
    char trailChars[cols][rows];
    for (i = 0; i < cols; i++) {
        drops[i] = rand() % rows;
        trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail;
        for (j = 0; j < rows; j++) {
            trailChars[i][j] = ' ';
        }
    }

    int maxCellSize = 25;
    int maxFrameSize = rows * cols * maxCellSize;
    char frameBuffer[maxFrameSize];

    DWORD written;

    // Main Loop
    while (1) {
        int pos = 0;
        pos += sprintf(frameBuffer + pos, "\033[H");
        
        for (i = 0; i < rows; i++) {
            for (j = sideway ? 1 : 0; j < cols; j++) {
                int drop = drops[j];
                int trail_length = trail_lengths[j];
                int d = (drop - i + rows) % rows;

                if (d < trail_length) {
                    double factor = 1.0 - ((double)d / trail_length);
                    int r_col = (int)(text_r * factor);
                    int g_col = (int)(text_g * factor);
                    int b_col = (int)(text_b * factor);

                    if (d == 0) {
                        trailChars[j][i] = randChar();
                        pos += sprintf(frameBuffer + pos, "\033[1;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, trailChars[j][i]);
                    } else {
                        pos += sprintf(frameBuffer + pos, "\033[0;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, trailChars[j][(i - 1 + rows) % rows]);
                    }
                } else {
                    frameBuffer[pos++] = ' ';
                }
            }
        }
        frameBuffer[pos] = '\0';

        WriteConsoleA(hConsole, frameBuffer, pos, &written, NULL);

        for (i = 0; i < cols; i++) {
            drops[i] = (drops[i] + 1) % rows;
            if (stopmidway && (rand() % 100 > 10)) {
                drops[i] = rand() % rows;
                trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail;
            }
        }
        Sleep(delay);
    }

    return 0;
}