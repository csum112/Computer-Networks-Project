#ifndef BUNDLER_H
#define BUNDLER_H
#include <vector>
#include <string>
#include "../filesystem/File.h"

typedef std::string Bundle;

Bundle pack(Files foo);
Files unpack(Bundle buffer);

#endif