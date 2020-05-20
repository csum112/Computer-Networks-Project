#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>

struct File
{
    std::string path;
    std::string content;
    mode_t st_mode;
    bool isDir;
    template <class Archive>
    void serialize( Archive & ar )
    {
        ar( path,content,st_mode,isDir );
    }
    File(){}
    File(std::string path, mode_t st_mode): path(path), content(""), st_mode(st_mode), isDir(true){}
    File(std::string path, std::string content, mode_t st_mode): path(path), content(content), st_mode(st_mode), isDir(false){}
};

typedef File Patch;
typedef std::vector<File> Files;
typedef std::vector<Patch> Patches;

#endif