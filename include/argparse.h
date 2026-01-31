#ifndef MY_ARGPARSE
#define MY_ARGPARSE
#include <stdlib.h>


typedef struct {
    char* file_path;
    size_t max_width;
    size_t max_height;
    double character_ratio;
    double edge_threshold;
    int use_retro_colors;
    int use_rainbow_colors;
} args_t;

args_t parse_args(int argc, char* argv[]);
#endif
