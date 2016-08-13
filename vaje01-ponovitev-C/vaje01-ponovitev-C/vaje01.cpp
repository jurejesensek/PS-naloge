/**
 * Vaje01-ponovitev-C:
 *
 * Funkcije random, matrix in max uporabimo v glavnem programu,
 * tako da polje, ki ga ustvari funkcija random kot argument podamo
 * preostalima dvema funkcijama in izpisemo rezultate.
 *
 * Izmerimo cas v sekundah, ki ga potrebuje funkcija random,
 * da ustvari in napolni polje stevil in ga izpisemo.
 *
 * Porazdeljeni sistemi, 26.10.2015
 *
 * Avtor: Jure Jesensek
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <errno.h>

double *random(int n);
double **matrix(double *A, int n, int r);
double *max(double *A, int n);

void print1D(double mat[], int length);
void print2D(double *matrix[], int rows, int columns);

extern int errno;

int main(int argc, char **argv)
{
	int n, r, err;
	printf("Vnesi n: ");	// dolzina 1D polja
	err = scanf_s("%d", &n);

	// error check
	switch (err) {
	case 'EOF':
		perror("Napacen n");
		exit(1);
	case 0:
		printf("Prazen n. Koncujem program.\n");
		exit(1);
	}	// end of error check

	printf("Vnesi r: ");	// st. vrstic 2D polja
	err = scanf_s("%d", &r);

	// error check
	switch (err) {
	case 'EOF':
		perror("Napacen r");
		exit(1);
	case 0:
		printf("Prazen r. Koncujem program.\n");
		exit(1);
	}	// end of error check

	clock_t start, stop;	// merjenje casa

	start = clock();
	double *randomArr = random(n);
	stop = clock();

	// ne izpisujemo polj vecjih od 100 elementov
	if (n < 100) {
		print1D(randomArr, n);
	}

	double **random2dArr = matrix(randomArr, n, r);

	// ne izpisujemo polj vecjih od 100 elementov
	if (n < 100) {
		print2D(random2dArr, r, n);
	}

	double *pMax = max(randomArr, n);
	printf("Najvecja vrednost: %f na naslovu %p.\n", *pMax, pMax);
	
	double genTime = (double)(stop - start) / CLOCKS_PER_SEC;	// cas potreben za generiranje stevil
	printf("Potreben cas za generiranje nakljucnih stevil: %f\n", genTime);


	// Free all of the arrays!
	free(randomArr);

	for (size_t i = 0; i < r; i++) {
		free(random2dArr[i]);
	}

	free(random2dArr);

	return 0;
}

/* 
 * Funkcija random dinamicno ustvari in vrne kazalec na polje n-nakljucnih
 * realnih stevil med 0 in 1.
 */
double *random(int n)
{
	double *arr = (double*)malloc(n * sizeof(double));

	if (arr == NULL) {
		perror("Malloc v random");
		exit(2);
	}

	for (size_t i = 0; i < n; i++) {
		arr[i] = (double) rand() / (double) RAND_MAX;
	}

	return arr;
}

/*
 * Funkcija matrix dinamicno ustvari matriko z r-vrsticami in vanjo prepise 
 * vrednosti iz polja A. Nato vrne kazalec na matriko.
 */
double **matrix(double *A, int n, int r)
{
	int len = (int)ceil((double)n / (double)r);	// dolzina vrstice
	double **mat = (double**)malloc(r * sizeof(double*));

	if (mat == NULL) {
		perror("Malloc v matrix");
		exit(2);
	}

	for (size_t i = 0; i < r; i++) {
		mat[i] = (double*)calloc((size_t)len, sizeof(double));

		if (mat[i] == NULL) {
			perror("Calloc v matrix");
			exit(2);
		}
	}

	int curPos = 0;		// polozaj v 1D polju
	for (size_t i = 0; i < r; i++) {
		for (size_t j = 0; j < len; j++) {
			curPos = (int)(i * len + j);
			if (curPos >= n) break;

			mat[i][j] = A[curPos];
		}
		if (curPos >= n) break;
	}

	return mat;
}

/*
 * Funkcija vrne kazalec na najvecjo vrednost v polju A
 */
double *max(double *A, int n)
{
	double *pMax = A;
	for (size_t i = 1; i < n; i++)
	{
		if (A[i] > *pMax) {
			pMax = &A[i];
		}
	}
	return pMax;
}

/*
 * Izpise 1-dimenzionalno polje realnih stevil. 
 */
void print1D(double matrix[], int length)
{
	printf("1D:\n");
	for (size_t i = 0; i < length; i++) {
		printf("%f ", matrix[i]);
	}
	printf("\n");
}

/*
* Izpise 2-dimenzionalno polje realnih stevil.
*/
void print2D(double *matrix[], int rows, int numOfElms)
{
	int columns = (int)ceil((double)numOfElms / (double)rows);
	printf("2D:\n");
	for (size_t i = 0; i < (size_t)rows; i++) {
		for (size_t j = 0; j < (size_t)columns; j++) {
			printf("%f ", matrix[i][j]);
		}
		printf("\n");
	}
}
