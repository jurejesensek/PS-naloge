/**
 * Vaje10-MPI-urejanje-stevil:
 *
 * MPI program, ki uredi polje stevil od najmanjsega do najvecjega.
 * Postopek:
 *  - Inicializiraj polje N nakljucnih stevil
 *  - Polje razsekaj in ga poslji delavcem (MPI_scatterv)
 *  - Vsak delavec naj svoj del podatkov lokalno uredi
 *  	-Uporabite poljuben algoritem za urejanje stevil (npr. qsort – vgrajena funkcija)
 *  - Vsak proces naj svoj del polja poslje nazaj glavnemu procesu (MPI_Gatherv)
 *  - Glavni proces naj prejeta polja zlije v dokoncno urejeno polje.
 *
 * Prevajaj z: "mpic++ -o mpi-urejanje-stevil mpi-urejanje-stevil.cpp".
 * Pozeni z: "mpirun -np NUM mpi-urejanje-stevil", kjer je -np NUM stevilo procesov.
 * 
 * Porazdeljeni sistemi: 11.1.2015
 *
 * Avtor: Jure Jesensek
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>	// knjiznica MPI
#include <sys/time.h>

#define N_ELEMENTS 1000000

int compfunc(const void *a, const void *b);
double dtime();

int main(int argc, char *argv[]) 
{
	int my_rank;		    // rank (oznaka) procesa
	int num_of_processes;	// stevilo procesov
	int numOfElms;          // stevilo stevil ki jih urejamo
	int *array;             // tabela stevil
	int *result;            // tabela urejenih stevil
	int *test;              // tabela za testiranje urejenosti

	int stopWatch;			// ali se izvaja merjenje casa
	double startMpi, endMpi,startLocal, endLocal;	// casi

	numOfElms = N_ELEMENTS;

	// obdelava prog. argumentov
	for (int i = 0; i < argc; ++i) {
		int noEl = atoi(argv[i]);

		if (!strcasecmp(argv[i], "t") || !strcasecmp(argv[i], "-t") || !strcasecmp(argv[i], "time")) { // ali merimo cas
			stopWatch = 1;
		} else if (noEl != 0) {		// stevilo elementov
			numOfElms = noEl;
		}
	}

	MPI_Init(&argc, &argv);								// inicializacija MPI okolja
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);			// poizvedba po ranku procesa
	MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);	// poizvedba po stevilu procesov

	if (my_rank == 0) {
		// izpis nacina izvajanja
		printf("Program info:\n\tSort %d elements\n\tTimed: %s\n\n", numOfElms, (stopWatch ? "yes" : "no"));

		// rezerviramo prostor za podatke
		array	= (int *) malloc(numOfElms * sizeof(int));
		result	= (int *) malloc(numOfElms * sizeof(int));
		test	= (int *) malloc(numOfElms * sizeof(int));

		// napolnimo tabeli z nakljucnimi stevili
		srand((unsigned int)time(NULL));
		for(int i = 0; i < numOfElms; i++) {
			array[i] = rand() % 256;
			test[i] = array[i];
		}
	}

	int *sendCounts;	// stevilo elementov, ki jih posljemo vsakemu procesu
	int *displs;		// odmiki od zacetka tabele, pri katerem se zacnejo elementi za i-ti proces
	int *localArray;	// za lokalne podatke

	sendCounts	= (int *) malloc(num_of_processes * sizeof(int));
	displs		= (int *) malloc(num_of_processes * sizeof(int));

	for (int i = 0; i < num_of_processes; ++i) {
		// izracun stevila elementov, ki ga bomo poslali vsakemu procesu
		// sendCounts = floor((i + 1) * n / p) - floor(i * n / p)
		sendCounts[i] = ((i + 1) * numOfElms) / num_of_processes - (i * numOfElms) / num_of_processes;
		// izracun odmika
		displs[i] = i ? sendCounts[i - 1] + displs[i - 1] : 0;
	}

	localArray = (int *) malloc(sendCounts[my_rank] * sizeof(int));		// doloci velikost lokalne tabele

	if (my_rank == 0 && stopWatch) startMpi = dtime();	// zacetek merjenja casa za MPI

	// raztrosi
	MPI_Scatterv(array, sendCounts, displs, MPI_INT, localArray, sendCounts[my_rank], MPI_INT, 0, MPI_COMM_WORLD);

	// uredi lokalno tabelo
	qsort(localArray, sendCounts[my_rank], sizeof(int), compfunc);

	// zdruzi
	MPI_Gatherv(localArray, sendCounts[my_rank], MPI_INT, array, sendCounts, displs, MPI_INT, 0, MPI_COMM_WORLD);

	// glavni proces zlije
	if (my_rank == 0) {
		// indeksi na najmanjse elemente posameznih tabel
		int *mergeIdxs = (int *) malloc(num_of_processes * sizeof(int));

		// v mergeIdxs prepisemo odmike
		memcpy(mergeIdxs, displs, num_of_processes * sizeof(int));

		//int nextIdx = 0;	// polozaj kamor bomo zapisali naslednjo urejeno stevilo
		for (int i = 0; i < numOfElms; ++i) {
			int minProc;	// "proces" ki ima najmanjsi trenutni element
			int minIdx;		// indeks najmanjsega elementa ^

			// poisce prvi "proces" ki se ni urejen
			for (int j = 0; j < num_of_processes; ++j) {
				if (mergeIdxs[j] < displs[j] + sendCounts[j]) {
					minIdx = mergeIdxs[j];
					minProc = j;
					break;
				}
			}

			// poisce "proces" z najmanjsim elementom
			for (int j = 0; j < num_of_processes; ++j) {
				if (mergeIdxs[j] < displs[j] + sendCounts[j] && array[mergeIdxs[j]] < array[minIdx]) {
					minIdx = mergeIdxs[j];
					minProc = j;
				}
			}
			mergeIdxs[minProc]++;	// se pomakne za en element naprej

			result[i] = array[minIdx];	// prepise rezultat
		}

		if (stopWatch) {
			endMpi = dtime();	// konec merjenja casa za MPI

			printf("Time for MPI: %lf\n", endMpi - startMpi);

			startLocal = dtime();	// zacetek merjenja casa za lokalno urejanje
		}

		// lokalno urejanje
		qsort(test, numOfElms, sizeof(int), compfunc);

		if (stopWatch) {
			endLocal = dtime();	// konec merjenja casa za lokalno urejanje
			printf("Time for local qsort: %lf\n", endLocal - startLocal);
		}

		// preveri urejenost
		int isSorted = 1;
		for (int i = 0; i < numOfElms; ++i) {
			if (result[i] != test[i]) {
				printf("Error, check fail! Index: %d; Value: %d; True value: %d\n", i, result[i], test[i]);
				isSorted = 0;
			}
		}

		if (isSorted) {
			printf("Test passed, array is sorted...\n");
		}
	}

	MPI_Finalize();

	return 0; 
}

/**
 * Compare function for qsort()
 */
int compfunc(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

/**
 * Funkcija za merjenje casa izvajanja
 */
double dtime()
{
	double tseconds = 0.0;
	struct timeval mytime;

	gettimeofday(&mytime, (struct timezone*) 0);
	tseconds = (double) (mytime.tv_sec + mytime.tv_usec * 1.0e-6);

	return (tseconds);
}
