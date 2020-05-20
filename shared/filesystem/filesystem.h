#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "File.h"
#include "hashlib2plus/trunk/src/hashlibpp.h"
#include <string>

class FileSystem
{
    void make_path(std::string path);
public:
    FileSystem(){ hasher = new sha256wrapper;}
    ~FileSystem(){ delete hasher;}
    hashwrapper* hasher;
    File read_file(std::string path);
    void dump_file(File foo);
    void dump_files(Files foo, Files old_files, std::vector<std::string> ignore);
    Files crawl(std::string path, std::vector<std::string> ignore);
};
#endif