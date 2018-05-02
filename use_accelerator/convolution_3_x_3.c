#include "vgg16.h"

#define USE_FPGA

float fpga_out[SIZE + 2][SIZE + 2] = {0.0};
float (*fpga_matrix)[SIZE+2];
float (*fpga_kernel)[CONV_SIZE];
int fpga_size;
unsigned char fpga_busy = 0;

pthread_t tid;

extern int numthreads;

void convolution_in_fpga(float matrix[SIZE+2][SIZE+2], float kernel[CONV_SIZE][CONV_SIZE], float out[SIZE+2][SIZE+2], int size) {
	
#ifdef USE_FPGA //TODO: Not tested
	fpga_set_matrix_kernel(matrix, kernel, size);
	fpga_start();
	fpga_poll();
#else
	int i, j;
	float sum;
	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			sum = matrix[i][j] * kernel[0][0] +
				matrix[i + 1][j] * kernel[1][0] +
				matrix[i + 2][j] * kernel[2][0] +
				matrix[i][j + 1] * kernel[0][1] +
				matrix[i + 1][j + 1] * kernel[1][1] +
				matrix[i + 2][j + 1] * kernel[2][1] +
				matrix[i][j + 2] * kernel[0][2] +
				matrix[i + 1][j + 2] * kernel[1][2] +
				matrix[i + 2][j + 2] * kernel[2][2];
			out[i+1][j+1] += sum;
		}
	}
#endif
}

void convolution_3_x_3(float matrix[SIZE+2][SIZE+2], float kernel[CONV_SIZE][CONV_SIZE], float out[SIZE+2][SIZE+2], int size) {
	int i, j;
	float sum;

	if(!fpga_busy) {
		fpga_busy = 1; 
		convolution_in_fpga(matrix, kernel, out, size);
	}
	else {
		for (i = 0; i < size; i++) {
			for (j = 0; j < size; j++) {
				sum = matrix[i][j] * kernel[0][0] +
					matrix[i + 1][j] * kernel[1][0] +
					matrix[i + 2][j] * kernel[2][0] +
					matrix[i][j + 1] * kernel[0][1] +
					matrix[i + 1][j + 1] * kernel[1][1] +
					matrix[i + 2][j + 1] * kernel[2][1] +
					matrix[i][j + 2] * kernel[0][2] +
					matrix[i + 1][j + 2] * kernel[1][2] +
					matrix[i + 2][j + 2] * kernel[2][2];
				out[i+1][j+1] += sum;
			}
		}

	}
}
// This method is called in parallel by multiple threads
void convolution_2d(int shape_depth, float input[][SIZE + 2][SIZE + 2], float weights_kernel[][CONV_SIZE][CONV_SIZE],
					float output[SIZE + 2][SIZE + 2], int size)
{
	int j=0;
	if (fpga_busy) {
		for (j = 0; j < shape_depth; j++) {
			convolution_3_x_3(input[j], weights_kernel[j], output, size);
		}
	}
	else {
		fpga_busy = 1;
#ifdef USE_FPGA
		fpga_set_out_size(output, size);
#endif
		for (j = 0; j < shape_depth; j++) {
			convolution_in_fpga(input[j], weights_kernel[j], output, size);
		}
#ifdef USE_FPGA
		fpga_read_out_r(output, size);
#endif
		fpga_busy = 0;
	}
}
