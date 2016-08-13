/**
 * Vaje02-pThreads:
 *
 * Enostaven streznik, ki zmore sprejeti enega ali vec klientov naenkrat.
 * Streznik sprejme klientove podatke in jih v nespremenjeni obliki poslje
 * nazaj klientu - odmev.
 * 
 * Povezava na streznik poteka preko protokola telnet s katerim se povezemo
 * na localhost na vrata PORT (trenutno 1053).
 *
 * V projekt je potrebno dodati knjiznico pthread (pthreadVC2.lib).
 *
 * Porazdeljeni sistemi, 02.11.2015
 *
 * Avtor: Jure Jesensek
 */

//Vkljucimo ustrezna zaglavja
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <winsock2.h>
#include <pthread.h>

void *serve(void *client);

/*
Definiramo vrata (port) na katerem bo streznik poslusal
in velikost medpomilnika za sprejemanje in posiljanje podatkov
*/
#define PORT 1053
#define BUFFER_SIZE 256
#define MAX_CONNECTIONS 3

// struktura, ki se posilja niti kot argument
struct clientStruct {
	SOCKET clientSock;
	bool busy;
};

int main(int argc, char **argv)
{

	//Spremenljivka za preverjane izhodnega statusa funkcij
	int iResult;

	//Vzpostavimo knjiznico, ki omogoci uporabo vticev (sockets)
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

	/*
	Ustvarimo nov vtic, ki bo poslusal
	in sprejemal nove kliente preko TCP/IP protokola
	*/
	SOCKET listener = INVALID_SOCKET;
	listener=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listener == SOCKET_ERROR) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	//Nastavimo vrata in mrezni naslov vtica
	SOCKADDR_IN listenerConf;
	listenerConf.sin_port=htons(PORT);
	listenerConf.sin_family=AF_INET;
	listenerConf.sin_addr.s_addr=INADDR_ANY;

	//Vtic povezemo z ustreznimi vrati
	iResult = bind( listener, (SOCKADDR *)&listenerConf, sizeof(listenerConf));
	if (iResult == SOCKET_ERROR) {
		printf("Bind failed with error: %d\n", WSAGetLastError());
		closesocket(listener);
		WSACleanup();
		return 1;
    }

	//Zacnemo poslusati
	if ( listen( listener, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Listen failed with error: %ld\n", WSAGetLastError() );
		closesocket(listener);
		WSACleanup();
		return 1;
	}

	//Definiramo nov vtic in medpomnilik
	pthread_t t[MAX_CONNECTIONS];	// nitke
	struct clientStruct clients[MAX_CONNECTIONS];	// argumenti za nitke

	bool acceptNewConnetions = TRUE;
	int nextSocket = 0;		// polozaj v tabeli, v kateri bo "deloval" naslednji socket

	for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
		clients[i].busy = FALSE;
	}
	
	/*
	V zanki sprejemamo nove povezave in klicemo "serve" za strezbo (lahko vec kot eno)
	*/
	while (1) {
		//Sprejmi povezavo in ustvari nov vtic
		clients[nextSocket].clientSock = accept(listener,NULL,NULL);
		if (clients[nextSocket].clientSock == INVALID_SOCKET) {
			printf("Accept failed: %d\n", WSAGetLastError());
			closesocket(listener);
			WSACleanup();
			return 1;
		}

		if (!acceptNewConnetions) {		// dosezeno max stevilo niti oz. klientov
			printf("Exceeded number of allowed connections: %d\n", MAX_CONNECTIONS);
		} else {
			pthread_create(&t[nextSocket], NULL, serve, (void *)&clients[nextSocket]);	// ustvari novo nit
			clients[nextSocket].busy = TRUE;		// v strukturi oznazi da je "delovna"
			acceptNewConnetions = FALSE;		// predpostavi da so zasedene vse niti
		}

		for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
			if (!clients[i].busy) {		// preveri ali so res vse niti zasedene
				nextSocket = i;
				acceptNewConnetions = TRUE;
				break;
			}
		}
		
	}

	for (size_t i = 0; i < MAX_CONNECTIONS; i++) {
		if (clients[i].busy)
			pthread_join(t[i], NULL);
	}

	//Pocistimo vse vtice in sprostimo knjiznico
	closesocket(listener);
    WSACleanup();

	return 0;
}

void *serve(void *client)
{
	struct clientStruct *cli = (struct clientStruct*) client;
	SOCKET clientSock = cli->clientSock;
	char buffer[BUFFER_SIZE];
	int iResult;

	//Postrezi povezanemu klientu
	do {

		//Sprejmi podatke
		iResult = recv(clientSock, buffer, BUFFER_SIZE, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);

			//Vrni prejete podatke posiljatelju
			iResult = send(clientSock, buffer, iResult, 0);
			if (iResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(clientSock);
				break;
			}
			printf("Bytes sent: %d\n", iResult);
		} else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(clientSock);
			break;
		}

	} while (iResult > 0);

	closesocket(clientSock);
	cli->busy = FALSE;

	return NULL;
}
