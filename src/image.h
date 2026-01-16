#ifndef IMAGE_H
#define IMAGE_H
#include <stdbool.h>
#include <stdint.h>

#include "vishash.h"

typedef union {
    double array[3];
    struct {
        double r, g, b;
    };
} FloatPixel;

typedef union {
    uint8_t array[3];
    struct {
        uint8_t r, g, b;
    };
} IntPixel;

typedef struct {
    FloatPixel* colors;
    int width, height;
} FloatImage;

typedef struct {
    IntPixel* colors;
    int width, height;
} IntImage;

struct partial_blur_param_s {
    FloatImage* src;
    FloatImage* dst;
    double* kernel;
    int radius;
    int ymin, ymax;
};

#define free_image(img)     free((img).colors)

int max(int, int);
FloatImage intimage_to_floatimage(IntImage img);
IntImage floatimage_to_intimage(FloatImage fimg);

IntImage load_image(const char* filename, int width, int height);
int save_png(const char* filename, IntImage img);
FloatImage gaussian_blur(FloatImage src, int kernel_size, double sigma, int njobs);
void make_iterations(FloatImage* img, int taille, int K, int n, int njobs, bool verbose);

#endif