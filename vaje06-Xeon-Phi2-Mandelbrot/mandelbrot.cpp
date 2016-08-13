/**
 * Vaje06-Xeon-Phi2-Mandelbrotova-mnozica
 * 
 * Algoritem za risanje Mandelbrotove mnozice, paraleliziran z OpenMP in prilagojen za izvajanje
 * na Xeon Phi v nacinu z razbremenitvijo (offload).
 * Poleg izrisa mnozice, izracuna tudi ploscino mnozice (metoda stetja pikslov). Uporabljena je 
 * knjiznica za delo z datotekami .pgm.
 *
 * Prevajaj z Intel C compiler:
 * "icc -openmp -O3 -qopt-report=2 -qopt-report-phase=vec mandelbrot.cpp" 
 *
 * Porazdeljeni sistemi: 7.12.2015
 *
 * Avtor: Jure Jesensek
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "pgm.h"

int mandelbrotCPU(PGMData *I);
double dtime();

int main(void) 
{
	double start;
	PGMData slika;

	slika.height = 4000;
	slika.width = 7000;
	slika.max_gray = 255;

	slika.image = (int *)malloc(slika.height * slika.width * sizeof(int));
	
	start = dtime();
	
	int size = mandelbrotCPU(&slika);
	
	printf("Time CPU: %f\n", dtime() - start);
	
	double normSize = 3.5 * 2 * size / (slika.height * slika.width);	// normalizirana ploscina
	
	printf("Size: %d / %d (= %f)\n", size, slika.height * slika.width, normSize);
	
	writePGM("mandelbrot.pgm", &slika);
	
	return 0;
}

int mandelbrotCPU(PGMData *I)
{
	int max_iteration = 1000;   // stevilo iteracij
	int size = 0;				// velikost lika
	
	int height = I->height;
	int width = I->width;
	int max_gray = I->max_gray;
	int *image = I->image;
	
	// za vsak piksel v sliki
	# pragma offload target(mic) in(max_iteration, height, width, max_gray) \
								 out(size) out(image:length(height * width))
	# pragma omp parallel for schedule(guided) reduction(+:size) \
							  firstprivate(max_iteration, height, width, max_gray)
	for(int i = 0; i < height; i++) {
		for(int j = 0; j < width; j++) {
			float x0 = (float)j / width * (float)3.5 - (float)2.5;	// zacetna vrednost
			float y0 = (float)i / height * (float)2.0 - (float)1.0;
			float x = 0;
			float y = 0;
			int iter = 0;
			
			// ponavljamo, dokler ne izpolnemo enega izmed pogojev
			while((x * x + y * y <= 4) && (iter < max_iteration)) {
				float xtemp = x * x - y * y + x0;
				y = 2 * x * y + y0;
				x = xtemp;
				iter++;
			}
			
			// pobarvamo piksel z ustrezno barvo
			int color = (int)(iter / (float)max_iteration * (float)max_gray);
			image[i * width + j] = max_gray - color;
			if (iter > 900)
				size++;
		}
	}
	
	return size;
}

double dtime()
{
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);
	
	return (tseconds);
}
