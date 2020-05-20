#include "database.h"
#include "../filesystem/filesystem.h"
#include <iostream>
using namespace std;

//Reference: sqlite documentation

Database::~Database()
{
    sqlite3_close(db);
}

Database::Database(std::string path)
{
    int rc;
    rc = sqlite3_open(path.c_str(), &db);
    if (rc)
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
}

string Database::get_repoid_client()
{
    mtx.lock();
    int rc;
    sqlite3_stmt *stmt = 0;
    string sql = "select repo_id from repo;";

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        fprintf(stderr, "No entries: %s\n", sqlite3_errmsg(db));

    int size = sqlite3_column_bytes(stmt, 0);
    string repo_id;
    const unsigned char* buffer = (unsigned char*)sqlite3_column_text(stmt, 0);
    repo_id.assign(buffer, buffer + size);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));

    fprintf(stderr, "[Database]: Size of package %d\n", size);
    mtx.unlock();
    return repo_id;
}

void Database::createClientRepo()
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };
    //---------Create Repo Table---------------
    string init_repo = "CREATE TABLE REPO(repo_id varchar(256), VERSION integer, latest_image blob, image_hash varchar(256));";
    rc = sqlite3_exec(db, init_repo.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table repo: %s\n", sqlite3_errmsg(db));

    //---------Create Commits Table---------------
    string ignore = "CREATE TABLE IGNORE(repo_id varchar(256), filename varchar(256));";
    rc = sqlite3_exec(db, ignore.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table ignore: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

void Database::createTables()
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };
    //---------Create Repo Table---------------
    string init_repo = "CREATE TABLE REPO(repo_id varchar(256) PRIMARY KEY, VERSION integer, latest_image blob, image_hash varchar(256));";
    rc = sqlite3_exec(db, init_repo.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table repo: %s\n", sqlite3_errmsg(db));

    //---------Create Commits Table---------------
    string init_commits = "CREATE TABLE COMMITS(repo_id varchar(256), VERSION integer, patch blob, hash varchar(256), PRIMARY KEY(REPO_ID, VERSION));";
    rc = sqlite3_exec(db, init_commits.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table commits: %s\n", sqlite3_errmsg(db));

    //---------Create Users Table---------------
    string users = "CREATE TABLE USERS(UUID varchar(256), USERNAME VARCHAR(256) PRIMARY KEY, PASSWORD VARCHAR(256));";
    rc = sqlite3_exec(db, users.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table users: %s\n", sqlite3_errmsg(db));

    //---------Create Permissions Table---------------
    string permissions = "CREATE TABLE PERMISSIONS(UUID varchar(256), REPO_ID VARCHAR(256), READ INTEGER, WRITE INTEGER, ERASE INTEGER, MODIFY INTEGER, PRIMARY KEY(UUID, REPO_ID));";
    rc = sqlite3_exec(db, permissions.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Can't create table permissions: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

void Database::initRepo(string repo_id, string hash)
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };

    //---------Insert Repo Data---------------
    string insert_row = "INSERT INTO REPO (REPO_ID, VERSION, IMAGE_HASH) VALUES('" + repo_id + "', " + "1" + ", '" + hash + "');";
    rc = sqlite3_exec(db, insert_row.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Failed insertion: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

void Database::update_latest(Bundle img, string hash, int version, string repo_id)
{
    mtx.lock();
    sqlite3_stmt *stmt = 0;
    int rc;
    string sql = "UPDATE REPO SET latest_image=?, image_hash=?, version=? where REPO_ID='" + repo_id + "';";
    fprintf(stderr, "Pushing image with size of: %d\n", img.size());

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed Prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_bind_blob(stmt, 1, &(*img.begin()), img.size(), SQLITE_STATIC);
    if (rc)
        fprintf(stderr, "Failed blob bind: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_bind_text(stmt, 2, hash.c_str(), hash.size(), SQLITE_STATIC);
    if (rc)
        fprintf(stderr, "Failed hash bind: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_bind_int(stmt, 3, version);
    if (rc)
        fprintf(stderr, "Failed version bind: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed Finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

Bundle Database::get_latest(string repo_id)
{
    mtx.lock();
    int rc;
    sqlite3_stmt *stmt = 0;
    string sql = "select latest_image from repo where repo_id='" + repo_id + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        fprintf(stderr, "No entries: %s\n", sqlite3_errmsg(db));

    int size = sqlite3_column_bytes(stmt, 0);
    Bundle data;
    const unsigned char* buffer = (unsigned char*)sqlite3_column_blob(stmt, 0);
    data.assign(buffer, buffer + size);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));

    fprintf(stderr, "[Database]: Size of package %d\n", size);
    mtx.unlock();
    return data;
}

string Database::get_latest_hash(string repo_id)
{
    mtx.lock();
    int rc;
    sqlite3_stmt *stmt = 0;
    string sql = "select IMAGE_HASH from REPO where repo_id='" + repo_id + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        fprintf(stderr, "No entries: %s\n", sqlite3_errmsg(db));

    const unsigned char *hash_buff = sqlite3_column_text(stmt, 0);
    int size = sqlite3_column_bytes(stmt, 0);
    string hash;
    hash.assign(hash_buff, hash_buff + size);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return hash;
}

int Database::get_commit_count(string repo_id)
{
    mtx.lock();
    int rc;
    sqlite3_stmt *stmt = 0;
    string sql = "select VERSION from REPO where repo_id='" + repo_id + "';";

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
        fprintf(stderr, "No entries: %s\n", sqlite3_errmsg(db));

    int version = sqlite3_column_int(stmt, 0);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return version;
}

void Database::push_commit(std::string repo_id, int version, std::string hash, Bundle blob)
{
    mtx.lock();
    string insert_row = "INSERT INTO commits (REPO_ID, VERSION, HASH, PATCH) VALUES('"
                        + repo_id + "', ?, '" + hash + "', ?);";
    sqlite3_stmt *stmt = 0;
    int rc;

    //---------Prepare statement---------------
    rc = sqlite3_prepare_v2(db, insert_row.c_str(), strlen(insert_row.c_str()) + 1, &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed Prepare: %s\n", sqlite3_errmsg(db));

    //---------Bind version---------------
    rc = sqlite3_bind_int(stmt, 1, version);
    if (rc)
        fprintf(stderr, "Failed version bind: %s\n", sqlite3_errmsg(db));

    //---------Bind patch---------------
    rc = sqlite3_bind_blob(stmt, 2, &(*blob.begin()), blob.size(), SQLITE_STATIC);
    if (rc)
        fprintf(stderr, "Failed blob bind: %s\n", sqlite3_errmsg(db));

    //---------Commit statement---------------
    rc = sqlite3_step(stmt);
    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed Finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

std::vector<Patches> Database::getPatches(std::string repo_id, int fromVersion, int toVersion, string& last_hash)
{
    mtx.lock();
    std::vector<Patches> results;
    string sql = "SELECT PATCH, HASH FROM COMMITS WHERE REPO_ID='" + repo_id + "'"
                 + " AND VERSION >= ? AND VERSION <= ? ORDER BY VERSION DESC;";
    sqlite3_stmt *stmt = 0;
    int rc;

    //---------Prepare statement---------------
    rc = sqlite3_prepare_v2(db, sql.c_str(), strlen(sql.c_str()) + 1, &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed Prepare: %s\n", sqlite3_errmsg(db));

    //---------Bind from version---------------
    rc = sqlite3_bind_int(stmt, 2, fromVersion);
    if (rc)
        fprintf(stderr, "Failed version bind: %s\n", sqlite3_errmsg(db));
    //---------Bind to version---------------
    rc = sqlite3_bind_int(stmt, 1, toVersion);
    if (rc)
        fprintf(stderr, "Failed version bind: %s\n", sqlite3_errmsg(db));

    //---------Read results---------------
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int size = sqlite3_column_bytes(stmt, 0);
        char* data = new char[size];
        memcpy(data, sqlite3_column_blob(stmt, 0), size);
        Bundle packaged;
        packaged.assign(data, data + size);
        Patches pkg = unpack(packaged);
        results.push_back(pkg);

        //Getting target version hash

        delete[] data;
        const unsigned char* hash_pointer = sqlite3_column_text(stmt, 1);
        size = sqlite3_column_bytes(stmt, 1);
        last_hash.assign(hash_pointer, hash_pointer + size);
    }

    //---------Finalizing statement---------------
    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed Finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return results;
}


void Database::drop_patches(std::string repo_id, int toVersion)
{
    mtx.lock();
    std::vector<Patches> results;
    string sql = "DELETE FROM COMMITS WHERE VERSION > ? and repo_id='" + repo_id + "';";
    sqlite3_stmt *stmt = 0;
    int rc;

    //---------Prepare statement---------------
    rc = sqlite3_prepare_v2(db, sql.c_str(), strlen(sql.c_str()) + 1, &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed Prepare: %s\n", sqlite3_errmsg(db));

    //---------Bind to version---------------
    rc = sqlite3_bind_int(stmt, 1, toVersion);
    if (rc)
        fprintf(stderr, "Failed version bind: %s\n", sqlite3_errmsg(db));

    //---------Read results---------------
    rc = sqlite3_step(stmt);
    if (rc)
        fprintf(stderr, "Failed Step: %s\n", sqlite3_errmsg(db));

    //---------Finalizing statement---------------
    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed Finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

void Database::add_ignore(std::string repo_id, std::string filename)
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };
    string insert_row = "INSERT INTO IGNORE (REPO_ID, FILENAME) VALUES('" + repo_id + "', '" + filename + "');";
    rc = sqlite3_exec(db, insert_row.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Failed ignore insertion: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

void Database::remove_ignore(std::string repo_id, std::string filename)
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };
    string sql = "DELETE FROM ignore WHERE REPO_ID='" + repo_id + "' and FILENAME='" + filename + "';";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &error);
    if (rc)
        fprintf(stderr, "Failed ignore deletion: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
}

std::vector<std::string> Database::get_ignore(std::string repo_id)
{
    mtx.lock();

    std::vector<std::string> ignore_list;
    string sql = "SELECT FILENAME FROM IGNORE WHERE REPO_ID='" + repo_id + "';";
    sqlite3_stmt *stmt = 0;
    int rc;

    //---------Prepare statement---------------
    rc = sqlite3_prepare_v2(db, sql.c_str(), strlen(sql.c_str()) + 1, &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed Prepare: %s\n", sqlite3_errmsg(db));


    //---------Read results---------------
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        int size = sqlite3_column_bytes(stmt, 0);
        const unsigned char* txtpointer = sqlite3_column_text(stmt, 0);
        std::string fileName;
        fileName.assign(txtpointer, txtpointer + size);
        ignore_list.push_back(fileName);
    }

    //---------Finalizing statement---------------
    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed Finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return ignore_list;
}


std::string Database::login(std::string username, std::string password)
{
    mtx.lock();
    FileSystem fs;
    string pw_hash = fs.hasher->getHashFromString(password);
    string sql = "SELECT UUID FROM USERS WHERE USERNAME='" + username + "' AND PASSWORD='" + pw_hash + "';";
    sqlite3_stmt * stmt = 0;
    int rc;


    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "Invalid login!: %s\n", sqlite3_errmsg(db));
        rc = sqlite3_finalize(stmt);
        if (rc)
            fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return "";
    }

    string uuid;
    const unsigned char* buffer;
    int size = sqlite3_column_bytes(stmt, 0);
    buffer = sqlite3_column_text(stmt, 0);
    uuid.assign(buffer, buffer + size);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return uuid;
}


bool Database::insert_user(std::string uuid, std::string username, std::string password)
{
    mtx.lock();
    FileSystem fs;
    string pw_hash = fs.hasher->getHashFromString(password);
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };

    //---------Insert User Data---------------
    string new_user = "INSERT INTO USERS(UUID, USERNAME, PASSWORD) VALUES('" + uuid + "', '" + username + "', '" + pw_hash + "');";
    rc = sqlite3_exec(db, new_user.c_str(), callback, 0, &error);
    if (rc)
    {
        fprintf(stderr, "Failed insertion: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return false;
    }
    mtx.unlock();
    return true;
}

char Database::get_permissions(std::string uuid, std::string repo_id)
{
    mtx.lock();
    string sql = "SELECT READ, WRITE, ERASE, MODIFY FROM PERMISSIONS WHERE UUID='" + uuid + "' AND REPO_ID='" + repo_id + "';";
    sqlite3_stmt * stmt = 0;
    int rc;


    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "No permissions: %s\n", sqlite3_errmsg(db));
        rc = sqlite3_finalize(stmt);
        if (rc)
            fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return 0;
    }

    char read, write, erase, modify;

    read = sqlite3_column_int(stmt, 0);
    write = sqlite3_column_int(stmt, 1);
    erase = sqlite3_column_int(stmt, 2);
    modify = sqlite3_column_int(stmt, 3);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return (read << 3) | (write << 2) | (erase << 1) | modify;
}

void Database::add_permissions(std::string uuid, std::string repo_id, bool read, bool write, bool erase, bool modify)
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };

    string sql_insert = (string)"INSERT INTO PERMISSIONS (UUID, REPO_ID, READ, WRITE, ERASE, MODIFY) VALUES(" +
                        "'" + uuid + "', '" + repo_id + "', " + to_string(read) + ", " + to_string(write) + ", " + to_string(erase) + ", " + to_string(modify)
                        + ");";
    rc = sqlite3_exec(db, sql_insert.c_str(), callback, 0, &error);
    if (!rc) {
        mtx.unlock();
        return;
    }

    string sql_modify = "UPDATE PERMISSIONS SET READ='" + to_string(read) +
                        "', WRITE='" + to_string(write) +
                        "', ERASE='" + to_string(erase) +
                        "', MODIFY='" + to_string(modify) +
                        "' WHERE REPO_ID='" + repo_id + "' AND UUID='" + uuid + "';";
    rc = sqlite3_exec(db, sql_modify.c_str(), callback, 0, &error);
    if (rc)
    {
        fprintf(stderr, "Failed to add permissions: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return;
    }
    mtx.unlock();
}

void Database::strip_permissions(std::string uuid, std::string repo_id)
{
    mtx.lock();
    char *error = 0;
    int rc;
    auto callback = [](void *NotUsed, int argc, char **argv, char **azColName) {
        return 0;
    };

    string sql = "DELETE FROM PERMISSIONS WHERE REPO_ID='" + repo_id + "' AND UUID='" + uuid + "';";
    rc = sqlite3_exec(db, sql.c_str(), callback, 0, &error);
    if (rc)
    {
        fprintf(stderr, "Failed delete: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return ;
    }
    mtx.unlock();
}

std::string Database::user2uuid(std::string username)
{
    mtx.lock();
    string sql = "SELECT UUID FROM USERS WHERE USERNAME='" + username + "';";
    sqlite3_stmt * stmt = 0;
    int rc;

    rc = sqlite3_prepare_v2(db, sql.c_str(), sql.size(), &stmt, 0);
    if (rc)
        fprintf(stderr, "Failed prepare: %s\n", sqlite3_errmsg(db));

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        fprintf(stderr, "Invalid username!: %s\n", sqlite3_errmsg(db));
        rc = sqlite3_finalize(stmt);
        if (rc)
            fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
        mtx.unlock();
        return "";
    }

    string uuid;
    const unsigned char* buffer;
    int size = sqlite3_column_bytes(stmt, 0);
    buffer = sqlite3_column_text(stmt, 0);
    uuid.assign(buffer, buffer + size);

    rc = sqlite3_finalize(stmt);
    if (rc)
        fprintf(stderr, "Failed finalize: %s\n", sqlite3_errmsg(db));
    mtx.unlock();
    return uuid;
}

