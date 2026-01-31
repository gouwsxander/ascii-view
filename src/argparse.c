#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/ioctl.h>
#endif

#include <unistd.h>
#include "../include/argparse.h"


#define DEFAULT_MAX_WIDTH 64
#define DEFAULT_MAX_HEIGHT 48
#define DEFAULT_CHARACTER_RATIO 2.0
#define DEFAULT_EDGE_THRESHOLD 4.0


void print_help(char* exec_alias) {
    printf("USAGE:\n");
    printf("\t%s <path/to/image> [OPTIONS]\n\n", exec_alias);

    printf("ARGUMENTS:\n");
    printf("\t<path/to/image>\t\tPath to image file\n\n");

    printf("OPTIONS:\n");
    printf("\t-mw <width>\t\tMaximum width in characters (default: terminal width OR %d)\n", DEFAULT_MAX_WIDTH);
    printf("\t-mh <height>\t\tMaximum height in characters (default: terminal height OR %d)\n", DEFAULT_MAX_HEIGHT);
    printf("\t-et <threshold>\t\tEdge detection threshold, range: 0.0 - 4.0 (default: %.1f, disabled)\n", DEFAULT_EDGE_THRESHOLD);
    printf("\t-cr <ratio>\t\tHeight-to-width ratio for characters (default: %.1f)\n", DEFAULT_CHARACTER_RATIO);
    printf("\t--retro-colors\t\tUse 3-bit retro color palette (8 colors) instead of 24-bit truecolor\n");
    printf("\t-r\t\t\tAnimate ascii art with rainbow by shifting colors (q or Q to quit)\n");
}

// Get size of terminal in characters. Returns 1 if successful.
int try_get_terminal_size(size_t* width, size_t* height) {
#ifdef _WIN32
// Windows implementation
    if (!_isatty(0))
        return 0;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return 0;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
        return 0;

    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
// POSIX implementation
    if (!isatty(0))
        return 0;
    struct winsize ws;

    if (ioctl(0, TIOCGWINSZ, &ws) == 0) {
        *width = (size_t) ws.ws_col;
        *height = (size_t) ws.ws_row;
        return 1;
    }
#endif
    return 0;
}


args_t parse_args(int argc, char* argv[]) {
    // Get variable defaults
    args_t args = {
        .file_path = NULL,
        .max_width = DEFAULT_MAX_WIDTH,
        .max_height = DEFAULT_MAX_HEIGHT,
        .character_ratio = DEFAULT_CHARACTER_RATIO,
        .edge_threshold = DEFAULT_EDGE_THRESHOLD,
        .use_retro_colors = 0,
        .use_rainbow_colors = 0
    };

    try_get_terminal_size(&args.max_width, &args.max_height);

    // If no file given
    if (argc == 1) {
        print_help(argv[0]);
        return args;
    }

    // Get file path
    if (!strcmp(argv[1], "-h")) {
        print_help(argv[0]);
        return args;
    } else {
        args.file_path = argv[1];
    }

    // Get optional parameters
    for (size_t i = 2; i < (size_t) argc; i++) {
        if (!strcmp(argv[i], "-mw") && i + 1 < (size_t) argc)
            args.max_width = (size_t) atoi(argv[++i]);
        else if (!strcmp(argv[i], "-mh") && i + 1 < (size_t) argc)
            args.max_height = (size_t) atoi(argv[++i]);
        else if (!strcmp(argv[i], "-et") && i + 1 < (size_t) argc)
            args.edge_threshold = atof(argv[++i]);
        else if (!strcmp(argv[i], "-cr") && i + 1 < (size_t) argc)
            args.character_ratio = atof(argv[++i]);
        else if (!strcmp(argv[i], "--retro-colors"))
            args.use_retro_colors = 1;
        else if (!strcmp(argv[i], "--rainbow"))
            args.use_rainbow_colors = 1;
        else
            fprintf(stderr, "Warning: Ignoring invalid or incomplete argument '%s'\n", argv[i]);
    }

    return args;
}
