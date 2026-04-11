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

// Byte sizes for ANSI & Chars
#define ANSI_CELL_SIZE 29
#define CHAR_CELL_SIZE 1

// Macros
#define TRAIL(i, j) trailChars[(i) * consoleSize.y + (j)]

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
    printf("\033[2J\033[H"); // Cleans up matrix
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
    // Ensures to set a charset if they didnt pass one in parameters
    activeCharset = charset_options[0].set_values;

    parseParameters(argc, argv); // Parse command-line arguments
    charsetLength = strlen(activeCharset);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get console handle

    // Prepare the console
    srand(time(NULL));
    system("cls");
    toggleCursor(false);

    // Get Console Size
    Vector2 consoleSize = getConsoleSize();

    // Handle SIGINT signal, ensures graceful exit
    signal(SIGINT, handleSigint);

    // Check if a seed was provided, if not generate random seed
    if (seed == -1) {
        seed = rand();
    }
    srand(seed);

    // Initialize trail positions and lengths for each column
    Column *columns = malloc(consoleSize.x * sizeof(Column));
    char *trailChars = malloc(consoleSize.x * consoleSize.y * sizeof(char));

    // Malloc allocation validation
    if (!columns || !trailChars) exit(1);

    for (int i = 0; i < consoleSize.x; i++) {
        columns[i].drop = rand() % consoleSize.y;
        columns[i].trail_length = rand() % (maxtrail - mintrail + 1) + mintrail; // Random trail length between mintrail and maxtrail
        for (int j = 0; j < consoleSize.y; j++) {
           TRAIL(i,j) = ' ';
        }
    }

    // Input Sanitizing
    text_r = clamp255(text_r);
    text_g = clamp255(text_g);
    text_b = clamp255(text_b);

    // Frame buffer assignment
    int maxCellSize = color ? ANSI_CELL_SIZE : CHAR_CELL_SIZE; // Estimated maximum size of a single cell's output (including ANSI codes, cannot be fully accurate due to dynamic user input)
    int maxFrameSize = consoleSize.y * consoleSize.x * maxCellSize + 2;
    char *frameBuffer = malloc(maxFrameSize);

    // Malloc allocation validation
    if (!frameBuffer) exit(1);

    DWORD written;

    // Main Loop
    while (running) {
        // Buffer position
        int pos = 0;
        pos += snprintf(frameBuffer + pos, maxFrameSize - pos, "\033[H"); // Move cursor to top-left

        // Double for loop to iterate through consoleSize.y and consoleSize.x
        for (int i = 0; i < consoleSize.y; i++) {
            // If sideways, slightly offset the trails resulting in a horizontal effect
            for (int j = sideway ? 1 : 0; j < consoleSize.x; j++) {
                // Gets current trail
                int drop = columns[j].drop;
                int trail_length = columns[j].trail_length;
                int d = (drop - i + consoleSize.y) % consoleSize.y;

                if (d < trail_length) {
                    // Assigns color based on distance from the top character of the trail
                    // The further away, the darker the color
                    double factor = 1.0 - ((double)d / trail_length);
                    int r_col = (int)(text_r * factor);
                    int g_col = (int)(text_g * factor);
                    int b_col = (int)(text_b * factor);

                    // Top of the trail: generate a new character
                    if (d == 0) {
                        // Assigns random character
                        TRAIL(j,i) = randChar();

                        // Assigns character to buffer
                        // ANSI codes if color enabled
                        if (color) {
                            pos += snprintf(frameBuffer + pos, maxFrameSize - pos, "\033[1;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, TRAIL(j,i));
                        } else {
                            frameBuffer[pos++] = TRAIL(j, i);
                        }
                    } else {
                        // Same principle as above
                        // Just a slightly different formula to get the current trail character
                        if (color) {
                            pos += snprintf(frameBuffer + pos, maxFrameSize - pos, "\033[0;38;2;%d;%d;%dm%c\033[0m", r_col, g_col, b_col, TRAIL(j,(i - 1 + consoleSize.y) % consoleSize.y));
                        } else {
                           frameBuffer[pos++] = TRAIL(j,(i - 1 + consoleSize.y) % consoleSize.y);
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
        for (int i = 0; i < consoleSize.x; i++) {
            columns[i].drop = (columns[i].drop + 1) % consoleSize.y; // Move trail down by 1 row
        }

        // Wait before next frame
        Sleep(delay);
    }

    free(frameBuffer);
    free(trailChars);
    free(columns);

    return 0;
}
