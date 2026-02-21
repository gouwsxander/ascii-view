#include <stdio.h>
#include <stdlib.h>

#include "../include/image.h"
#include "../include/print_image.h"
#include "../include/argparse.h"

#ifdef _WIN32
#include <windows.h>
#endif

static void enable_virtual_terminal_processing(void) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode))
        return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
#endif
}

int main(int argc, char* argv[]) {
    enable_virtual_terminal_processing();

    // Parses arguments
    args_t args = parse_args(argc, argv);
    if (args.file_path == NULL)
        return 1;

    // Loads image
    image_t original = load_image(args.file_path);
    if (!original.data)
        return 1;

    // Resizes image
    image_t resized = make_resized(&original, args.max_width, args.max_height, args.character_ratio);
    if (!resized.data) {
        free_image(&original);
        return 1;
    }
    
    print_image(&resized, args.edge_threshold, args.use_retro_colors);
    
    free_image(&original);
    free_image(&resized);

    return 0;
}
