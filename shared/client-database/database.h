#ifndef DATABASE_H
#define DATABASE_H

#include "sqlite3/sqlite3.h"
#include <string>
#include <vector>
#include "../bundler/bundler.h"
#include <mutex>

class Database
{
    sqlite3 *db;
    std::mutex mtx;
public:
    Database(std::string path);
    void createClientRepo();
    void createTables();
    void initRepo(std::string repo_id, std::string hash);
    void update_latest(Bundle img, std::string hash, int latest, std::string repo_id);
    void push_commit(std::string repo_id, int version, std::string hash, Bundle blob);
    Bundle get_latest(std::string repo_id);
    int get_commit_count(std::string repo_id);
    std::string get_latest_hash(std::string repo_id);
    std::vector<Patches> getPatches(std::string repo_id, int fromVersion, int toVersion, std::string& last_hash);
    void drop_patches(std::string repo_id, int toVersion);
    void add_ignore(std::string repo_id, std::string filename);
    void remove_ignore(std::string repo_id, std::string filename);
    std::vector<std::string> get_ignore(std::string repo_id);
    std::string get_repoid_client();
    std::string login(std::string username, std::string password);
    bool insert_user(std::string uuid, std::string username, std::string password);
    char get_permissions(std::string uuid, std::string repo_id);
    void add_permissions(std::string uuid, std::string repo_id, bool read, bool write, bool erase, bool modify);
    void strip_permissions(std::string uuid, std::string repo_id);
    std::string user2uuid(std::string username);
    ~Database();
};

#endif