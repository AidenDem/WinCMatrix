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
#define DEFAULT_MINTRAIL 4
#define DEFAULT_MAXTRAIL 12
#define DEFAULT_SIDEWAY false
#define DEFAULT_COLOR_ENABLED true

#define CHARSET_ASCII "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#define CHARSET_BINARY "01"
#define CHARSET_KATAKANA "ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝ"

// Console Handle
HANDLE hConsole = NULL;

// Variables
int cols, rows;
int delay = DEFAULT_DELAY;
int text_r = DEFAULT_COLOR_R, text_g = DEFAULT_COLOR_G, text_b = DEFAULT_COLOR_B;
int mintrail = DEFAULT_MINTRAIL, maxtrail = DEFAULT_MAXTRAIL;
bool stopmidway = DEFAULT_STOPMIDWAY, sideway = DEFAULT_SIDEWAY, color = DEFAULT_COLOR_ENABLED;
int i,j;

const char* activeCharset = CHARSET_ASCII;
int charsetLength = 0;

// Structs
typedef struct {
    const char* short_opt;
    const char* long_opt;
    int* target;
} IntOption;

typedef struct {
    const char* short_opt;
    const char* long_opt;
    bool* target;
} BoolOption;

// Commands
IntOption int_options[] = {
    {"-d", "--delay", &delay},
    {"-m", "--mintrail", &mintrail},
    {"-M", "--maxtrail", &maxtrail},
    {NULL, NULL, NULL}
};

BoolOption bool_options[] = {
    {"-s", "--stopmidway", &stopmidway},
    {"-S", "--sideway", &sideway},
    {"-C", "--color", &color},
    {NULL, NULL, NULL}
};

// Functions
void getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

char randChar() {
    // Returns a random printable character out of charset
    return activeCharset[rand() % charsetLength];
}

void parseColor(const char *arg) {
    // Parses string in format "(r,g,b)" to RGB color values
    sscanf_s(arg, "(%d,%d,%d)", &text_r, &text_g, &text_b);
}

bool strToBool(const char *str) {
    // Parses string to boolean value
    if (str == NULL) return false;
    if (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 || strcmp(str, "yes") == 0 || strcmp(str, "on") == 0) {
        return true;
    }
    return false;
}

void toggleCursor(bool toggle) {
    // Toggles the cursor visibility
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = toggle;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void handleSigint(int sig) {
    // Handles SIGINT signal
    toggleCursor(true);
    printf("\033[2J\033[H"); // Cleans up matrix
    exit(0);
}

void parseParameters(int argc, char *argv[]) {
    // Parses all parameters given in the command line
    // Example: cmatrix.exe -delay 100 -textcolor (0,255,0) -stopmidway true -mintrail 5 -maxtrail 10 -sideway false -color true

    for (i = 1; i < argc; i++) {
        // Int Flags
        for (int j = 0; int_options[j].short_opt != NULL; j++) {
            if ((strcmp(argv[i], int_options[j].short_opt) == 0 || strcmp(argv[i], int_options[j].long_opt) == 0) && i + 1 < argc) {
                *(int_options[j].target) = atoi(argv[++i]);

                // Get out of both loops
                goto parsed;
            }
        }

        // Bool Flags
        for (int j = 0; bool_options[j].short_opt != NULL; j++) {
            if ((strcmp(argv[i], bool_options[j].short_opt) == 0 || strcmp(argv[i], bool_options[j].long_opt) == 0) && i + 1 < argc) {
                *(bool_options[j].target) = strToBool(argv[++i]);

                // Get out of both loops
                goto parsed;
            }
        }

        // Other cases
        if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--textcolor") == 0) && i + 1 < argc) {
            parseColor(argv[i + 1]);
        } else if ((strcmp(argv[i], "--charset") == 0 || strcmp(argv[i], "-ch") == 0) && i + 1 < argc) {
            const char* arg = argv[++i];
            if (strcmp(arg, "ascii") == 0) {
                activeCharset = CHARSET_ASCII;
            } else if (strcmp(arg, "binary") == 0) {
                activeCharset = CHARSET_BINARY;
            } else if (strcmp(arg, "katakana") == 0) {
                activeCharset = CHARSET_KATAKANA;
            } else {
                activeCharset = arg;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf(
                "Usage: cmatrix.exe [options]\n"
                "Options:\n"
                "  -d, --delay <value>             Set the speed of the animation (ms)\n"
                "  -c, --textcolor (r,g,b)         Set the text color using RGB values\n"
                "  -s, --stopmidway true|false     Enable or disable text stopping midway\n"
                "  -m, --mintrail <value>          Minimum trail length\n"
                "  -M, --maxtrail <value>          Maximum trail length\n"
                "  -S, --sideway true|false        Enable or disable sideways movement\n"
                "  -C, --color true|false          Enable or disable color output\n"
                "  -h, --help                      Display this help message\n"
                "  -ch, --charset <name|custom>    Set character set: ascii | binary | katakana | <custom>\n"
                "Made by AidenDem (https://github.com/AidenDem)\n"
                "\nCopyright (c) 2025 AidenDem\n"
                "Licensed under the MIT License\n"
            );
            exit(0);
        }

        parsed: continue;
    }
}

// Main Program
int main(int argc, char *argv[]) {
    parseParameters(argc, argv); // Parse command-line arguments
    charsetLength = strlen(activeCharset);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get console handle

    // Prepare the console
    srand(time(NULL));
    getConsoleSize();
    system("cls");
    toggleCursor(false);

    // Handle SIGINT signal, ensures graceful exit
    signal(SIGINT, handleSigint);

    // Initialize trail positions and lengths for each column
    int drops[cols];
    int trail_lengths[cols];
    char trailChars[cols][rows];
    for (i = 0; i < cols; i++) {
        drops[i] = rand() % rows;
        trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail; // Random trail length between mintrail and maxtrail
        for (j = 0; j < rows; j++) {
            trailChars[i][j] = ' ';
        }
    }

    // Frame buffer assignment
    int maxCellSize = 25; // Estimated maximum size of a single cell's output (including ANSI codes, cannot be fully accurate due to dynamic user input)
    int maxFrameSize = rows * cols * maxCellSize;
    char frameBuffer[maxFrameSize];

    DWORD written;

    // Main Loop
    while (1) {
        // Buffer position
        int pos = 0;
        pos += sprintf(frameBuffer + pos, "\033[H"); // Move cursor to top-left

        // Double for loop to iterate through rows and cols
        for (i = 0; i < rows; i++) {
            // If sideways, slightly offset the trails resulting in a horizontal effect
            for (j = sideway ? 1 : 0; j < cols; j++) {
                // Gets current trail
                int drop = drops[j];
                int trail_length = trail_lengths[j];
                int d = (drop - i + rows) % rows;

                if (d < trail_length) {
                    // Assigns color based on distance from the top character of the trail
                    // The further away, the darker the color
                    double factor = 1.0 - ((double)d / trail_length);
                    int r_col = (int)(text_r * factor);
                    int g_col = (int)(text_g * factor);
                    int b_col = (int)(text_b * factor);

                    // Top of the trail: generate a new character
                    if (d == 0) {
                        trailChars[j][i] = randChar(); // Assign random character for first

                        // Assigns character to buffer
                        // ANSI codes if color enabled
                        if (color) {
                            pos += sprintf(frameBuffer + pos, "\033[1;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, trailChars[j][i]);
                        } else {
                            pos += sprintf(frameBuffer + pos, "%c", trailChars[j][i]);
                        }
                    } else {
                        // Same principle as above
                        // Just a slightly different formula to get the current trail character
                        if (color) {
                            pos += sprintf(frameBuffer + pos, "\033[0;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, trailChars[j][(i - 1 + rows) % rows]);
                        } else {
                            pos += sprintf(frameBuffer + pos, "%c", trailChars[j][(i - 1 + rows) % rows]);
                        }
                    }
                } else {
                    frameBuffer[pos++] = ' ';
                }
            }
        }
        frameBuffer[pos] = '\0';

        // Flush the buffer to the console
        WriteConsoleA(hConsole, frameBuffer, pos, &written, NULL);

        // Iterate through columns
        for (i = 0; i < cols; i++) {
            drops[i] = (drops[i] + 1) % rows; // Move trail down by 1 row
            // If stopmidway is enabled and a random condition is met, reset the trail
            if (stopmidway && (rand() % 100 > 10)) {
                drops[i] = rand() % rows;
                trail_lengths[i] = rand() % (maxtrail - mintrail + 1) + mintrail;
            }
        }

        // Wait before next frame
        Sleep(delay);
    }

    return 0;
}
