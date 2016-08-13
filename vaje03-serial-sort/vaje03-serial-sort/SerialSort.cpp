/**
  * Vaje03-BubbleSort:
  *
  * Program z enostavnim urejanjem z mehurcki (BubbleSort) vecnitno uredi tabelo celih stevil.
  * Na koncu primerja urejenost in cas urejanja z vgrajeno funkcijo qsort() (QuickSort).
  *
  * Porazdeljeni sistemi, 16.11.2015
  *
  * Avtor: Jure Jesensek
  */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

//Stevilo elementov v tabeli
#define N 15000

//Stevilo niti
#define NTHREADS 2

//Zamenjaj elementa
#define SWAP(a,b) do {int temp=a; a = b; b = temp;} while(0)

// Tabela urejenosti za signaliziranje ostalim nitim.
// NTHREADS mutex-ov za singaliziranje "delavnih" niti,
// NTHREADS+1-ti mutex za signaliziranje gl. niti
pthread_cond_t  sortedCond[NTHREADS + 1];			// pogojna spremenjivka

int isSortedArr[NTHREADS];							// tabela "boolean-ov"

pthread_mutex_t sortedAccess = PTHREAD_MUTEX_INITIALIZER;

// Tabela mutex-ov za skupne elemente dveh niti
pthread_mutex_t bufferZone[NTHREADS - 1];

//Struktura za prenasanje argumentov funkciji za urejanje
struct params {
	int rank;	//indeks niti
	int *L;		//kazalec na seznam
};

//Funkcija za primerjavo dveh elementov (test s qsort())
int compare(const void * a, const void * b)
{
	return (*(int*)a - *(int*)b);
}

void * bubbleSort(void *arg) {
	int *L = ((params *)arg)->L;
	int rank = ((params *)arg)->rank;

	//Razdelitev tabele na podsezname
	//Izracunajmo zgornjo in spodnjo mejo podseznama za dano nit (vkljucno)
	int m = (rank*N) / NTHREADS - 1;
	int M = ((rank + 1)*N) / NTHREADS;
	
	if (rank == 0) m = 0;
	if (rank == NTHREADS - 1) M--;

	int leftBuffer = (rank != 0 ? m + 1 : -1);
	int rightBuffer = (rank != NTHREADS - 1 ? M - 2 : M + 2);

	int lockLeft;

	int sorted = 0;

	while (!sorted) {

		//Glavna zanka
		while (!sorted) {

			sorted = 1;
			//Veliko stevilo navzgor
			for (int i = m; i < M; i++) {
				if (i <= leftBuffer || i >= rightBuffer) {
					lockLeft = (i <= leftBuffer ? 1 : 0);
					pthread_mutex_lock(&bufferZone[rank - lockLeft]);
					if (L[i] > L[i + 1]) {
						SWAP(L[i], L[i + 1]);
						sorted = 0;
					}
					pthread_mutex_unlock(&bufferZone[rank - lockLeft]);

					int indexToLock = (rank + 1) - 2 * lockLeft;

					pthread_mutex_lock(&sortedAccess);
					isSortedArr[indexToLock] = 0;
						
					pthread_mutex_unlock(&sortedAccess);

					pthread_cond_signal(&sortedCond[indexToLock]);
					
				} else {
					if (L[i] > L[i + 1]) {
						SWAP(L[i], L[i + 1]);
						sorted = 0;
					}
				}
			}

			sorted = 1;
			//Ucinkoviteje, ce zacnemo kar na isti strani seznama
			//Majhno stevilo navzdol
			for (int i = M; i > m; i--) {
				if (i <= leftBuffer || i > rightBuffer) {
					lockLeft = (i <= leftBuffer ? 1 : 0);
					pthread_mutex_lock(&bufferZone[rank - lockLeft]);
					if (L[i - 1] > L[i]) {
						SWAP(L[i], L[i - 1]);
						sorted = 0;
					}
					pthread_mutex_unlock(&bufferZone[rank - lockLeft]);

					int indexToLock = (rank + 1) - 2 * lockLeft;

					pthread_mutex_lock(&sortedAccess);
					isSortedArr[indexToLock] = 0;

					pthread_mutex_unlock(&sortedAccess);

					pthread_cond_signal(&sortedCond[indexToLock]);
				} else {
					if (L[i - 1] > L[i]) {
						SWAP(L[i], L[i - 1]);
						sorted = 0;
					}
				}
			}

		}
		
		pthread_mutex_lock(&sortedAccess);

		if (rank > 0) {
			pthread_mutex_lock(&bufferZone[rank - 1]);
		}

		if (rank < NTHREADS - 1) {
			pthread_mutex_lock(&bufferZone[rank]);
		}

		if (L[m] <= L[m + 1] && L[M - 1] <= L[M]) {
			isSortedArr[rank] = 1;
		} else {
			isSortedArr[rank] = 0;
		}

		sorted = isSortedArr[rank];

		if (rank < NTHREADS - 1) {
			pthread_mutex_unlock(&bufferZone[rank]);
		}

		if (rank > 0) {
			pthread_mutex_unlock(&bufferZone[rank - 1]);
		}

		if (!sorted) {
			pthread_mutex_unlock(&sortedAccess);
			continue;
		}

		// to sporoci glavni niti
		pthread_cond_signal(&sortedCond[NTHREADS]);

		// gre v spanje
		while (pthread_cond_wait(&sortedCond[rank], &sortedAccess) != 0);

		sorted = isSortedArr[rank];

		pthread_mutex_unlock(&sortedAccess);
	}
	
	return NULL;
}


int main(int argc, char **argv) {

	//Prostor za niti in argumente
	pthread_t t[NTHREADS];
	params p[NTHREADS];

	//Zasedemo prostor za seznam neurejenih stevil
	int *L = (int *)malloc(N*sizeof(int));

	//Napolnimo z nakljucnimi stevili
	srand(time(NULL));

	for (int i = 0; i < N; i++) {
		L[i] = rand();
	}

	//Zasedemo prostor za referencni seznam in prepisemo vanj vrednosti (za preverjanje)
	int *Lref = (int *)malloc(N*sizeof(int));
	memcpy(Lref, L, N*sizeof(int));

	// Inicializiramo (skoraj vse) mutexe in pog. sprem. za tabelo urejenosti 
	// in mutexe za "skupne" elemente
	for (size_t i = 0; i < NTHREADS; i++) {
		pthread_cond_init(&sortedCond[i], NULL);
		//pthread_mutex_init(&sortedLock[i], NULL);
		isSortedArr[i] = 0;

		if (i < NTHREADS - 1) {
			pthread_mutex_init(&bufferZone[i], NULL);
		}
	}
	// Zadnja elementa mutexov in pog. sprem. za sortiranje se inicializirata
	// izven for loopa
	pthread_cond_init(&sortedCond[NTHREADS], NULL);

	//Spremenljivke za merjenje casa izvajanja
	clock_t ts1, ts2, te1, te2;

	//Merimo cas BubbleSort
	ts1 = clock();

	//Ustvarimo niti
	for (int i = 0; i < NTHREADS; i++) {
		p[i].L = L;
		p[i].rank = i;

		if (pthread_create(&t[i], NULL, bubbleSort, (void *)&p[i])) {	// error check
			perror("Napaka pri kreiranju niti. Koncujem izvajanje...\n");
			exit(-1);
		}
	}

	int allSorted = 0;

	

	while (!allSorted) {

		pthread_mutex_lock(&sortedAccess);
		// glavna nit gre v spanje
		while (pthread_cond_wait(&sortedCond[NTHREADS], &sortedAccess) != 0);

		allSorted = 1;
		size_t i;
		for (i = 0; i < NTHREADS; i++) {
			if (!isSortedArr[i]) {
				allSorted = 0;
				break;
			}
		}

		pthread_mutex_unlock(&sortedAccess);

	}

	for (size_t i = 0; i < NTHREADS; i++) {
		pthread_cond_signal(&sortedCond[i]);
	}
	

	//Pocakamo na vse niti
	for (int i = 0; i < NTHREADS; i++) {
		pthread_join(t[i], NULL);
	}

	te1 = clock();

	//Zazenemo qsort nad enakimi stevili
	ts2 = clock();
	qsort(Lref, N, sizeof(int), compare);
	te2 = clock();

	//Preverjanje rezultatov
	int test = 1;
	for (int i = 0; i < N; i++) {
		if (L[i] != Lref[i]) {
			printf("Error, check fail! Index: %d; Value: %d; True value: %d\n", i, L[i], Lref[i]);
			test = 0;
			break;
		}
	}

	//Izpis casov
	if (test) {
		printf("Test passed!\n");
		printf("Time Quicksort: %f s\n", (double)(te2 - ts2) / CLOCKS_PER_SEC);
		printf("Time Bubblesort: %f s\n", (double)(te1 - ts1) / CLOCKS_PER_SEC);
	}



	return 0;
}
