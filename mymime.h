#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#define MAXRCVLEN 1000
#define PORTNUM 110


int init(const char* serwer)
{

	char buffer[MAXRCVLEN + 1];

	int  mysocket;
	struct sockaddr_in dest;
	int len;
	struct hostent* hostinfo;


	hostinfo = gethostbyname(serwer);

	if (hostinfo == 0)
		return -1;

	mysocket = socket(AF_INET, SOCK_STREAM, 0);

	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_addr = *(struct in_addr*)hostinfo->h_addr;
	dest.sin_port = htons(PORTNUM);

	if (connect(mysocket, (struct sockaddr*)&dest, sizeof(struct sockaddr)) < 0)
		return -1;
	return mysocket;

}
bool logowanie(int socket, const char* login, const char* haslo)
{
	char buffer[MAXRCVLEN + 1];
	bzero(buffer, MAXRCVLEN + 1);
	int len;
	char komenda[] = "USER ";
	char pass[] = "PASS ";
	strcat(komenda, login);
	strcat(komenda, "\r\n");
	send(socket, komenda, strlen(komenda), 0);
	//printf("%s\n",komenda);
	len = recv(socket, buffer, MAXRCVLEN, 0);
	strcat(buffer, "\r\n");
	buffer[len] = '\0';
	//printf("login %s\n",buffer);
	if (strncmp(buffer, "+OK", 3) != 0)
		return false;
	bzero(buffer, MAXRCVLEN + 1);
	strcpy(komenda, pass);
	strcat(komenda, haslo);
	strcat(komenda, "\r\n");
	send(socket, komenda, strlen(komenda), 0);
	// printf("%s\n",komenda);
	len = recv(socket, buffer, MAXRCVLEN, 0);
	strcat(buffer, "\r\n");
	//  buffer[len]='\0';
	 //printf("haslo %s\n",buffer);
	if (strncmp(buffer, "+OK", 3) != 0)
		return false;

	return true;
}
void wyloguj(int socket)
{
	char komenda[] = "QUIT\r\n";
	int len;
	char buffer[MAXRCVLEN + 1];
	send(socket, komenda, strlen(komenda), 0);
	len = recv(socket, buffer, MAXRCVLEN, 0);

	if (strncmp(buffer, "+OK", 3) != 0)
		printf("Wystąpiły problemy z poprawnym wylogowaniem.\n");//else
	 //   printf("Wylogowano poprawnie.\n");  
}

void finito(int socket)
{
	close(socket);
}
bool FindFilename(const char buffer[], int len, char filename[])
{
	bool f = false; int idx = 0; int i = 9;
	int find = 0; char string[] = "filename="; char* wsk;
	bzero(filename, 150);
	if ((wsk = strstr(buffer, string)) != NULL)
	{
		f = true;
		wsk += strlen(string);
		while ((*(wsk + find) != '\r') && (*(wsk + find) != '\n'))
		{
			if ((*(wsk + find) != '"') && (*(wsk + find) != ' '))
				filename[idx++] = *(wsk + find);
			find++;
		}
	}
	// filename[idx]='\0';
	return f;
}

bool FindBoundary(const char buffer[], int len, char b[])
{
	bool f = false; int idx = 0; int i = 0; char btmp[98];
	int find = 0; char string[] = "boundary="; char* wsk;
	//bzero(filename,100);
	if ((wsk = strstr(buffer, string)) != NULL)
	{
		f = true;
		wsk += strlen(string);
		while ((*(wsk + find) != '\r') && (*(wsk + find) != '\n'))
		{
			if (*(wsk + find) != '"')
				btmp[idx++] = *(wsk + find);
			find++;
		}
		//   printf("jest plik %s",buffer);
		btmp[idx] = '\0'; //printf("Banduary = %s\n",btmp);
		bzero(b, 100);
		strcat(b, "--");
		strcat(b, btmp); //printf("Banduary = %s\n",b);
	}

	return f;
}
bool EndAttach(const char buffer[], int len, const char b[])
{
	//  printf("End attach buffer = %s\n",buffer);
	//  printf("End attach b      = %s\n",b);
	if (strncmp(buffer, b, len) == 0)
		return true;
	return false;
}

bool IsContent(const char buffer[])
{
	bool f = false;
	if ((strncmp(buffer, "Content-", 8) == 0) || (buffer[0] == '=') || (buffer[0] == '<') || (buffer[0] == ' ') && (buffer[0] == '\r') && (buffer[0] == '\n'))
		return true; else
		return false;
}
bool EndMsg(const char buffer[], int len, const char b[])
{
	bool f = false;
	char tmp[102]; bzero(tmp, 102);
	strcat(tmp, b);
	strcat(tmp, "--"); //printf("End = %s\n",tmp);
	//printf("END MSG buffer = %s\n",buffer);
	//printf("END MSG tmp    = %s\n",tmp);
	if (strstr(buffer, b) == 0)
		return true;
	return false;
}
int scanline(FILE* f, char buf[])
{

	int i = 0; char c;
	bzero(buf, 100);
	while ((c != EOF) && (c != '\n'))
	{
		c = fgetc(f);
		//  if((c!=EOF)&&(c!='\r')&&(c!='\n'))
		buf[i++] = c;
	}
	if (i > 0)  buf[i] = '\0'; printf("%s\n", buf);
	return i;
}
int UnpackAttachements(FILE* f)
{
	char buf[200]; int len = 0;
	char filename[150]; FILE* b64file = NULL;
	int acount = 0;
	char b64[] = "base64coded";
	bool band = false;
	char bound[100]; int o = 0;
	fseek(f, 0, SEEK_SET); bool fname = false;
	bzero(buf, 200);
	bzero(bound, 100); //%[^\n]\r\n
	while ((len = fscanf(f, "%[^\n]\n", buf)) > 0) //czyta całą wiadomość email
	{
		if (!band) //czy ogranicznik Boundary został ustawiony dla tego pliku?
			if ((band = FindBoundary(buf, len, bound)) == true)
			{
				//  printf("BOUNDUARY [%s]\n",bound);
				continue;
			}
		if (band == true)
			if (fname == false) //czy filename zostało odnalezione w wiadomości?
				if ((fname = FindFilename(buf, len, filename)) == true)
				{
					for (o = 0; o < strlen(filename); o++)
						if (filename[o] == '\32')
							filename[o] = '_';
					b64file = fopen(b64, "w+"); acount++;
					//printf("NAZWA PLIKU [%s]\n",filename);
					continue;
				}
				else continue;
		if (band == true)
			if (fname == true)
				if (EndAttach(buf, len, bound) == true)
				{
					// printf("%s\n","koniec zalacznika");
					fclose(b64file);
					fname = false;
					char komenda[] = "./b64decode ";
					strcat(komenda, b64);
					strcat(komenda, " ");
					strcat(komenda, filename);
					system(komenda);
					unlink(b64);
					if ((EndMsg(buf, len, bound)) == true) break;
					continue;
				}
				else
					if (IsContent(buf) == false)
					{
						//buf[len]='\n';
						if (b64file != NULL)
							fprintf(b64file, "%s", buf);
						//  printf("%s",buf);
					}
		bzero(buf, 200);
	}
	return acount;
}
int Czytaj(char* buff, int len) //zapisanie uidl do pliku numerow wiadomosci
{
	int ret = 0; int i;
	for (i = 0; i < len; i++)
		if (buff[i] == '\r') ret++;
	return ret;
}


int DownloadFiles(int socket, int num)
{
	FILE* f1; int l; char str[10];
	char buffer[MAXRCVLEN + 1];
	int lattachments = 0;
	bool filename = false;
	char filename_char[100];
	for (l = 1; l < num; l++) //ściaga i zapisuje wszystkie emaile
	{

		sprintf(str, "%d", l);
		printf("Sciagam email numer %s\n", str);
		f1 = fopen(str, "w+");
		char komenda[] = "RETR ";
		strcat(komenda, str);
		strcat(komenda, "\r\n");
		int len, i;
		send(socket, komenda, strlen(komenda), 0);
		do {
			len = 0;
			len = recv(socket, buffer, MAXRCVLEN, 0);
			buffer[len] = '\0';
			fwrite(buffer, len, 1, f1);
			if (buffer[len - 3] == '.')break;
		} while (len != 0);
		//printf("Skanuje email numer %s w poszukiwaniu zalacznikow\n",str);
		lattachments += UnpackAttachements(f1);
		fclose(f1);
		unlink(str);
	}
	return lattachments;
}
int GetFiles(int socket) //zwraca true gdy jest nowa wiadomość i false gdy nie ma
{
	char komenda[] = "UIDL\r\n";
	int len, i;
	int ret = 0;
	char buffer[MAXRCVLEN + 1];
	int num = 0;
	send(socket, komenda, strlen(komenda), 0);
	do {
		len = 0;
		len = recv(socket, buffer, MAXRCVLEN, 0);
		buffer[len] = '\0';
		num = num + Czytaj(buffer, len);
		if (buffer[len - 3] == '.')break;
	} while (len != 0);
	if ((num - 2) > 0)
		ret = DownloadFiles(socket, num - 2);
	return ret;

}
void SendNOP(int socket)
{
	char komenda[] = "NOP\r\n";
	int len;
	char buffer[MAXRCVLEN + 1];
	send(socket, komenda, strlen(komenda), 0);
	len = recv(socket, buffer, MAXRCVLEN, 0);
	buffer[len] = '\0';
	/*  if(strncmp(buffer,"+OK",3)!=0)
	   printf("Wystąpiły problemy z komendą NOP.\n");else
		printf("Poprawnie wysłano komendę NOP.\n");
   */
   // if(strncmp(buffer,"+OK",3)!=0)
}