#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>


#include "serverlogic.h"
#include "../shared/client-database/database.h"

#define DB_PATH "mySvn.database"

#define PORT 2908

class Server
{
	struct sockaddr_in server;
	struct sockaddr_in from;
	int sd;
	Database* db;
public:
	Server(int port);
	~Server();
	void serve(void(*stuffToDo)(int fd, Database* db));
};

void concurency_test(int fd, Database* db)
{
	int nr;		//mesajul primit de trimis la client
	if (read (fd, &nr, sizeof(int)) <= 0)
		perror ("Eroare la read() de la client.\n");
	/*pregatim mesajul de raspuns */
	nr++;
	/* returnam mesajul clientului */
	write (fd, &nr, sizeof(int));
	close(fd);
}

int main()
{
	Server foo(PORT);
	foo.serve(logic);
	return 0;
}


void Server::serve(void(*stuffToDo)(int fd, Database* db))
{
	db->createTables();
	while (1)
	{
		int client;
		unsigned int length = sizeof (from);
		printf ("[server]Asteptam..\n");
		fflush (stdout);

		if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
		{
			perror ("[server]Eroare la accept().\n");
			continue;
		}

		std::thread worker(stuffToDo, client, db);
		worker.detach();
	}
}

Server::Server(int port)
{
	//Initializare database
	this->db = new Database(DB_PATH);
	/* crearea unui socket */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("[server]Eroare la socket().\n");
		exit(errno);
	}
	/* utilizarea optiunii SO_REUSEADDR */
	int on = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	/* pregatirea structurilor de date */
	bzero (&server, sizeof (server));
	bzero (&from, sizeof (from));

	/* umplem structura folosita de server */
	/* stabilirea familiei de socket-uri */
	server.sin_family = AF_INET;
	/* acceptam orice adresa */
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	/* utilizam un port utilizator */
	server.sin_port = htons (PORT);

	/* atasam socketul */
	if (::bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
	{
		perror ("[server]Eroare la bind().\n");
		exit(errno);
	}

	/* punem serverul sa asculte daca vin clienti sa se conecteze */
	if (listen (sd, 2) == -1)
	{
		perror ("[server]Eroare la listen().\n");
		exit(errno);
	}
}

Server::~Server()
{
	delete this->db;
	close(sd);
}