#ifndef MY_PRINT_IMAGE
#define MY_PRINT_IMAGE
#include "image.h"

typedef struct {
    double hue;
    double saturation;
    double value;
} hsv_t;

void print_image(image_t* image, double edge_threshold, int use_retro_colors);
void print_rainbow_image(image_t* image, double edge_threshold, int use_retro_colors);
void get_ascii_and_color(char* ascii_dest, hsv_t* hsv_dest, image_t* image, double edge_threshold, int use_retro_colors);

#endif
