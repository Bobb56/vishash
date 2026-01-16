#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <pthread.h>
#include "image.h"
#include "fptc.h"
#include "random.h"
#include "vishash.h"

#define kernel_size(sigma)   fpt2i(fpt_add(fpt_mul(FPT_THREE, sigma), FPT_ONE))


long int get_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    long int size = ftell(file);
    return size;
}

uint8_t* load_file_data(const char* filename, long int* file_size) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        printf("File %s does not exist, or cannot be open.\n", filename);
        return NULL;
    }

    *file_size = get_size(file);
    uint8_t* data = malloc(*file_size);
    rewind(file);
    fread(data, sizeof(uint8_t), *file_size, file);
    fclose(file);
    return data;
}

IntImage create_empty_int_image(int width, int height) {
    IntImage img;
    img.height = height;
    img.width = width;
    img.colors = malloc(sizeof(IntPixel) * width * height);
    return img;
}

FloatImage create_empty_float_image(int width, int height) {
    FloatImage img;
    img.height = height;
    img.width = width;
    img.colors = malloc(sizeof(FloatPixel) * width * height);
    return img;
}

IntImage floatimage_to_intimage(FloatImage fimg) {
    IntImage img = create_empty_int_image(fimg.width, fimg.height);
    for (int i=0 ; i < fimg.width * fimg.height ; i++) {
        img.colors[i].r = (uint8_t)fpt2i(fimg.colors[i].r);
        img.colors[i].g = (uint8_t)fpt2i(fimg.colors[i].g);
        img.colors[i].b = (uint8_t)fpt2i(fimg.colors[i].b);
    }
    return img;
}

FloatImage intimage_to_floatimage(IntImage img) {
    FloatImage fimg = create_empty_float_image(img.width, img.height);
    for (int i=0 ; i < img.width * img.height ; i++) {
        fimg.colors[i].r = i2fpt(img.colors[i].r);
        fimg.colors[i].g = i2fpt(img.colors[i].g);
        fimg.colors[i].b = i2fpt(img.colors[i].b);
    }
    return fimg;
}


IntPixel combine(uint8_t byte, uint32_t random_number) {
    IntPixel pix;
    pix.r = byte ^ random_number;
    pix.g = byte ^ (random_number >> 8);
    pix.b = byte ^ (random_number >> 16);
    return pix;
}


void affect_image(FloatImage* left, FloatImage right) {
    free_image(*left);
    left->colors = right.colors;
}

int max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

int min(int a, int b) {
    if (a < b)
        return a;
    else
        return b;
}


IntImage load_image(const char* filename, int width, int height) {
    IntImage img = create_empty_int_image(width, height);
    int image_size = img.height * img.width;

    long int file_size;
    uint8_t* file_data = load_file_data(filename, &file_size);

    if (file_data == NULL) {
        return (IntImage){0};
    }

    init_random_number(file_data, file_size);

    // on s'assure que l'image sera remplie et qu'on parcourt tout le fichier
    int nb_iterations = max(image_size, file_size);
    
    for (int i = 0 ; i < nb_iterations ; i++) {
        img.colors[i%image_size] = combine(file_data[i%file_size], next_random_number());
    }

    free(file_data);
    return img;
}



int save_png(const char* filename, IntImage img) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return 0;

    png_structp png_ptr =
        png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return 0;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return 0;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return 0;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(
        png_ptr,
        info_ptr,
        img.width,
        img.height,
        8,                      // profondeur
        PNG_COLOR_TYPE_RGB,     // RGB
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png_ptr, info_ptr);

    png_bytep* rows = malloc(sizeof(png_bytep) * img.height);
    for (int y = 0; y < img.height; y++) {
        rows[y] = (png_bytep)(img.colors + y * img.width);
    }

    png_write_image(png_ptr, rows);
    png_write_end(png_ptr, NULL);

    free(rows);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    
    return 1;
}



fpt* gaussian_kernel_1d(int radius, fpt sigma) {
    int size = 2 * radius + 1;
    fpt* k = malloc(sizeof(fpt) * size);
    if (!k) return NULL;

    fpt sum = FPT_ZERO;
    fpt inv_2sigma2 = fpt_div(FPT_ONE, (fpt_mul(FPT_TWO, fpt_mul(sigma, sigma))));

    for (int i = -radius; i <= radius; i++) {
        fpt v = fpt_exp(fpt_mul(fpt_mul(FPT_MINUS_ONE, fpt_mul(i, i)), inv_2sigma2));
        k[i + radius] = v;
        sum = fpt_add(sum, v);
    }

    // normalisation
    for (int i = 0; i < size; i++) {
        k[i] = fpt_div(k[i], sum);
    }

    return k;
}




static inline int clamp(int x, int a, int b) {
    if (x < a) return a;
    if (x > b) return b;
    return x;
}


void* partial_blur(void* args) {
    struct partial_blur_param_s* params = args;
    FloatImage* src = params->src;
    FloatImage* dst = params->dst;
    fpt* kernel = params->kernel;
    int radius = params->radius;
    int ymin = params->ymin;
    int ymax = params->ymax;

    int w = src->width;
    int h = src->height;

    FloatPixel* tmp = malloc(sizeof(FloatPixel) * w * h);

    /* -------- passe verticale -------- */
    for (int y = ymin; y < ymax; y++) {
        for (int x = 0; x < w; x++) {
            fpt r = 0, g = 0, b = 0;

            for (int k = -radius; k <= radius; k++) {
                int yy = clamp(y + k, 0, h-1);
                FloatPixel p = src->colors[yy * w + x];
                fpt wgt = kernel[k + radius];

                r = fpt_add(r, fpt_mul(wgt, p.r));
                g = fpt_add(g, fpt_mul(wgt, p.g));
                b = fpt_add(r, fpt_mul(wgt, p.b));
            }

            FloatPixel* dst_pix = &tmp[y * w + x];
            dst_pix->r = r;
            dst_pix->g = g;
            dst_pix->b = b;
        }
    }

    /* -------- passe horizontale -------- */
    for (int y = ymin; y < ymax ; y++) {
        for (int x = 0; x < w; x++) {
            fpt r = 0, g = 0, b = 0;

            for (int k = -radius; k <= radius; k++) {
                int xx = clamp(x + k, 0, w - 1);
                FloatPixel p = tmp[y * w + xx];
                fpt wgt = kernel[k + radius];

                r = fpt_add(r, fpt_mul(wgt, p.r));
                g = fpt_add(g, fpt_mul(wgt, p.g));
                b = fpt_add(r, fpt_mul(wgt, p.b));
            }

            FloatPixel* dst_pix = &dst->colors[y * w + x];
            dst_pix->r = r;
            dst_pix->g = g;
            dst_pix->b = b;
        }
    }

    free(tmp);

    return NULL;
}



FloatImage gaussian_blur(FloatImage src, int kernel_size, fpt sigma, int njobs) {
    /* Noyau de largeur impaire */
    int radius = kernel_size/2;

    FloatImage dst = create_empty_float_image(src.width, src.height);
    fpt* kernel = gaussian_kernel_1d(radius, sigma);
    
    /* Application du flou */

    int nlines = src.height/njobs; // nombre de lignes à calculer par thread

    struct partial_blur_param_s* args = alloca(sizeof(struct partial_blur_param_s) * njobs);
    pthread_t* threads = alloca(sizeof(pthread_t) * njobs);

    for (int job = 0 ; job < njobs ; job++) {
        args[job] = (struct partial_blur_param_s) {
            .dst = &dst,
            .src = &src,
            .kernel = kernel,
            .radius = radius,
            .ymin = job * nlines,
            .ymax = min((job+1)*nlines, src.height)
        };

        pthread_create(&threads[job], NULL, partial_blur, &args[job]);
    }

    for (int job = 0 ; job < njobs ; job++) {
        pthread_join(threads[job], NULL);
    }

    free(kernel);
    return dst;
}

void extend_image(FloatImage img, fpt factor) {
    for (int x = 0 ; x < img.width ; x++) {
        for (int y = 0 ; y < img.height ; y++) {
            FloatPixel* p = &img.colors[y * img.width + x];
            p->r = i2fpt(fpt2i(fpt_mul(p->r, factor)) % 256);
            p->g = i2fpt(fpt2i(fpt_mul(p->g, factor)) % 256);
            p->b = i2fpt(fpt2i(fpt_mul(p->b, factor)) % 256);
        }
    }
}

void remove_zero(FloatImage img) {
    for (int x = 0 ; x < img.width ; x++) {
        for (int y = 0 ; y < img.height ; y++) {
            for (int color = 0 ; color < 3 ; color++) {
                fpt* color_ptr = &img.colors[y * img.width + x].array[color];
                *color_ptr = (*color_ptr == FPT_ZERO) ? FPT_ONE : *color_ptr;
            }
        }
    }
}

void make_iterations(FloatImage* img, int taille, int K, int n, int njobs, bool verbose) {
    debug("Lancement de %d iterations avec K=%d et taille=%d\n", n, K, taille);
    int size;
    FloatImage tmp;
    for (int i = 1 ; i < n ; i++) {
        debug_log(verbose, "Iteration %d/%d\n", i, n-1);

        fpt sigma = fpt_div(i2fpt(i*taille), i2fpt(K));
        size = kernel_size(sigma);
        debug("Starting gaussian blur, kernel_size=%d, sigma=%lf\n", size, sigma);
        tmp = gaussian_blur(*img, size, sigma, njobs);

        affect_image(img, tmp);
        debug("Extension de l'image\n");
        extend_image(*img, fpt_add(FPT_TWO, fpt_mul(FPT_FIVE, fpt_div(i2fpt(n-i), i2fpt(n)))));
        //remove_zero(*img); // pas forcément nécessaire
    }
}