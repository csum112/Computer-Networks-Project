#include <iostream>
#include <string>
#include <vector>
#include "config.h"
#include "commands.h"



using namespace std;

typedef vector<string> strings;

int main(int argc, char** argv)
{
	Persistent_data config;
	//Converting to vector of strings
	strings args;
	for (int i = 1; i < argc; i++)
		args.push_back((string)argv[i]);
	if (argc == 1)
	{
		cout << "More arguments required...\n";
		return 0;
	}

	//Set config files
	if (args[0] == "set")
		if (argc != 4)
		{
			cout << "Bad args...\n";
			return 0;
		}
		else if (args[1] == "port")
		{
			config.set_port(args[2]);
		}
		else if (args[1] == "ip")
		{
			config.set_ip(args[2]);
		}
		else if (args[1] == "username")
		{
			config.set_username(args[2]);
		}
		else cout << "Unknown property\n";
	else 
		if(args[0] == "register")
		{
			Connection sv(config.get_port(), config.get_ip());
			reg(&config, &sv);
		}
	else {
		Connection sv(config.get_port(), config.get_ip());
		if (!login(&config, &sv))
		{
			cout << "[mySvn] Login failed!\n";
			return 0;
		}

		if (args[0] == "init")
			client_init(args, &sv);
		else if (args[0] == "commit")
			client_commit(args, &sv);
		else if (args[0] == "clone")
			client_clone(args, &sv);
		else if (args[0] == "checkout")
			client_checkout(args, &sv);
		else if (args[0] == "revert")
			client_revert(args, &sv);
		else if (args[0] == "add")
			add_track(args);
		else if (args[0] == "rm")
			rm_track(args);
		else if (args[0] == "info")
			info(&config, &sv);
		else if (args[0] == "permissions")
			set_permission(args, &config, &sv);
		else if (args[0] == "log")
			see_log(args, &config, &sv);
		else cout << "[mySvn] Unkown command\n";
	}
	return 0;
}