// WinCMatrix@v1.4.0
// Made by AidenDem

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#define CELL(m, i, j) (m).trailCells[(i) * (m).consoleSize.y + (j)]

// Variables
HANDLE hConsole = NULL;
bool running = true;

// Structs
// Generic Structs
typedef struct {
    const char* set_name;
    const char* set_values;
} StringMap;

typedef struct {
    int x;
    int y;
} Vector2;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

// Matrix Structs
typedef struct {
    int drop;
    int trail_length;
} Column;

typedef struct {
    uint8_t bytes[4];
    uint8_t len;
} Glyph;

typedef struct {
    Glyph ch;
} Cell;

typedef struct {
    Glyph *glyphs;
    int count;
} Charset;

typedef struct {
    Column *columns;
    Cell *trailCells;
    char *frameBuffer;
    Vector2 consoleSize;
    int bufferSize;
} Matrix;

// Config Structs
typedef struct {
    const char *arg;
} ConfigTypeInfo;

typedef struct {
    const char* short_opt;
    const char* long_opt;
    const char* description;
    void (*parser)(const char* arg, void* ptr);
    void* ptr;
} ConfigField;

typedef struct {
    int delay;
    int mintrail;
    int maxtrail;
    bool sideway;
    bool color;
    Color matrixColor;
    Charset charset;
    int seed;
} Config;

// Prototypes
void parseInt(const char* arg, void* ptr);
void parseBool(const char* arg, void* ptr);
void parseColor(const char* arg, void* ptr);
void parseString(const char* arg, void* ptr);
void parseCharset(const char* arg, void* ptr);

// Configs
Config config = {
    .delay = DEFAULT_DELAY,
    .mintrail = DEFAULT_MINTRAIL,
    .maxtrail = DEFAULT_MAXTRAIL,
    .sideway = DEFAULT_SIDEWAY,
    .color = DEFAULT_COLOR_ENABLED,
    .matrixColor = {DEFAULT_COLOR_R, DEFAULT_COLOR_G, DEFAULT_COLOR_B},
    .charset = {0},
    .seed = -1
};

const ConfigField config_fields[] = {
    {"-d", "--delay", "Set the delay between frames", parseInt, &config.delay},
    {"-m", "--mintrail", "Set the minimum trail length", parseInt, &config.mintrail},
    {"-M", "--maxtrail", "Set the maximum trail length", parseInt, &config.maxtrail},
    {"-S", "--sideway", "Set the sideways mode", parseBool, &config.sideway},
    {"-C", "--color", "Set the color mode", parseBool, &config.color},
    {"-c", "--textcolor", "Set the text color", parseColor, &config.matrixColor},
    {"-s", "--seed", "Set the random seed", parseInt, &config.seed},
    {"-ch", "--charset", "Set the character set", parseCharset, &config.charset},
    {"-h", "--help", "Show this help message", NULL, NULL},
    {NULL, NULL, 0}
};

// Default Charsets
const StringMap charset_options[] = {
    {"ascii","ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"},
    {"binary","01"},
    {"hex","0123456789ABCDEF"},
    {"number","0123456789"},
    {"punctuation","!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"},
    {"katakana","ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝ"},
    {NULL, NULL}
};

// Functions
static inline int utf8_len(unsigned char c)
{
    if ((c & 0x80) == 0x00) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

Vector2 getConsoleSize() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    return (Vector2){csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1};
}

void randChar(Glyph *glyph) {
    // Returns a random printable character out of charset
    *glyph = config.charset.glyphs[rand() % config.charset.count];
}

Charset buildCharset(const char *str)
{
    Charset cs = {0};

    int len = strlen(str);

    cs.glyphs = malloc(len * sizeof(Glyph));
    cs.count = 0;

    for (int i = 0; i < len;)
    {
        int l = utf8_len((unsigned char)str[i]);

        memcpy(cs.glyphs[cs.count].bytes, &str[i], l);
        cs.glyphs[cs.count].len = l;

        cs.count++;
        i += l;
    }

    return cs;
}

// Parsing functions
void parseInt(const char* arg, void* ptr) {
    *(int*)ptr = atoi(arg);
}
void parseBool(const char* arg, void* ptr) {
    if (strcmp(arg, "true") == 0 || strcmp(arg, "1") == 0 || strcmp(arg, "yes") == 0 || strcmp(arg, "on") == 0) {
        *(bool*)ptr = true;
    } else {
        *(bool*)ptr = false;
    }
}
void parseColor(const char* arg, void* ptr) {
    Color *color_ptr = (Color*)ptr;
    sscanf_s(arg, "(%hhu,%hhu,%hhu)", &color_ptr->r, &color_ptr->g, &color_ptr->b);
}
void parseString(const char* arg, void* ptr) {
    *(char**)ptr = (char*)arg;
}
void parseCharset(const char* arg, void* ptr) {
    Charset *cs_ptr = (Charset*)ptr;
    free(cs_ptr->glyphs);
    for (int i = 0; charset_options[i].set_name != NULL; i++) {
        if (strcmp(arg, charset_options[i].set_name) == 0) {
            *cs_ptr = buildCharset(charset_options[i].set_values);
            return;
        }
    }
    *cs_ptr = buildCharset(arg);
}

void toggleCursor(bool toggle) {
    // Toggles the cursor visibility
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hConsole, &cursorInfo);
    cursorInfo.bVisible = toggle;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    SetConsoleOutputCP(CP_UTF8);
}

void handleSigint(int sig) {
    // Handles SIGINT signal
    toggleCursor(true);
    printf("\033[?1049l");
    running = false;
}

void parseParameters(int argc, char *argv[]) {
    // Parses all parameters given in the command line
    // Example: cmatrix.exe -delay 100 -textcolor (0,255,0) -mintrail 5 -maxtrail 10 -sideway false -color true

    for (int i = 1; i < argc; i++) {
        // Config Fields
        for (int j = 0; config_fields[j].short_opt != NULL; j++) {
            if ((strcmp(argv[i], config_fields[j].short_opt) == 0 || strcmp(argv[i], config_fields[j].long_opt) == 0) && i + 1 < argc) {
                config_fields[j].parser(argv[++i], config_fields[j].ptr);
            }
        }

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: cmatrix.exe [options]\nOptions:\n");

            for (int j = 0; config_fields[j].short_opt != NULL; j++) {
                printf("  %-3s, %-15s %-20s %s\n",
                    config_fields[j].short_opt,
                    config_fields[j].long_opt,
                    config_fields[j].description
                );
            }

            printf(
                "\nMade by AidenDem (https://github.com/AidenDem)\n"
                "Copyright (c) 2025-2026 AidenDem\n"
                "Licensed under the MIT License\n"
            );

            exit(0);
        }
    }
}

void initializeMatrix(Matrix *matrix) {
    srand(config.seed); // Ensures it remains consistent

    matrix->consoleSize = getConsoleSize();
    matrix->bufferSize = matrix->consoleSize.y * matrix->consoleSize.x * (sizeof(Cell) + ANSI_CELL_SIZE) + 2;
    matrix->columns = malloc(matrix->consoleSize.x * sizeof(Column));
    matrix->trailCells = malloc(matrix->consoleSize.x * matrix->consoleSize.y * sizeof(Cell));
    matrix->frameBuffer = malloc(matrix->bufferSize);

    // Malloc allocation validation
    if (!matrix->columns || !matrix->trailCells || !matrix->frameBuffer) exit(1);

    for (int i = 0; i < matrix->consoleSize.x; i++) {
        matrix->columns[i].drop = rand() % matrix->consoleSize.y;
        matrix->columns[i].trail_length = rand() % (config.maxtrail - config.mintrail + 1) + config.mintrail; // Random trail length between mintrail and maxtrail
        for (int j = 0; j < matrix->consoleSize.y; j++) {
           memcpy(matrix->trailCells[i * matrix->consoleSize.y + j].ch.bytes, " ", 1);
           matrix->trailCells[i * matrix->consoleSize.y + j].ch.len = 1;
        }
    }
}

void freeMatrix(Matrix *matrix) {
    free(matrix->columns);
    free(matrix->trailCells);
    free(matrix->frameBuffer);
}

// Main Program
int main(int argc, char *argv[]) {
    // Ensures to set a charset if they didnt pass one in parameters
    config.charset = buildCharset(charset_options[0].set_values);

    parseParameters(argc, argv); // Parse command-line arguments

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Get console handle

    // Switch to alternate buffer
    printf("\033[?1049h");

    // Prepare the console
    toggleCursor(false);

    // Handle SIGINT signal, ensures graceful exit
    signal(SIGINT, handleSigint);

    // Check if a seed was provided, if not set seed
    if (config.seed == -1) {
        config.seed = (int)time(NULL);
    }

    // Initialize Matrix
    Matrix matrix;
    initializeMatrix(&matrix);

    DWORD written;

    // Main Loop
    while (running) {
        // Check for matrix resize
        Vector2 newSize = getConsoleSize();
        if (newSize.x != matrix.consoleSize.x || newSize.y != matrix.consoleSize.y) {
            freeMatrix(&matrix);
            initializeMatrix(&matrix);
        }

        // Last Color
        Color lastColor = {-1, -1, -1};

        // Buffer position
        int pos = 0;
        pos += snprintf(matrix.frameBuffer + pos, matrix.bufferSize - pos, "\033[H"); // Move cursor to top-left

        // Double for loop to iterate through matrix.consoleSize.y and matrix.consoleSize.x
        for (int i = 0; i < matrix.consoleSize.y; i++) {
            // If sideways, slightly offset the trails resulting in a horizontal effect
            for (int j = config.sideway ? 1 : 0; j < matrix.consoleSize.x; j++) {
                // Gets current trail
                int drop = matrix.columns[j].drop;
                int trail_length = matrix.columns[j].trail_length;
                int d = (drop - i + matrix.consoleSize.y) % matrix.consoleSize.y;

                if (d < trail_length) {
                    // Assigns color based on distance from the top character of the trail
                    // The further away, the darker the color
                    double factor = 1.0 - ((double)d / trail_length);
                    Color currentColor = {
                        (uint8_t)(config.matrixColor.r * factor),
                        (uint8_t)(config.matrixColor.g * factor),
                        (uint8_t)(config.matrixColor.b * factor)
                    };

                    // Generate random character if top of trail
                    if (d == 0) {
                        randChar(&CELL(matrix, j, i).ch);
                    }
                    
                    // Update color if it has been changed
                    if (config.color) {
                        if (currentColor.r != lastColor.r || currentColor.g != lastColor.g || currentColor.b != lastColor.b) {
                            lastColor = currentColor;
                            pos += snprintf(matrix.frameBuffer + pos, matrix.bufferSize - pos, "\033[1;38;2;%d;%d;%dm", currentColor.r, currentColor.g, currentColor.b);
                        }
                    }
                    
                    // Write character to buffer
                    memcpy(&matrix.frameBuffer[pos], CELL(matrix, j, i).ch.bytes, CELL(matrix, j, i).ch.len);
                    pos += CELL(matrix, j, i).ch.len;
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
        Sleep(config.delay);
    }

    // Free allocated memory
    freeMatrix(&matrix);
    free(config.charset.glyphs);

    return 0;
}
