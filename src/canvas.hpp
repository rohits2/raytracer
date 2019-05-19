#ifndef CANVAS_H
#define CANVAS_H

#include "primitive.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct Canvas {
  public:
    int width, height;
    Vec3 *buffer;

    Canvas(int rows, int cols) : width(cols), height(rows) {
        buffer = new Vec3[width * height];
        memset(buffer, 0, width * height * sizeof(Vec3));
    }
    ~Canvas() { delete[] buffer; }
    Vec3 *operator[](int row) { return &buffer[row * width]; }
    void write_ppm(char *ppm_file) {
        // Set up header
        char *header;
        asprintf(&header, "P6 %d %d 255\n", width, height);

        // Compute file size
        long long header_sz = strlen(header);
        long long length = header_sz + width * height * 3 ;

        // Open fd
        int fd = open(ppm_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            fprintf(stderr, "Could not open file %s!\n", ppm_file);
            exit(-1);
        }

        // create buffer
        unsigned char *file = new unsigned char[length];

        // write headers        
        memcpy(file, header, header_sz);

        // write imdata
        unsigned char *bblob = file + header_sz;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                const Vec3 &rgb = (*this)[i][j];
                unsigned char r, g, b;
                r = rgb.x * 255;
                g = rgb.y * 255;
                b = rgb.z * 255;
                int ind = (i * width + j) * 3;
                bblob[ind] = r;
                bblob[ind + 1] = g;
                bblob[ind + 2] = b;
            }
        }

        // close file
        write(fd, file, length); 
        close(fd);

        // free buffers
        free(header);
        delete[] file;
    }
};

#endif