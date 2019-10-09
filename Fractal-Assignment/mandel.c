
#include "bitmap.h"

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

struct thread_payload {
	struct bitmap * bm;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int chunk_bottom;
	int chunk_top;
	int max;
	int width;
	int height;
};

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
void compute_image(struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max, int threads);
void * compute_chunk(void * args);

void show_help() {
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=500)\n");
	printf("-H <pixels> Height of the image in pixels. (default=500)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-t <threads>   Set number of threads. (default=1)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] ) {
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int    threads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:t:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 't':
				threads = atoi(optarg);
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d threads=%d outfile=%s\n",xcenter,ycenter,scale,max,threads, outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Compute the Mandelbrot image
	compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max, threads);


	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

/**
 * Meant to be called by compute_image(), computes only
 * a small partition of the overall image
 */
void * compute_chunk(void * args) {
	struct thread_payload * p = (struct thread_payload *)args;
	int j,i;

	for(j = p->chunk_bottom; j < p->chunk_top; j++) {

		for(i=0; i  <p->width; i++) {

			// Determine the point in x,y space for that pixel.
			double x = p->xmin + i*(p->xmax-p->xmin)/p->width;
			double y = p->ymin + j*(p->ymax-p->ymin)/p->height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,p->max);

			// Set the pixel in the bitmap.
			bitmap_set(p->bm,i,j,iters);
		}
	}
	return NULL;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image( struct bitmap *bm, double xmin, double xmax, double ymin, double ymax, int max , int threads) {
	int chunk_size, i;
	int width = bitmap_width(bm);
	int height = bitmap_height(bm);

	/* Create an array of thread args, populate them on each iteration.*/
	pthread_t tid[threads];
	struct thread_payload args[threads];
	chunk_size = height / threads;
	for (i = 0; i < threads; i++) {
		args[i].bm = bm;
		args[i].xmin = xmin;
		args[i].xmax = xmax;
		args[i].ymin = ymin;
		args[i].ymax = ymax;
		args[i].max = max;
		args[i].chunk_top = ((i+1) * chunk_size);
		args[i].chunk_bottom = i * chunk_size;
		args[i].width = width;
		args[i].height = height;

		/* spawn thread */
		pthread_create(&tid[i], NULL, compute_chunk, &args[i]);
	}

	/* close threads */
	for (i = 0; i < threads; i++) {
		pthread_join(tid[i], NULL);
	}
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max ) {
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max ) {
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}




