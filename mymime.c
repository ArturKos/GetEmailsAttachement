#include "mymime.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


int nowe = 0;
int main(int argc, char* argv[])
{
	int gniazdo;
	if (argc != 4)
	{
		printf("%s \n", "Program wymaga podania 3 argumentow.");
		printf("%s \n", "mymime adres_serwera nazwa_konta hasło");
		return 1;
	}
	printf("Program pobiera wszystkie maile ze skrzynki email oraz wypakowuje zalaczniki.\r\n");

	gniazdo = init(argv[1]); // próba połączenia z wybranym serwerem

	if (gniazdo < 0)
	{ //nie udało się połączyć z serwerem
		printf("%s %s\n", "Nie mogę się połączyć z wybranym serwerem:", argv[1]);
		return 1;
	}
	printf("%s %s\r\n", "Logowanie do serwera:", argv[1]);
	printf("%s", "Sprawdzam poczte...\t\n");
	if (logowanie(gniazdo, argv[2], argv[3])) //próba logowania do serwera
	{ //udane logowanie
	 // printw ("Pomyślnie zalogowano.\n Sprawdzam wiadomości...\n");
 //  printw("Minelo : %d sekund\n",czas);
		if ((nowe = GetFiles(gniazdo)) > 0) { //zwraca true gdy jest nowa wiadomość i false w przeciwnym wypadku
			printf("Otrzymano %d zalacznikow!\r\n", nowe);
		}
		else { printf("Brak zalacznikow.\r\n"); }

	}
	else
	{ // nie udało się zalogować
		printf("%s\n", "Logowanie zakończyło się niepowodzeniem. Sprawdź login i hasło.\n");
	}
	wyloguj(gniazdo);// wylogowanie z serwera
	finito(gniazdo); // zamknięcie gniazda



	return 0;
}