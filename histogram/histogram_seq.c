// HPC course University of Macedonia
// Compile: gcc -O3 histogram_seq.c -o histogram_seq.out -fopenmp
// Execute: ./histogram_seq.out
/* image magic cli : mogrify -format bmp original.jpg */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

int main()
{
   	double start, end;
    FILE *fIn = fopen("original.bmp", "rb");
    FILE *fOut = fopen("new.bmp", "wb");
    if (!fIn || !fOut)
    if (!fIn)
    {
        printf("File error.\n");
        return 0;
    }

    unsigned char header[54];
    fread(header, sizeof(unsigned char), 54, fIn);
    fwrite(header, sizeof(unsigned char), 54, fOut);

    int width = *(int*)&header[18];
    int height = abs(*(int*)&header[22]);
    int stride = (width * 3 + 3) & ~3;
    int padding = stride - width * 3;

    printf("width: %d (%d)\n", width, width * 3);
    printf("height: %d\n", height);
    printf("stride: %d\n", stride);
    printf("padding: %d\n", padding);

    int histogram[256][4];
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 4; j++)
            histogram[i][j] = 0;

    unsigned char pixel[3];

    start = omp_get_wtime();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            fread(pixel, 3, 1, fIn);
            histogram[pixel[0]][0]++;
            histogram[pixel[1]][1]++;
            histogram[pixel[2]][2]++;
            unsigned char gray = pixel[0] * 0.299 + pixel[1] * 0.587 + pixel[2] * 0.114;
            histogram[gray][3]++;
            memset(pixel, gray, sizeof(pixel));
            fwrite(&pixel, 3, 1, fOut);
        }
        fread(pixel, padding, 1, fIn);
        fwrite(pixel, padding, 1, fOut);
    }

    end = omp_get_wtime();

    fclose(fOut);
    fclose(fIn);

    // for (int j = 0; j < 4; j++) {
    // 	printf("\n\n");
    // 	for (int i = 0; i < 256; i++)
    //         printf("%d \t", histogram[i][j]);
    //     printf("\n");
    // }

    printf("Calculated in %f seconds\n", end - start);

    return 0;
}
