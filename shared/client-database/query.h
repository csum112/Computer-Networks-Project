#ifndef QUERRY_H
#define QUERRY_H

#include <string>

using namespace std;

static string init_repo = "CREATE TABLE REPO(varchar repo_id(256), integer version, blob latest_image);";

#endif