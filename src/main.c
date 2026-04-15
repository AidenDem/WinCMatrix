// WinCMatrix@v1.2.1
// Made by AidenDem

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

// Byte sizes for ANSI & Chars per Cell
#define ANSI_CELL_SIZE 29
#define CHAR_CELL_SIZE 1

// Macros
#define TRAIL(i, j) matrix.trailChars[(i) * matrix.consoleSize.y + (j)]

// Console Handle
HANDLE hConsole = NULL;

// Variables
bool running = true;
int delay = DEFAULT_DELAY;
int text_r = DEFAULT_COLOR_R, text_g = DEFAULT_COLOR_G, text_b = DEFAULT_COLOR_B;
int mintrail = DEFAULT_MINTRAIL, maxtrail = DEFAULT_MAXTRAIL;
bool sideway = DEFAULT_SIDEWAY, color = DEFAULT_COLOR_ENABLED;

const char* activeCharset;
int charsetLength = 0;

int seed = -1;

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

typedef struct {
    const char* set_name;
    const char* set_values;
} CharsetOption;

typedef struct {
    int x;
    int y;
} Vector2;

typedef struct {
    int drop;
    int trail_length;
} Column;

typedef struct {
    Column *columns;
    char *trailChars;
    char *frameBuffer;
    Vector2 consoleSize;
    int bufferSize;
} Matrix;

// Commands
const IntOption int_options[] = {
    {"-d", "--delay", &delay},
    {"-m", "--mintrail", &mintrail},
    {"-M", "--maxtrail", &maxtrail},
    {"-s", "--seed", &seed},
    {NULL, NULL, NULL}
};

const BoolOption bool_options[] = {
    {"-S", "--sideway", &sideway},
    {"-C", "--color", &color},
    {NULL, NULL, NULL}
};

// Default Charsets
const CharsetOption charset_options[] = {
    {"ascii","ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"},
    {"binary","01"},
    {"hex","0123456789ABCDEF"},
    {"number","0123456789"},
    {"punctuation","!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"},
    {"katakana","ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝ"},
    {NULL, NULL}
};

// Functions
Vector2 getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return (Vector2){csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
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
    printf("\033[?1049l");
    running = false;
}

// Used for sanitizing RGB input
inline int clamp255(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

void parseParameters(int argc, char *argv[]) {
    // Parses all parameters given in the command line
    // Example: cmatrix.exe -delay 100 -textcolor (0,255,0) -mintrail 5 -maxtrail 10 -sideway false -color true

    for (int i = 1; i < argc; i++) {
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
            parseColor(argv[++i]);
        } else if ((strcmp(argv[i], "--charset") == 0 || strcmp(argv[i], "-ch") == 0) && i + 1 < argc) {
            const char* arg = argv[++i];
            bool found_match = false;
            for (int j = 0; charset_options[j].set_name != NULL; j++) {
                if (strcmp(arg, charset_options[j].set_name) == 0) {
                    activeCharset = charset_options[j].set_values;
                    found_match = true;
                    break;
                }
            }
            if (!found_match) {
                if (strlen(arg) > 0) {
                    activeCharset = arg;
                }
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf(
                "Usage: cmatrix.exe [options]\n"
                "Options:\n"
                "  -d, --delay <value>             Set the speed of the animation (ms)\n"
                "  -c, --textcolor (r,g,b)         Set the text color using RGB values\n"
                "  -m, --mintrail <value>          Minimum trail length\n"
                "  -M, --maxtrail <value>          Maximum trail length\n"
                "  -S, --sideway true|false        Enable or disable sideways movement\n"
                "  -C, --color true|false          Enable or disable color output\n"
                "  -h, --help                      Display this help message\n"
                "  -ch, --charset <name|custom>    Set character set: ascii | binary | katakana | <custom>\n"
                "  -s, --seed <value>              Sets the seed of the effect\n"
                "\nMade by AidenDem (https://github.com/AidenDem)\n"
                "Copyright (c) 2025-2026 AidenDem\n"
                "Licensed under the MIT License\n"
            );
            exit(0);
        }

        parsed: continue;
    }
}

void initializeMatrix(Matrix *matrix, int cellSize) {
    srand(seed); // Ensures it remains consistent

    matrix->consoleSize = getConsoleSize();
    matrix->bufferSize = matrix->consoleSize.y * matrix->consoleSize.x * cellSize + 2;
    matrix->columns = malloc(matrix->consoleSize.x * sizeof(Column));
    matrix->trailChars = malloc(matrix->consoleSize.x * matrix->consoleSize.y * sizeof(char));
    matrix->frameBuffer = malloc(matrix->bufferSize);

    // Malloc allocation validation
    if (!matrix->columns || !matrix->trailChars || !matrix->frameBuffer) exit(1);

    for (int i = 0; i < matrix->consoleSize.x; i++) {
        matrix->columns[i].drop = rand() % matrix->consoleSize.y;
        matrix->columns[i].trail_length = rand() % (maxtrail - mintrail + 1) + mintrail; // Random trail length between mintrail and maxtrail
        for (int j = 0; j < matrix->consoleSize.y; j++) {
           matrix->trailChars[i * matrix->consoleSize.y + j] = ' ';
        }
    }
}

void freeMatrix(Matrix *matrix) {
    free(matrix->columns);
    free(matrix->trailChars);
    free(matrix->frameBuffer);
}

// Main Program
int main(int argc, char *argv[]) {
    // Ensures to set a charset if they didnt pass one in parameters
    activeCharset = charset_options[0].set_values;

    parseParameters(argc, argv); // Parse command-line arguments
    charsetLength = strlen(activeCharset);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get console handle

    // Switch to alternate buffer
    printf("\033[?1049h");

    // Prepare the console
    toggleCursor(false);

    // Handle SIGINT signal, ensures graceful exit
    signal(SIGINT, handleSigint);

    // Check if a seed was provided, if not set seed
    if (seed == -1) {
        seed = (int)time(NULL);
    }

    // Input Sanitizing
    text_r = clamp255(text_r);
    text_g = clamp255(text_g);
    text_b = clamp255(text_b);

    // Cell Size
    int cellSize = color ? ANSI_CELL_SIZE : CHAR_CELL_SIZE;

    // Initialize Matrix
    Matrix matrix;
    initializeMatrix(&matrix, cellSize);

    DWORD written;

    // Main Loop
    while (running) {
        // Check for matrix resize
        Vector2 newSize = getConsoleSize();
        if (newSize.x != matrix.consoleSize.x || newSize.y != matrix.consoleSize.y) {
            freeMatrix(&matrix);
            initializeMatrix(&matrix, cellSize);
        }

        // Last Color
        int last_r = -1, last_g = -1, last_b = -1;

        // Buffer position
        int pos = 0;
        pos += snprintf(matrix.frameBuffer + pos, matrix.bufferSize - pos, "\033[H"); // Move cursor to top-left

        // Double for loop to iterate through matrix.consoleSize.y and matrix.consoleSize.x
        for (int i = 0; i < matrix.consoleSize.y; i++) {
            // If sideways, slightly offset the trails resulting in a horizontal effect
            for (int j = sideway ? 1 : 0; j < matrix.consoleSize.x; j++) {
                // Gets current trail
                int drop = matrix.columns[j].drop;
                int trail_length = matrix.columns[j].trail_length;
                int d = (drop - i + matrix.consoleSize.y) % matrix.consoleSize.y;

                if (d < trail_length) {
                    // Assigns color based on distance from the top character of the trail
                    // The further away, the darker the color
                    double factor = 1.0 - ((double)d / trail_length);
                    int r_col = (int)(text_r * factor);
                    int g_col = (int)(text_g * factor);
                    int b_col = (int)(text_b * factor);

                    // Generate random character if top of trail
                    if (d == 0) {
                        TRAIL(j, i) = randChar();
                    }
                    
                    // Update color if it has been changed
                    if (color) {
                        if (r_col != last_r || g_col != last_g || b_col != last_b) {
                            last_r = r_col;
                            last_g = g_col;
                            last_b = b_col;
                            pos += snprintf(matrix.frameBuffer + pos, matrix.bufferSize - pos, "\033[1;38;2;%d;%d;%dm", r_col, g_col, b_col);
                        }
                    }
                    
                    // Write character to buffer
                    matrix.frameBuffer[pos++] = TRAIL(j, i);
                } else {
                    matrix.frameBuffer[pos++] = ' ';
                }
            }
        }
        pos += snprintf(matrix.frameBuffer + pos, matrix.bufferSize - pos, "\033[0m\0");

        // Flush the buffer to the console
        WriteConsoleA(hConsole, matrix.frameBuffer, pos, &written, NULL);

        // Iterate through columns
        for (int i = 0; i < matrix.consoleSize.x; i++) {
            matrix.columns[i].drop = (matrix.columns[i].drop + 1) % matrix.consoleSize.y; // Move trail down by 1 row
        }

        // Wait before next frame
        Sleep(delay);
    }

    // Free allocated memory
    freeMatrix(&matrix);

    return 0;
}
