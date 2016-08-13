/**
 * Vaje09-MPI:
 *
 * Program v katerem racunalniki ciklicno dopolnjujejo sporocilo:
 * 	– Proces 0 poslje svoj rank procesu 1
 * 	– Proces 1 sprejetemu sporocilu doda svoj rank in novo sporocilo poslje procesu 2
 * 	– …
 * 	– Proces n-1 sprejetemu sporocilu doda svoj rank in novo sporocilo poslje procesu 0
 * 	– Proces 0 sprejetemu sporocilu doda svoj rank ter izpise koncno sporocilo
 * 
 * Primer izpisa, ko so vpleteni stirje procesi: 0–1–2–3–0
 *
 * Prevajaj z: "mpic++ -o vaje09-mpi vaje09-mpi.cpp".
 * Pozeni z: "mpirun -np NUM vaje09-mpi", kjer je -np NUM stevilo procesov.
 * 
 * Porazdeljeni sistemi: 6.1.2015
 *
 * Avtor: Jure Jesensek
 */
 
#include <stdio.h>
#include <string.h>
#include <mpi.h>	// knjiznica MPI 

#define MSG_LEN 3

int main(int argc, char *argv[]) 
{
	int my_rank;		// rank (oznaka) procesa 
	int num_of_processes;	// stevilo procesov 
	int source;			// rank posiljatelja 
	int destination;	// rank sprejemnika 
	int tag = 0;		// zaznamek sporocila 
	MPI_Status status;	// status sprejema 

	MPI_Init(&argc, &argv);								// inicializacija MPI okolja 
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);			// poizvedba po ranku procesa 
	MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);	// poizvedba po stevilu procesov 

	int messageLength = MSG_LEN * (num_of_processes + 1);
	char message[messageLength];	// rezerviran prostor za sporocilo
	// velikost sporocila vsakega procesa * (st. procesov + se enkrat 1. proces)
	
	if (my_rank != 0) {		// procesi z rankom != 0 imajo enako funkcijo
		source = my_rank - 1;
		
		// sprejme sporocilo
		MPI_Recv(message, messageLength, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		
		char myMessage[MSG_LEN + 1];
		sprintf(myMessage, "%d-", my_rank);
		strcat(message, myMessage);		// pripise svoj rank
		
		// vsak proces poslje naslednjemu, zadnji proces pa glavnemu
		destination = (my_rank < num_of_processes - 1 ? my_rank + 1 : 0);
		
		// poslje
		MPI_Send(message, (int)strlen(message) + 1, MPI_CHAR, destination, tag, MPI_COMM_WORLD);
		
	} else {	// proces z rankom == 0 se obnasa drugace
	
		sprintf(message, "%d-", my_rank);	// napise svoj rank
		destination = my_rank + 1;
		
		// poslje prvo sporocilo
		MPI_Send(message, (int)strlen(message)+1, MPI_CHAR, destination, tag, MPI_COMM_WORLD);
		
		// nato pocaka
		
		source = num_of_processes - 1;
		MPI_Recv(message, messageLength, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
		
		strcat(message, "0");	// doda 0
		printf("%s\n", message);	// izpise
	}
	
	MPI_Finalize(); 

	return 0; 
} 
