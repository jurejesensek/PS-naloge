/**
 * Vaje04-openMP-prijateljska-stevila:
 *
 * Vzporeden program, ki s pomocjo OpenMP izracuna vsoto vseh parov 
 * prijateljskih stevil na intervalu [1, INTERVAL_END] (npr. [1, 1000000])
 * 
 * Prevajaj z opcijami "-fopenmp -lm -std=c99" (gcc).
 * 
 * Porazdeljeni sistemi: 23.11.2015
 *
 * Avtor: Jure Jesensek
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define NTHREADS		32
#define INTERVAL_START	1
#define INTERVAL_END	(1000000 + 1)

int main(int argc, char **argv) {

	// tabela za sestevke deliteljev posameznega stevila
	int *allSums = (int*)malloc(INTERVAL_END * sizeof(int));

	// Spremenljivke za merjenje casa izvajanja
	clock_t start, end;

	// nastavi stevilo niti
	omp_set_num_threads(NTHREADS);

	// Zacetek merjenja casa
	start = clock();

#pragma omp parallel for schedule(guided)
	for (int i = INTERVAL_START + 2; i < INTERVAL_END; i++)
	{
		// sestevek deliteljev trenutnega stevila (i)
		int divisorSum = 1;

		// meja do kje racuna delitelje
		int ceiling = (int)sqrt(i);

		// izracuna delitelje
		for (int j = 2; j <= ceiling; j++)
		{
			if (i % j == 0) {
				divisorSum += j;

				int pair = (int)(i / j);
				if (j != pair && pair != i) {
					divisorSum += pair;
				}
			}
		}
		
		allSums[i] = divisorSum;
	}

	// nastavimo zacetni vrednosti izhodov
	allSums[0] = 0;
	allSums[1] = 1;

	// skupna vsota deliteljev
	int sumOfPairs = 0;

	// pogleda ce sta para stevilo-sestevek in sestevek-stevilo enaka, in ju izpise
	for (size_t i = INTERVAL_START + 2; i < INTERVAL_END; i++)
	{
		int sum = allSums[i];
		if (sum < INTERVAL_END && i != sum && i <= sum && allSums[sum] == i) {
			int tmp = i + sum;
			printf("%d + %d = %d\n", i, sum, tmp);
			sumOfPairs += tmp;
		}
	}

	// Konec merjenja casa
	end = clock();

	printf("Overall sum: %d\n", sumOfPairs);

	// Izpis casa izvajanja
	printf("Time: %f\n", (double)(end - start) / CLOCKS_PER_SEC);

	free(allSums);

	return 0;
}