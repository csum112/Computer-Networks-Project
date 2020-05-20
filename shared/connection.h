#ifndef CONNECTION_H
#define CONNECTION_H
#endif

#define CHUNK_SIZE 1024

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <iostream>

using namespace std;

class Connection
{
	int sd;
public:
	Connection(int port, string ip);
	Connection(int fd);
	~Connection() {close(sd);}
	void send(string message);
	string receive();
};

Connection::Connection(int port, string ip)
{
	struct sockaddr_in server;
	bzero (&server, sizeof (server));
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror ("Eroare la socket().\n");
		exit(errno);
	}

	/* umplem structura folosita pentru realizarea conexiunii cu serverul */
	/* familia socket-ului */
	server.sin_family = AF_INET;
	/* adresa IP a serverului */
	server.sin_addr.s_addr = inet_addr(ip.c_str());
	/* portul de conectare */
	server.sin_port = htons (port);

	/* ne conectam la server */
	if (connect (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
	{
		perror ("[client]Eroare la connect().\n");
		exit(errno);
	}
}

Connection::Connection(int fd)
{
	this->sd = fd;
}

void Connection::send(string message)
{
	{
		long long size = message.size();
		int bytes_read = write(sd, &size, sizeof(long long));
		if (bytes_read < 0)
			perror("[Connection] Error whilst sending data: ");
	}
	int CHUNKS = 0;
	while (CHUNKS < message.size())
	{
		int to_send = (CHUNK_SIZE < message.size() - CHUNKS) ? CHUNK_SIZE : message.size() - CHUNKS;
		int bytes_read = write(sd, &(*message.begin()) + CHUNKS, to_send);
		if (bytes_read < 0)
			perror("[Connection] Error whilst sending data: ");
		CHUNKS += bytes_read;
	}
}

string Connection::receive()
{
	string result = "";
	int bytes_read;
	char buffer[CHUNK_SIZE];
	long long expected_size = 0;
	bytes_read = read(sd, &expected_size, sizeof(long long));
	if (bytes_read < 0)
		perror("[Connection] Error whilst receiving data: ");

	while (result.size() < expected_size)
	{
		int to_receive = (CHUNK_SIZE < expected_size - result.size()) ? CHUNK_SIZE : expected_size - result.size();
		bytes_read = read(sd, buffer, to_receive);
		if (bytes_read < 0)
			perror("[Connection] Error whilst receiving data: ");
		string temp;
		temp.assign(buffer, buffer + to_receive);
		result += temp;
	}


	if (expected_size < result.size())
	{
		cout << "[Connection] Received more data than expected\n";
		return "";
	} else if (expected_size > result.size())
	{
		cout << "[Connection] Received less data than expected:\n";
		return "";
	}
	return result;
}



