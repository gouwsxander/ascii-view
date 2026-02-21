#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "../include/image.h"

// Characters to print
#define VALUE_CHARS " .-=+*x#$&X@"
#define N_VALUES (sizeof(VALUE_CHARS) - 1) // Exclude null

// Color ANSI codes
#define RESET "\x1b[0m"


typedef struct {
    double hue;
    double saturation;
    double value;
} hsv_t;


double* get_max(double* a, double* b, double* c) {
    if ((*a >= *b) && (*a >= *c)) {
        return a;
    } else if (*b >= *c) {
        return b;
    } else {
        return c;
    }
}

double* get_min(double* a, double* b, double* c) {
    if ((*a <= *b) && (*a <= *c)) {
        return a;
    } else if (*b <= *c) {
        return b;
    } else {
        return c;
    }
}


hsv_t rgb_to_hsv(double red, double green, double blue) {
    hsv_t hsv;
    
    double* max = get_max(&red, &green, &blue);
    double* min = get_min(&red, &green, &blue);
    
    hsv.value = *max;
    double chroma = hsv.value - *min;
    
    // Calculate saturation
    if (fabs(hsv.value) < 1e-4) {
        hsv.saturation = 0.0;
    } else {
        hsv.saturation = chroma / hsv.value;
    }
    
    // Calculate hue
    if (chroma < 1e-4) {
        hsv.hue = 0.0;
    } else if (max == &red) {
        hsv.hue = 60.0 * fmod((green - blue) / chroma, 6.0);
        if (hsv.hue < 0.0) hsv.hue += 360.0;
    } else if (max == &green) {
        hsv.hue = 60.0 * (2.0 + (blue - red) / chroma);
    } else {
        hsv.hue = 60.0 * (4.0 + (red - green) / chroma);
    }
    
    return hsv;
}


void hsv_to_rgb(const hsv_t* hsv, double* r, double* g, double* b) {
    double c = hsv->value * hsv->saturation;
    double h_prime = hsv->hue / 60.0;
    double x = c * (1.0 - fabs(fmod(h_prime, 2.0) - 1.0));
    
    double r1, g1, b1;
    
    if (h_prime >= 0.0 && h_prime < 1.0) {
        r1 = c; g1 = x; b1 = 0.0;
    } else if (h_prime >= 1.0 && h_prime < 2.0) {
        r1 = x; g1 = c; b1 = 0.0;
    } else if (h_prime >= 2.0 && h_prime < 3.0) {
        r1 = 0.0; g1 = c; b1 = x;
    } else if (h_prime >= 3.0 && h_prime < 4.0) {
        r1 = 0.0; g1 = x; b1 = c;
    } else if (h_prime >= 4.0 && h_prime < 5.0) {
        r1 = x; g1 = 0.0; b1 = c;
    } else {
        r1 = c; g1 = 0.0; b1 = x;
    }
    
    double m = hsv->value - c;
    *r = r1 + m;
    *g = g1 + m;
    *b = b1 + m;
}


void get_retro_rgb(const hsv_t* hsv, int* out_r, int* out_g, int* out_b) {
    // For retro colors: quantize hue and saturation for 8-color palette
    hsv_t quantized_hsv = *hsv;
    
    // Set value to full brightness (character controls apparent brightness)
    quantized_hsv.value = 1.0;
    
    // Quantize hue to nearest multiple of 60 degrees (6 hues: R, Y, G, C, B, M)
    quantized_hsv.hue = round(quantized_hsv.hue / 60.0) * 60.0;
    if (quantized_hsv.hue >= 360.0) {
        quantized_hsv.hue = 0.0;
    }
    
    // Quantize saturation: either 0% (grayscale) or 100% (full color)
    // Using 0.25 threshold as before
    quantized_hsv.saturation = (quantized_hsv.saturation < 0.25) ? 0.0 : 1.0;
    
    // Convert back to RGB
    double r, g, b;
    hsv_to_rgb(&quantized_hsv, &r, &g, &b);
    
    // Convert to 0-255 range
    *out_r = (int)(r * 255);
    *out_g = (int)(g * 255);
    *out_b = (int)(b * 255);
}


double calculate_grayscale_from_hsv(const hsv_t* hsv) {
    // Use value * value for increased contrast
    return hsv->value * hsv->value;
}


char get_ascii_char(double grayscale) {
    size_t index = (size_t) (grayscale * N_VALUES);

    // Clamp
    if (index >= N_VALUES) {
        index = N_VALUES - 1;
    }

    return VALUE_CHARS[index];
}


char get_sobel_angle_char(double sobel_angle) {
    if ((22.5 <= sobel_angle && sobel_angle <= 67.5) || (-157.5 <= sobel_angle && sobel_angle <= -112.5))
        return '\\';
    else if ((67.5 <= sobel_angle && sobel_angle <= 112.5) || (-112.5 <= sobel_angle && sobel_angle <= -67.5))
        return '_';
    else if ((112.5 <= sobel_angle && sobel_angle <= 157.5) || (-67.5 <= sobel_angle && sobel_angle <= -22.5))
        return '/';
    else
        return '|';
}


void print_image(image_t* image, double edge_threshold, int use_retro_colors) {
    image_t grayscale = make_grayscale(image);
    double* sobel_x = calloc(grayscale.width * grayscale.height, sizeof(*sobel_x));
    double* sobel_y = calloc(grayscale.width * grayscale.height, sizeof(*sobel_y));
    if (!sobel_x || !sobel_y)
        fprintf(stderr, "Error: Failed to allocate memory for edge detection!\n");

    if (edge_threshold < 4.0)
        get_sobel(&grayscale, sobel_x, sobel_y);

    for (size_t y = 0; y < image->height; y++) {
        for (size_t x = 0; x < image->width; x++) {
            double* pixel = get_pixel(image, x, y);

            size_t index = y * image->width + x;
            double sx = sobel_x[index];
            double sy = sobel_y[index];

            double square_sobel_magnitude = sx * sx + sy * sy;
            double sobel_angle = atan2(sy, sx) * 180. / M_PI;

            char ascii_char;
            
            double grayscale;
            int r = 255, g = 255, b = 255; // Default white for grayscale
            
            if (image->channels <= 2) {
                // Grayscale image
                grayscale = pixel[0];
                r = g = b = (int)(pixel[0] * 255);
            } else {
                // RGB image
                hsv_t hsv = rgb_to_hsv(pixel[0], pixel[1], pixel[2]);
                
                grayscale = calculate_grayscale_from_hsv(&hsv);
                
                // Set value to full brightness for both modes
                // Character choice controls apparent brightness, not color value
                hsv.value = 1.0;
                
                if (use_retro_colors) {
                    // Retro mode: quantize hue to 60° and saturation to 0% or 100%
                    get_retro_rgb(&hsv, &r, &g, &b);
                } else {
                    // Truecolor mode: convert HSV back to RGB with full brightness
                    double r_d, g_d, b_d;
                    hsv_to_rgb(&hsv, &r_d, &g_d, &b_d);
                    r = (int)(r_d * 255);
                    g = (int)(g_d * 255);
                    b = (int)(b_d * 255);
                }
            }

            ascii_char = get_ascii_char(grayscale);

            // If edge
            if (square_sobel_magnitude >= edge_threshold * edge_threshold)
                ascii_char = get_sobel_angle_char(sobel_angle);
            
            // Use 24-bit truecolor ANSI escape code
            printf("\x1b[38;2;%d;%d;%dm%c", r, g, b, ascii_char);
        }
        printf("\n");
    }

    printf("%s", RESET);
    
    free(sobel_x);
    free(sobel_y);
    free_image(&grayscale);
}
