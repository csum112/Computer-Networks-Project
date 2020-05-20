#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <fcntl.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#define CONFIG_FILE_PATH "/Users/catalin121/Desktop/.mySvnClient"
#define CONF_BUFFER_SIZE 1024
using namespace std;


class Persistent_data
{
	int port;
	string ip;
	string username;
public:
	Persistent_data();
	void set_ip(string ip);
	void set_port(string port);
	void set_username(string username);
	int get_port();
	string get_ip();
	string get_username();
};



Persistent_data::Persistent_data()
{
	mkdir(CONFIG_FILE_PATH, 777);
	char buffer[CONF_BUFFER_SIZE];
	int fd, no_bytes;
	fd = open(((string)CONFIG_FILE_PATH + "/port").c_str(), O_RDONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	no_bytes = read(fd, &port, sizeof(int));
	close(fd);

	fd = open(((string)CONFIG_FILE_PATH + "/ip").c_str(), O_RDONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	no_bytes = read(fd, buffer, CONF_BUFFER_SIZE);
	if (no_bytes > 0)
		ip.assign(buffer, buffer + no_bytes);
	close(fd);

	fd = open(((string)CONFIG_FILE_PATH + "/username").c_str(), O_RDONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	no_bytes = read(fd, buffer, CONF_BUFFER_SIZE);
	if (no_bytes > 0)
		username.assign(buffer, buffer + no_bytes);
	close(fd);
}


void Persistent_data::set_port(string port)
{
	this->port = stoi(port);
	remove(((string)CONFIG_FILE_PATH + "/port").c_str());
	int fd = open(((string)CONFIG_FILE_PATH + "/port").c_str(), O_WRONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	int bytes = write(fd, &(this->port), sizeof(int));
	if (bytes <= 0) perror("[Config] Error writing to file");
	close(fd);
}

void Persistent_data::set_ip(string ip)
{
	this->ip = ip;
	remove(((string)CONFIG_FILE_PATH + "/ip").c_str());
	int fd = open(((string)CONFIG_FILE_PATH + "/ip").c_str(), O_WRONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	int bytes = write(fd, &(*ip.begin()), ip.size());
	if (bytes <= 0) perror("[Config] Error writing to file");
	close(fd);
}

void Persistent_data::set_username(string username)
{
	this->username = username;
	remove(((string)CONFIG_FILE_PATH + "/username").c_str());
	int fd = open(((string)CONFIG_FILE_PATH + "/username").c_str(), O_WRONLY | O_CREAT);
	if (fd < 0)
		perror("[Config] Error opening file: ");
	int bytes = write(fd, &(*username.begin()), username.size());
	if (bytes <= 0) perror("[Config] Error writing to file");
	close(fd);
}

int Persistent_data::get_port()
{
	return this->port;
}

string Persistent_data::get_ip()
{
	return this->ip;
}

string Persistent_data::get_username()
{
	return this->username;
}


#endif

