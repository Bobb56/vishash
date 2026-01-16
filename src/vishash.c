#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "image.h"
#include "vishash.h"



int parse_args(int argc, char** argv, int* width, int* height, char** filename, char** output, int* njobs, int* K, bool* verbose) {

    for (int i=1 ; i < argc ; i++) {
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--size") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of size.\n");
                return -1;
            }

            *height = *width = atoi(argv[++i]);

            if (*height == 0) {
                printf("Invalid size parameter \"%s\"\n", argv[i+1]);
                return -1;
            }
        }

        else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of width.\n");
                return -1;
            }

            *width = atoi(argv[i+1]);

            if (*width == 0) {
                printf("Invalid width parameter \"%s\"\n", argv[i+1]);
                return -1;
            }
            
            i++;
        }

        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of height.\n");
                return -1;
            }

            *height = atoi(argv[i+1]);

            if (*height == 0) {
                printf("Invalid height parameter \"%s\"\n", argv[i+1]);
                return -1;
            }
            
            i++;
        }

        else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jobs") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of njobs.\n");
                return -1;
            }

            *njobs = atoi(argv[i+1]);

            if (*njobs == 0) {
                printf("Invalid njobs parameter \"%s\"\n", argv[i+1]);
                return -1;
            }
            
            i++;
        }

        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of output.\n");
                return -1;
            }

            *output = strdup(argv[i+1]);
            
            i++;
        }

        else if (strcmp(argv[i], "-K") == 0) {
            if (i + 1 >= argc) {
                printf("Please specify the value of K.\n");
                return -1;
            }

            *K = atoi(argv[i+1]);

            if (*K == 0) {
                printf("Invalid K parameter \"%s\". Should be an integer.\n", argv[i+1]);
                return -1;
            }
            
            i++;
        }

        else if (strcmp(argv[i], "--logs") == 0 || strcmp(argv[i], "-l") == 0) {
            *verbose = true;            
        }

        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            return -1;
        }

        else if (*filename == NULL) {
            *filename = argv[i];
        }
        
        else {
            printf("Unknown argument \"%s\"\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

char* make_output_name(char* filename) {
    const char* added_string = ".hash.png";
    char* output_name = malloc(strlen(filename) + strlen(added_string) + 1);
    strcpy(output_name, filename);
    strcat(output_name, added_string);
    return output_name;
}




int main(int argc, char** argv) {
    int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT, njobs = DEFAULT_NJOBS, K = DEFAULT_K;
    bool verbose = DEFAULT_VERBOSE;
    char* filename = NULL;
    char* output = NULL;

    int error = parse_args(argc, argv, &width, &height, &filename, &output, &njobs, &K, &verbose);

    if (error != 0 || filename == NULL) {
        printf("\nVishash v1.0\n");
        printf("Vishash is a program that computes an easily recognizable png image using only the data of any a given file, such that any change in the file produces a completely different image. The image is only depending from the file and the given parameters.\n");
        printf("Thanks to its properties, Vishash can help to easily check if two files are the same.\n");
        printf("Usage : %s filename\n\n", argv[0]);
        printf("Optional parameters:\n");
        printf(" -s | --size   : With parameter size enabled, the image is a square. size corresponds to the length of one side of the image in pixels. (default: %d)\n", DEFAULT_HEIGHT);
        printf(" -w | --width  : Width of the image in pixels. (default: %d)\n", DEFAULT_WIDTH);
        printf(" -h | --height : Height of the image in pixels. (default: %d)\n", DEFAULT_HEIGHT);
        printf(" -K            : K is a constant representing the level of details of the image. 50 is no details and 300 is too much details. (default: %d)\n", DEFAULT_K);
        printf(" -j | --jobs   : The maximal number of cores to use during calculation (default: %d)\n", DEFAULT_NJOBS);
        printf(" -l | --logs   : Display logs\n");
        printf(" -h | --help   : Displays this help\n");
        return 0;
    }

    if (output == NULL) {
        output = make_output_name(filename);
    }

    debug_log(verbose, "Computing hash of %s...\n", filename);
    
    IntImage img = load_image(filename, width, height);

    if (img.width == 0) {
        printf("Error, exiting Vishash.\n");
        return 0;
    }

    FloatImage fimg = intimage_to_floatimage(img);
    free_image(img);
    
    make_iterations(&fimg, sqrt(width * height), K, max(5, 3000/K), njobs, verbose);

    IntImage img2 = floatimage_to_intimage(fimg);
    free_image(fimg);
    save_png(output, img2);

    debug_log(verbose, "Output image saved in %s.\n", output);

    free(output);

    return 0;
}