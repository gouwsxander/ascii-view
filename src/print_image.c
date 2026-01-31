#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define s_sleep(x) Sleep(x * 1000)
#else
    #include <unistd.h>
    #include <termios.h>
    #define s_sleep(x) usleep(x * 1000)
#endif

#include "../include/image.h"
#include "../include/print_image.h"

// Characters to print
#define VALUE_CHARS " .-=+*x#$&X@"
#define N_VALUES (sizeof(VALUE_CHARS) - 1) // Exclude null
#define SLEEP_TIME 0.1

// Color ANSI codes
#define RESET "\x1b[0m"

#ifndef _WIN32
    //these functions are used to allow for non blocking scan (used for quiting rainbow mode)
    static struct termios original_settings;
    void set_raw_mode(void) {
        struct termios new_settings;
        //backup current terminal settings
        tcgetattr(STDIN_FILENO, &original_settings);
        new_settings = original_settings;

        //disable ICANON (ui now processes by character not by line)
        //disable ECHO (user input not echoed to terminal)
        new_settings.c_lflag &= ~(ICANON | ECHO);
        //minimum num characters needed before read() returns is set to 0
        new_settings.c_cc[VMIN] = 0;
        //maximum num characters needed before read() returns is set to 0
        new_settings.c_cc[VTIME] = 0;

        tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
    }

    //restore terminal to original settings
    void restore_mode(void) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_settings);
    }

#endif


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

void print_rainbow_image(image_t* image, double edge_threshold, int use_retro_colors) {
    char true = 1;
    char* ascii = (char*)malloc(sizeof(char) * image->height * image->width);
    hsv_t* hsvs = (hsv_t*)malloc(sizeof(hsv_t) * image->height * image->width);

    if (!ascii || !hsvs)
        fprintf(stderr, "Error: Failed to allocate memory for edge detection!\n");

    //get the regular ascii and hsv values
    get_ascii_and_color(ascii, hsvs, image, edge_threshold, use_retro_colors);

    #ifndef _WIN32
        set_raw_mode();
    #endif

    char key_press = 0;
    //loop until killed by terminal
    while(true) {
        //now print the image with correct colors
        for (size_t y = 0; y < image->height; y++) {
            for(size_t x = 0; x < image->width; x++) {
                //get the ascii character and hsv value
                hsv_t hsv = hsvs[y * image->width + x];

                double r_d, g_d, b_d;
                hsv_to_rgb(&hsv, &r_d, &g_d, &b_d);

                //get the rgb values and ascii character
                int r = (int)(r_d * 255);
                int g = (int)(g_d * 255);
                int b = (int)(b_d * 255);
                char ascii_char = ascii[y * image->width + x];

                //print the character
                printf("\x1b[38;2;%d;%d;%dm%c", r, g, b, ascii_char);

                //now peform a hue rotation on the hsv value and store it for next time
                if(use_retro_colors) {
                    hsv.hue += 20.0;
                    if (hsv.hue >= 60.0)
                        hsv.hue -= 60.0;

                    hsvs[y * image->width + x] = hsv;

                } else {
                    hsv.hue += 2.0;
                    if (hsv.hue >= 360.0)
                        hsv.hue -= 360.0;

                    hsvs[y * image->width + x] = hsv;
                }

            }
            printf("\n");
        }

        printf("\x1b[0mPress q to quit\n");

        printf("\x1b[%luA", image->height+2);
        if(use_retro_colors) {
            s_sleep(1000);
        } else {
            s_sleep(50);
        }

        if (read(STDIN_FILENO, &key_press, 1) > 0) {
            if (key_press == 'q' || key_press == 'Q') {
                break; // Exit the loop
            }
        }
    }

    #ifdef _WIN32
        if (_kbhit()) {
            char KEY_PRESS = _getch();

            if (KEY_PRESS == 'q' || KEY_PRESS == 'Q') {
                break;
            }
        }
    #else
        restore_mode();
    #endif
    free(ascii);
    free(hsvs);
    //clear the terminal
    printf("\x1b[2J");
}

void get_ascii_and_color(char* ascii_dest, hsv_t* hsv_dest, image_t* image, double edge_threshold, int use_retro_colors) {
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

            if (image->channels <= 2) {
                // Grayscale image
                grayscale = pixel[0];
            } else {
                // RGB image
                hsv_t hsv = rgb_to_hsv(pixel[0], pixel[1], pixel[2]);

                grayscale = calculate_grayscale_from_hsv(&hsv);

                if (use_retro_colors) {
                    // Retro mode: quantize hue to 60° and saturation to 0% or 100%
                    int r, g, b = 255;
                    get_retro_rgb(&hsv, &r, &g, &b);
                    hsv = rgb_to_hsv(r, g, b);
                }

                hsv.value = 1.0;
                hsv_dest[index] = hsv;
            }

            ascii_char = get_ascii_char(grayscale);

            // If edge
            if (square_sobel_magnitude >= edge_threshold * edge_threshold)
                ascii_char = get_sobel_angle_char(sobel_angle);

            ascii_dest[index] = ascii_char;
        }
    }

    free(sobel_x);
    free(sobel_y);
    free_image(&grayscale);
}
