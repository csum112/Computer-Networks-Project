#ifndef SERVERLOGIC_H
#define SERVERLOGIC_H


#include <unistd.h>
#include <string>
#include <iostream>
#include <time.h>
#include "../shared/connection.h"
#include "../shared/client-database/database.h"
#include "../shared/bundler/bundler.h"
#include "../shared/patcher/Patcher.h"
#include "../shared/filesystem/filesystem.h"
using namespace std;

void init_req(Connection* c, Database* db, string uuid);
void commit_req(Connection* c, Database *db, string uuid);
void clone_req(Connection* c, Database* db, string uuid);
void checkout_req(Connection* c, Database* db, string uuid);
void revert_req(Connection* c, Database* db, string uuid);
void info_req(Connection* c, Database* db, string uuid);
void modify_req(Connection* c, Database* db, string uuid);
void log_req(Connection* c, Database* db, string uuid);

bool has_read(char permissions)
{
	return permissions & (1 << 3);
}

bool has_write(char permissions)
{
	return permissions & (1 << 2);
}

bool has_erase(char permissions)
{
	return permissions & (1 << 1);
}

bool has_modify(char permissions)
{
	return permissions & 1;
}

void logic(int fd, Database* db)
{
	Connection foo(fd);
	string uuid;
	string login_or_reg = foo.receive();
	//TODO: Assert uuid
	if (login_or_reg == "LOGIN")
	{
		string username = foo.receive();
		string password = foo.receive();
		uuid = db->login(username, password);
		if (uuid == "")
		{
			cout << "[LOGIN] Failed auth\n";
			foo.send("0");
			return;
		}
		foo.send("1");
	} else if (login_or_reg == "REGISTER")
	{
		string username = foo.receive();
		string password = foo.receive();

		FileSystem fs;
		string brute_uuid = username + to_string(time(NULL));
		uuid = fs.hasher->getHashFromString(brute_uuid);

		bool success =  db->insert_user(uuid, username, password);

		if (success == true) foo.send("1");
		else foo.send("0");

		return;
	} else return;

	//Get request type, should be the first thing sent
	int req_type = stoi(foo.receive());
	switch (req_type)
	{
	case 0:
		init_req(&foo, db, uuid);
		break;
	case 1:
		commit_req(&foo, db, uuid);
		break;
	case 2:
		clone_req(&foo, db, uuid);
		break;
	case 3:
		checkout_req(&foo, db, uuid);
		break;
	case 4:
		revert_req(&foo, db, uuid);
		break;
	case 5:
		info_req(&foo, db, uuid);
		break;
	case 6:
		modify_req(&foo, db, uuid);
		break;
	case 7:
		log_req(&foo, db, uuid);
		break;
	default:
		cout << "[Server] Bad request.\n";
	}
	close(fd);
}

void init_req(Connection* c, Database* db, string uuid)
{
	//Begin init request
	//Expect Repo_ID, Initial_Image, Hash of Initial image
	FileSystem fs;
	string junk = uuid + to_string(time(NULL));
	string REPO_ID = fs.hasher->getHashFromString(junk);
	string IMAGE = c->receive();
	string HASH = c->receive();

	db->initRepo(REPO_ID, HASH);
	db->update_latest(IMAGE, HASH, 1, REPO_ID);
	db->add_permissions(uuid, REPO_ID, 1, 1, 1, 1);
	c->send(REPO_ID);

}

void commit_req(Connection* c, Database* db, string uuid)
{
	//Begin commit request
	string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_write(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	int version = db->get_commit_count(REPO_ID);
	c->send(to_string(version));

	//Sending latest image and hash
	c->send(db->get_latest(REPO_ID));
	c->send(db->get_latest_hash(REPO_ID));

	//Get Latest Image Patches and Hash
	Bundle latest_image = c->receive();
	Bundle patches = c->receive();
	string hash = c->receive();
	db->push_commit(REPO_ID, version, db->get_latest_hash(REPO_ID), patches);
	db->update_latest(latest_image, hash, version + 1, REPO_ID);
}

void clone_req(Connection* c, Database* db, string uuid)
{
	//Begin commit request
	const string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_read(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	const int version = db->get_commit_count(REPO_ID);
	const Bundle latest = db->get_latest(REPO_ID);
	const string hash = db->get_latest_hash(REPO_ID);
	c->send(to_string(version));
	c->send(latest);
	c->send(hash);
}

void checkout_req(Connection* c, Database* db, string uuid)
{
	//Receive repo_id and version
	//Send patches and expected hash
	//Assert wanted version is between 1 and latest
	string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_read(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	const int client_version = stoi(c->receive());
	const int wanted_version = stoi(c->receive());
	const int latest_version = db->get_commit_count(REPO_ID);
	string hash;
	if (1 > wanted_version || wanted_version > latest_version)
	{
		cout << "[Checkout request error] Outside boundries\n";
		c->send("-1");
		return;
	}
	if (wanted_version <= client_version)
	{
		vector<Patches> arr = db->getPatches(REPO_ID, client_version, wanted_version, hash);
		//Sending the patches one at a time
		c->send(to_string(arr.size()));
		for (auto patch : arr)
			c->send(pack(patch));
		c->send(hash);
	} else {
		//Making custom patch
		//Building wanted state
		Patcher dmt;
		string wanted_hash = db->get_latest_hash(REPO_ID);
		vector<Patches> arr = db->getPatches(REPO_ID, latest_version, wanted_version, wanted_hash);
		Bundle img = db->get_latest(REPO_ID);
		Files wanted = unpack(img);
		int i = 0;
		for (auto patches : arr)
		{
			cout << "[Checkout request] Applying patch " << i << "/" << arr.size() << endl;
			wanted = dmt.patch_files(wanted, patches);
			i++;
		}

		//Building client state
		arr.clear();
		arr = db->getPatches(REPO_ID, latest_version, client_version, hash);
		Files client_files = unpack(img);
		i = 0;
		for (auto patches : arr)
		{
			cout << "[Checkout request] Applying patch " << i << "/" << arr.size() << endl;
			client_files = dmt.patch_files(client_files, patches);
			i++;
		}

		//Making patch from client -> wanted
		Patches to_send = dmt.make_patches(client_files, wanted);
		c->send("1");
		c->send(pack(to_send));
		c->send(wanted_hash);
	}
}

void revert_req(Connection* c, Database* db, string uuid)
{
	string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_erase(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	const int wanted_version = stoi(c->receive());
	const int latest_version = db->get_commit_count(REPO_ID);

	if (1 > wanted_version || wanted_version > latest_version)
	{
		cout << "[Revert request error] Outside boundries\n";
		c->send("-1");
		return;
	}

	//Building old image
	Bundle img = db->get_latest(REPO_ID);
	Files foo = unpack(img);
	string wanted_hash;
	vector<Patches> arr = db->getPatches(REPO_ID, latest_version, wanted_version, wanted_hash);
	Patcher dmt;
	int i = 0;
	for (auto patches : arr)
	{
		cout << "[Checkout request] Applying patch " << i << "/" << arr.size() << endl;
		foo = dmt.patch_files(foo, patches);
		i++;
	}


	//Checking hashes
	FileSystem fs;
	img = pack(foo);
	string hash = fs.hasher->getHashFromString(img);
	if (hash != wanted_hash)
	{
		cout << "[ERROR] Image Checksums dont match up! \n";
		c->send("-1");
		return;
	}

	//Commit changes
	db->update_latest(img, wanted_hash, wanted_version, REPO_ID);
	db->drop_patches(REPO_ID, wanted_version - 1);


	//Send to client
	c->send(to_string(arr.size()));
	for (auto patch : arr)
		c->send(pack(patch));
	c->send(hash);
}


void info_req(Connection* c, Database* db, string uuid)
{

	string REPO_ID = c->receive();
	char permissions = db->get_permissions(uuid, REPO_ID);
	cout << "Permissions: " << int(permissions) << endl;
	int latest_version = db->get_commit_count(REPO_ID);
	c->send(to_string(latest_version));
	c->send(to_string(has_read(permissions)));
	c->send(to_string(has_write(permissions)));
	c->send(to_string(has_erase(permissions)));
	c->send(to_string(has_modify(permissions)));
}

void modify_req(Connection* c, Database* db, string uuid)
{
	string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_modify(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	cout << "everything ok \n";
	string permission = c->receive();
	bool value = stoi(c->receive());
	string username = c->receive();
	string target = db->user2uuid(username);

	permissions = db->get_permissions(target, REPO_ID);
	bool r = has_read(permissions);
	bool w = has_write(permissions);
	bool e = has_erase(permissions);
	bool m = has_modify(permissions);

	if (permission == "read")
		r = value;
	else if (permission == "write")
		w = value;
	else if (permission == "erase")
		e = value;
	else if (permission == "modify")
		m = value;

	if ( !r && !w && !e && !m)
		db->strip_permissions(target, REPO_ID);
	else db->add_permissions(target, REPO_ID, r, w, e, m);
}

void log_req(Connection* c, Database* db, string uuid)
{
	string REPO_ID = c->receive();
	//--------assert has permissions
	char permissions = db->get_permissions(uuid, REPO_ID);
	if (!has_read(permissions))
	{
		c->send("0");
		return;
	}
	c->send("1");
	//------------------------------
	int version = stoi(c->receive());
	const int latest_version = db->get_commit_count(REPO_ID);

	if(version < 1 || version >= latest_version)
	{
		c->send("0");
		return;
	}
	c->send("1"); //OK
	string hash;
	Patches to_send = (db->getPatches(REPO_ID, version + 1, version, hash))[0];
	c->send(pack(to_send));
}

#endif
