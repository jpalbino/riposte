

#define N_DIMS 30 
#define N_ROWS 50000
#define N_ITERATIONS 300

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
//#define VIEW
#ifdef VIEW
#include <vdb.h>
#endif

#include "timing.h"

int main() {
	double * w = new double[N_DIMS];
	bzero(w, sizeof(double) * (N_DIMS));
	double (*data)[N_ROWS] = (double(*)[N_ROWS]) malloc(sizeof(double) * N_DIMS * N_ROWS);
	double * response = new double[N_ROWS];
	
	FILE * file = fopen("../data/lr_p.txt","r");
	assert(file);
	for(int j = 0; j < N_DIMS; j++) {
		for(int i = 0; i < N_ROWS; i++) {
			fscanf(file,"%lf",&data[j][i]);
		}
	}

	file = fopen("../data/lr_r.txt","r");
	assert(file);
	for(int j = 0; j < N_ROWS; j++) {
		fscanf(file, "%lf", &response[j]);
	}
	
	file = fopen("../data/lr_wi.txt","r");
	assert(file);
	for(int j = 0; j < N_DIMS; j++) {
		fscanf(file, "%lf", &w[j]);
	}
	
	/*for(int i = 0; i < N_ROWS; i++) {
		double p = drand();
		response[i] = 3;
		for(int d = 0; d < N_DIMS; d++) {
			data[i][d] = drand();
			response[i] += (d + 1)*data[i][d] + (drand() - .5) * .1; 
		}
		#ifdef VIEW
		vdb_color(1,1,1);
		vdb_point(data[i][0],data[i][1],response[i]);
		#endif
	}*/

	double begin = current_time();
	
	double * grad = new double[N_DIMS];
	for(int round = 0; round < N_ITERATIONS; round++) {
		bzero(grad,sizeof(double) * (N_DIMS));
		
		for(int r = 0; r < N_ROWS; r++) {
			double r_1 = 1.0 / (r + 1);
			double diff = w[0];
			for(int d = 1; d < N_DIMS; d++) {
				diff += w[d] * data[d][r];
			}
			diff = 1.0/(1.0+exp(-diff));
			diff -= response[r];
			grad[0] += (diff - grad[0]) * r_1;
			for(int d = 1; d < N_DIMS; d++) {
				grad[d] += (diff*data[d][r] - grad[d]) * r_1;
			}
		}
		
		for(int d = 0; d < N_DIMS; d++) {
			w[d] -= .07 * grad[d];
		}
	}

	printf("Elapsed: %f\n", current_time()-begin);
	for(int i = 0; i < N_DIMS; i++)	
		printf("w[%d] = %f\n", i, w[i]);

	#ifdef VIEW
	vdb_color(1,0,0);
	vdb_triangle(0,0,w[0],1,0,w[0]+ w[1],0,1,w[0]+w[2]);
	#endif
	
	return 0;
	
}

