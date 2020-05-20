#ifndef COMMANDS_H
#define COMMANDS_H


#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include "../shared/connection.h"
#include "../shared/bundler/bundler.h"
#include "../shared/filesystem/filesystem.h"
#include "../shared/client-database/database.h"
#include "../shared/patcher/Patcher.h"
#include "config.h"

#define MANIFEST ".mySvn.manifest"


using namespace std;

typedef vector<string> strings;

void client_init(strings params, Connection* sv)
{

	FileSystem fs;
	vector<string> ignore;
	ignore.push_back(".DS_Store");
	Files dir = fs.crawl(".", ignore);

	//Check for manifest
	for (auto file = dir.begin(); file != dir.end(); file++)
		if (file->path.find(MANIFEST) != string::npos)
		{
			cout << "[Init]Repo already exists. \n";
			return;
		}

	//Create image
	Bundle image = pack(dir);
	cout << "[Init] Size of IMAGE is " << image.size() << endl;
	//Create SHA256 of the latest image
	string hash = fs.hasher->getHashFromString(image);

	//Sending to server

	sv->send("0");	//INIT_REQ
	sv->send(image);
	sv->send(hash);
	string REPO_ID = sv->receive();

	//Saving manifest
	Database db(MANIFEST);
	db.createClientRepo();
	db.initRepo(REPO_ID, hash);
	db.add_ignore(REPO_ID, MANIFEST);
	db.add_ignore(REPO_ID, ".DS_Store");
}

void client_commit(strings params, Connection* sv)
{
	//Assert manifest exists
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Commit] Manifest not found...\n";
		return;
	}


	Database db(MANIFEST);
	FileSystem fs;
	Patcher dmt;
	string REPO_ID = db.get_repoid_client();
	vector<string> ignore = db.get_ignore(REPO_ID);
	Files current = fs.crawl(".", ignore);

	cout << "[Commit] Packing image...\n";
	Bundle current_image = pack(current);
	cout << "[Commit] Hashing image...\n";
	string current_hash = fs.hasher->getHashFromString(current_image);

	// --------------------- Server interaction
	sv->send("1"); //COMMIT_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------
	int version = stoi(sv->receive()) + 1;
	cout << "[Commit]Commiting version: " << version << endl;
	cout << "[Commit] Pulling latest image...\n";
	Bundle old_image = sv->receive();
	cout << "[Commit] Pulling latest hash...\n";
	string old_hash = sv->receive();

	cout << "[Commit] Unpacking latest image...\n";
	Files old = unpack(old_image);
	Patches foo = dmt.make_patches(current, old);
	Bundle patch_img = pack(foo);

	//--------- TESTING IMAGE
	{

		cout << "[Commit] Testing image\n";
		Files test = dmt.patch_files(current, foo);
		std::sort(test.begin(), test.end(), [](File x, File y) {return x.path < y.path;});
		std::sort(old.begin(), old.end(), [](File x, File y) {return x.path < y.path;});
		bool ok = true;
		for (int i = 0; i < test.size(); i++)
			if (!(test[i].path == old[i].path && test[i].content == old[i].content && test[i].st_mode == old[i].st_mode))
			{
				cout << "--------------------------------\n";
				cout << "[Test] File " << i << " ERROR!:\n";
				cout << "Old file path: " << old[i].path << endl;
				cout << "New file path: " << test[i].path << endl;
				cout << "Old file hash: " << fs.hasher->getHashFromString(old[i].content) << endl;
				cout << "New file hash: " << fs.hasher->getHashFromString(test[i].content) << endl;
				cout << "--------------------------------\n";
				cin.get();
				ok = false;
			} else cout << "[Commit Test] Patching file " << i << "/" << test.size() << endl;
		if (ok) cout << "[Commit] Test passed succesfully\n";
		else {
			cout << "[Commit] Test failled\n";
			return;
		}
	}

	//Sending Patches
	sv->send(current_image);
	sv->send(patch_img);
	sv->send(current_hash);
	db.update_latest(current_image, current_hash, version, REPO_ID);
}

void client_clone(strings params, Connection* sv)
{
	if (params.size() != 2)
	{
		cout << "BAD ARGS\n";
		return;
	}
	string REPO_ID = params[1];
	FileSystem fs;
	vector<string> ignore;
	ignore.push_back(".DS_Store");
	Files dir = fs.crawl(".", ignore);
	//Check for manifest
	for (auto file = dir.begin(); file != dir.end(); file++)
		if (file->path.find(MANIFEST) != string::npos)
		{
			cout << "[Init]Repo already exists. \n";
			return;
		}


	sv->send("2"); //CLONE_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------
	int version = stoi(sv->receive());
	Bundle image = sv->receive();
	string hash = sv->receive();
	Files new_files = unpack(image);

	fs.dump_files(new_files, dir, ignore);
	Database db(MANIFEST);
	db.createTables();
	db.initRepo(REPO_ID, hash);
	db.add_ignore(REPO_ID, MANIFEST);
	db.add_ignore(REPO_ID, ".DS_Store");
	db.update_latest(image, hash, version, REPO_ID);
}


void client_checkout(strings params, Connection* sv)
{
	if (params.size() != 2)
	{
		cout << "[Error] mysvn checkout <version> expected\n";
		return;
	}

	//Assert manifest exists
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Checkout] Manifest not found...\n";
		return;
	}

	Database db(MANIFEST);
	FileSystem fs;
	Patcher dmt;
	string REPO_ID = db.get_repoid_client();
	vector<string> ignore = db.get_ignore(REPO_ID);
	Files current = fs.crawl(".", ignore);

	Bundle img = db.get_latest(REPO_ID);
	int version = db.get_commit_count(REPO_ID);
	int expected_version = stoi(params[1]);

	sv->send("3"); //CHECKOUT_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------
	sv->send(to_string(version));
	sv->send(to_string(expected_version));

	//Await patches and hash
	int to_be_rec = stoi(sv->receive());
	if (to_be_rec <= 0)
	{
		cout << "[Checkout] Received bad patches\n";
		return;
	}

	vector<Patches> arr;
	for (int i = 0; i < to_be_rec; i++)
	{
		cout << "[Checkout] Unpacking patch " << i << endl;
		arr.push_back(unpack(sv->receive()));
	}
	string expected_hash = sv->receive();

	//Apply patches
	Files foo = unpack(img);
	int i = 0;
	for (auto patches : arr)
	{
		cout << "[Checkout] Applying patch " << i << "/" << patches.size() << endl;
		foo = dmt.patch_files(foo, patches);
		i++;
	}

	//Checking hashes
	img = pack(foo);
	string hash = fs.hasher->getHashFromString(img);
	if (hash != expected_hash)
	{
		cout << "[ERROR] Image Checksums dont match up! \n";
		return;
	}
	fs.dump_files(foo, current, ignore);
	db.update_latest(img, expected_hash, expected_version, REPO_ID);
}


void client_revert(strings params, Connection* sv)
{
	if (params.size() != 2)
	{
		cout << "[Error] mysvn revert <version> expected\n";
		return;
	}

	//Assert manifest exists
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}

	Database db(MANIFEST);
	FileSystem fs;
	Patcher dmt;
	string REPO_ID = db.get_repoid_client();
	vector<string> ignore = db.get_ignore(REPO_ID);
	Files current = fs.crawl(".", ignore);
	Bundle img = db.get_latest(REPO_ID);
	int expected_version = stoi(params[1]);


	//Sending request
	sv->send("4"); //CHECKOUT_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------
	sv->send(params[1]); //Wanted version


	//Make changes locally
	//Await patches and hash
	int to_be_rec = stoi(sv->receive());
	if (to_be_rec <= 0)
	{
		cout << "[Revert] Received bad patches\n";
		return;
	}

	vector<Patches> arr;
	for (int i = 0; i < to_be_rec; i++)
	{
		cout << "[Revert] Unpacking patch " << i << endl;
		arr.push_back(unpack(sv->receive()));
	}
	string expected_hash = sv->receive();

	//Apply patches
	Files foo = unpack(img);
	int i = 0;
	for (auto patches : arr)
	{
		cout << "[Revert] Applying patch " << i << "/" << patches.size() << endl;
		foo = dmt.patch_files(foo, patches);
		i++;
	}

	//Checking hashes
	img = pack(foo);
	string hash = fs.hasher->getHashFromString(img);
	if (hash != expected_hash)
	{
		cout << "[ERROR] Image Checksums dont match up! \n";
		return;
	}

	fs.dump_files(foo, current, ignore);
	db.update_latest(img, expected_hash, expected_version, REPO_ID);

}

void add_track(strings params)
{
	//Assert manifest exists
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}
	if (params.size() == 1)
	{
		cout << "Not enough arguments. \n";
		return;
	}

	if (params.size() > 2)
	{
		cout << "Too many arguments. \n";
		return;
	}

	Database db(MANIFEST);
	string REPO_ID = db.get_repoid_client();
	db.remove_ignore(REPO_ID, params[1]);
}

void rm_track(strings params)
{
	//Assert manifest exists
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}
	if (params.size() == 1)
	{
		cout << "Not enough arguments. \n";
		return;
	}

	if (params.size() > 2)
	{
		cout << "Too many arguments. \n";
		return;
	}

	Database db(MANIFEST);
	string REPO_ID = db.get_repoid_client();
	db.add_ignore(REPO_ID, params[1]);
}

bool login(Persistent_data* config, Connection* sv)
{

	if (config->get_username() == "")
	{
		cout << "[mySvn] You need to register first or add a username. $mysvn set username <<username>> \n";
		return false;
	}

	cout << "[mySvn] Please enter the password for <<" + config->get_username() + ">>: ";

	string password = "";
	cin >> password;
	sv->send("LOGIN");
	sv->send(config->get_username());
	sv->send(password);
	return stoi(sv->receive());
}

void reg(Persistent_data* config, Connection* sv)
{
	string username, password;
	cout << "[mySvn] Please enter a username: ";
	cin >> username;
	cout << "[mySvn] Please enter a password: ";
	cin >> password;
	sv->send("REGISTER");
	sv->send(username);
	sv->send(password);

	bool response = stoi(sv->receive());
	if (!response)
	{
		cout << "[mySvn] Username is taken or server is down\n";
		return;
	}

	cout << "[mySvn] Account created succesfully\n";
	config->set_username(username);
}

void info(Persistent_data* config, Connection* sv)
{
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}
	Database db(MANIFEST);
	string REPO_ID = db.get_repoid_client();
	int local_version = db.get_commit_count(REPO_ID);
	sv->send("5"); //INFO_REQ
	sv->send(REPO_ID);
	int global_version = stoi(sv->receive());
	bool READ_PERMISSION = stoi(sv->receive());
	bool WRITE_PERMISSION = stoi(sv->receive());
	bool ERASE_PERMISSION = stoi(sv->receive());
	bool MODIFY_PERMISSION = stoi(sv->receive());

	cout << "---------------------------------\n";
	cout << "[mySvn] Repo Meta \n";
	cout << "Repo: " << REPO_ID << endl;
	cout << "Local Version: " << local_version << endl;
	cout << "Global Version: " << global_version << endl;
	cout << "Permissions for user " << config->get_username() << ": \n";
	cout << "Read: " <<  ((READ_PERMISSION) ? "True" : "False") << endl;
	cout << "Write: " <<  ((WRITE_PERMISSION) ? "True" : "False") << endl;
	cout << "Erase: " <<  ((ERASE_PERMISSION) ? "True" : "False") << endl;
	cout << "Modify: " <<  ((MODIFY_PERMISSION) ? "True" : "False") << endl;
	cout << "---------------------------------\n";
}

void set_permission(strings params, Persistent_data* config, Connection* sv)
{
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}
	if (params.size() != 4)
	{
		cout << "[mySvn] Bad Args." << endl;
		cout << "usage: mysvn permisions <permission> <true/false> <username>\n";
		return ;
	}
	if (params[2] != "true" && params[2] != "false")
	{
		cout << "[mySvn] Bad Args." << endl;
		cout << "usage: mysvn permissions <permission> <true/false> <username>\n";
		return ;
	}

	string permission = params[1];
	int value = (params[2] == "true");
	string username = params[3];

	Database db(MANIFEST);
	string REPO_ID = db.get_repoid_client();

	//Sending request
	sv->send("6"); //PERM_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------

	sv->send(permission);
	sv->send(to_string(value));
	sv->send(username);
}






void see_log(strings params, Persistent_data* config, Connection* sv)
{
	if ( access( MANIFEST, F_OK ) < 0 )
	{
		cout << "[Revert] Manifest not found...\n";
		return;
	}

	Database db(MANIFEST);
	string REPO_ID = db.get_repoid_client();
	int local_version = db.get_commit_count(REPO_ID);
	//Sending request
	sv->send("7"); //LOG_REQ
	sv->send(REPO_ID);
	//------Permissions
	bool ok = stoi(sv->receive());
	if (!ok)
	{
		cout << "[mySvn] Not enough permissions\n";
		return;
	}
	//-----------------

	sv->send(to_string(local_version - 1));

	if (!stoi(sv->receive()))
	{
		cout << "[Log] Bad Request\n";
		return;
	}

	Patcher dmt;
	Bundle patch_img = sv->receive();
	Patches foo = unpack(patch_img);

	Bundle latest = db.get_latest(REPO_ID);
	Files lst = unpack(latest);
	Files old = dmt.patch_files(lst, foo);

	Patches diff = dmt.make_patches(old, lst);

	for (auto patch : diff)
	{
		cout << "----------------------------\n";
		cout << "File path: " + patch.path << endl;
		cout << patch.content << endl;
		cout << "----------------------------\n";

	}
}




#endif