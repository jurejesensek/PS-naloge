/**
  * Vaje05-Xeon-Phi-Gaussova-eliminacija:
  * 
  * Algoritem za Gaussovo eliminacijo z delnim pivotiranjem, paraleliziran
  * na Xeon Phi koprocesorju z uporabo OpenMP knjižnice in načinom z
  * z razbremenitvijo (offload).
  *
  * Prevajaj z Intel C compiler:
  * "icc -openmp -O3 -qopt-report=2 -qopt-report-phase=vec -o gauss gauss.cpp" 
  *
  * Porazdeljeni sistemi: 30.11.2015
  * 
  * Avtor: Jure Jesensek
  */

#include <stdio.h>
#include <sys/time.h>
#include <omp.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ABS(x) (x < 0.0 ? -(x) : (x))
#define SWAP(a,b) do {double temp = a; a = b; b = temp;} while(0)

double dtime();
int gepp(double *A, double *B, int N);
double backsubs(double *A, double *B, double *x, int N);

// Stevilo enacb/neznank
int N = 2048;

/**
 * Program za 1. argument vzame stevilo ponovitev izvajanja, za 2. za velikost
 * matrike. V kolikor se program zazene brez argumentov se privzameta
 * vrednosti 1 in 2048.
 */
int main(int argc, char **argv){
	
	// Rezervirajmo prostor za matrike
	double *A = (double *) _mm_malloc(N * N * sizeof(double), 64);	// poravnava
	double *B = (double *) _mm_malloc(N * sizeof(double), 64);		// poravnava
	double *x = (double *) malloc(N * sizeof(double));
	
	int numIter = 0;
	if (argc > 1) {
		numIter = (int)strtol(argv[1], NULL, 10);	// st. iteracij
		
		if (argc == 3) N = (int)strtol(argv[2], NULL, 10);	// velikost matrike
	}
	
	if (!numIter) numIter = 1;
	
	double avgTime = 0.0;
	double avgRmse = 0.0;
	
	for(int it = 0; it < numIter; it++) {
		// Nakljucno napolnimo sistem enacb
		for(int i = 0; i < N; i++) {
			for(int j = 0; j < N; j++) {
				A[i * N + j] = (double) rand() / RAND_MAX;
			}
			B[i] = (double) rand() / RAND_MAX;
			x[i] = 0.0;
		}
		
		// Gaussova eliminacija
		double tstart = dtime();
		gepp(A, B, N);
		double tstop = dtime();
		
		// Dokoncno resimo enacbo z vzvratno substitucijo
		double rmse = backsubs(A, B, x, N);
		
		avgTime += tstop - tstart;
		avgRmse += rmse;
		
		// Izpis
		printf("System of %d equations solved in %.2f s. RMSE = %.2f.\n", N, tstop-tstart, rmse);
	}
	
	if(numIter) {
		avgTime /= numIter;
		avgRmse /= numIter;
		
		// Izpis povprecij
		printf("\nAverage:\n");
		printf("System of %d equations solved %d times in %.2f s. RMSE = %.2f.\n", N, numIter, avgTime, avgRmse);
	}	
	
	// sprostimo pomnilnik
	_mm_free(A);
	_mm_free(B);
	free(x);
	
	return 0;
}

// Funkcija za merjenje casa izvajanja
double dtime() {
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);
	
	return (tseconds);
}

// Gaussova eliminacija z delnim pivotiranjem
// Pretvori matriko A v zgornje trikotno matriko
int gepp(double *A, double *B, int N) {
	
	// offload na Xeon Phi
	# pragma offload target(mic) inout(A:length(N * N)) inout(B:length(N))
	for(int i = 0; i < N - 1; i++) {
		
		// Najdimo vrstico z najvecjim elementom v trenutnem stolpcu
		int maxi = i;
		int max = A[i];
		# pragma vector
		for(int j = i + 1; j < N; j++) {
			if (ABS(A[j * N + i]) > max) {
				maxi = j;
				max  = ABS(A[j * N + i]);
			}
		}
		
		// Zamenjajmo vrstici
		# pragma ivdep
		for(int j = i; j < N; j++) {
			SWAP(A[i * N + j], A[maxi * N + j]);
		}
		
		SWAP(B[maxi], B[i]);
		
		// Eliminacija
		# pragma omp parallel for
		for(int j = i + 1; j < N; j++) {
			double tmp=A[j * N + i] / A[i * N + i];
			
			# pragma ivdep
			for(int k = i; k < N; k++) {
				A[j * N + k] -= (tmp * A[i * N + k]);
			}
			
			B[j] -= (tmp * B[i]);
		}
	}
	
	return 1;
}

// Vzvratna substitucija
// Resi sistem enacb oblike Ax=B
// A mora biti zgornje trikotna matrika
double backsubs(double *A, double *B, double *x, int N) {
	
	x[N - 1] = B[N - 1] / A[(N - 1) * N + N - 1];
	for(int i = N - 2; i >= 0; i--) {
		double tmp = B[i];
		for(int j = i + 1; j < N; j++) {
			tmp -= A[i * N + j] * x[j];	
		}
		
		x[i] = tmp / A[i * N + i];
	}
	
	// Preverimo resitev
	// Izracun povprecne kvadratne napake
	double err = 0.0;
	for(int i = 0; i < N; i++) {
		double bt = 0.0;
		for(int j = 0; j < N; j++) {
			bt += A[i * N + j] * x[j];
		}
		err += ((bt - B[i]) * (bt - B[i]));
	}
	return sqrt(err / N);
}
