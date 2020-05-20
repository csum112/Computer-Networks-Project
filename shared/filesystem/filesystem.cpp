#include "filesystem.h"
#include <string.h>
#include <unistd.h>
#include <stack>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#define BUFFER_SIZE 1024

File FileSystem::read_file(std::string path)
{
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat))
    {
        int err = errno;
        std::cout << "[Reader] Error reading file <" + path + "> code: " << err << std::endl;
    }

    if (S_ISREG(fileStat.st_mode) == true)
    {
        File foo(path, "", fileStat.st_mode);
        int fd;
        if ((fd = open(path.c_str(), O_RDONLY)) < 0)
        {
            int err = errno;
            std::cout << "[Reader] Error opening file: " << path << " Code: " << err << std::endl;
            std::cin.get();
        }

        int file_size = fileStat.st_size;


        char buffer[BUFFER_SIZE];
        int bytes_read;

        while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
        {
            std::string temp;
            temp.assign(buffer, buffer + bytes_read);
            foo.content += temp;
        }
        if(bytes_read < 0 || foo.content.size() < file_size)
        {
            int err = errno;
            std::cout<<"[Reader] Error reading file: " << path << " Code: " << err << std::endl;
        }
        close(fd);
        return foo;
    }
    else if (S_ISDIR(fileStat.st_mode) == true)
    {
        File foo(path, fileStat.st_mode);
        return foo;
    }
    else {
        std::cout << "[Reader] Unkwown file type: " << path << std::endl;
        std::cin.get();
    }
}

void FileSystem::dump_file(File foo)
{
    if (foo.isDir)
    {
        int status = mkdir(foo.path.c_str(), foo.st_mode);
    }
    else
    {
        int fd = open(foo.path.c_str(), O_CREAT | O_WRONLY, foo.st_mode);
        int CHUNK = 0;
        while(CHUNK < foo.content.size())
        {
            int to_write = ((foo.content.size() - CHUNK) < BUFFER_SIZE) ? (foo.content.size() - CHUNK) : BUFFER_SIZE;
            int bytes_wrote = write(fd, &(*foo.content.begin()) + CHUNK, to_write);
            CHUNK += bytes_wrote;
        }
        if(CHUNK < foo.content.size())
            std::cout << "[ERROR] Cannot write file << " + foo.path + " >> to disk\n";
        close(fd);
    }
}



void lazy_crawl(std::string path, std::vector<std::string> *results, std::vector<std::string> ignore)
{
    //Timeout
    std::stack<std::string> s;
    s.push(path);
    while(!s.empty())
    {
        std::string wp = s.top();
        s.pop();


            DIR *dirPointer;
            struct dirent *entry;
            dirPointer = opendir(wp.c_str());

            if (dirPointer != NULL)
            {
                while ((entry = readdir(dirPointer)))
                {
                    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                    {

                        //Builds path
                        std::string child = wp + "/" + entry->d_name;
                        std::cout << "[Spider] Found file/dir " + child + "\n";
                        
                        bool ok = true;
                        for(auto i : ignore)
                            if(child.find(i) != std::string::npos)
                            {
                                std::cout << "[Spider]File is ignored: " << child << std::endl;
                                ok = false;
                            }
                        if(ok)
                        {
                            results->push_back(child);
                            s.push(child);
                        }
                    }
                }
                closedir(dirPointer);
            }
    }
    std::cout << "[Spider] Finished crawling... \n";
}

Files FileSystem::crawl(std::string path, std::vector<std::string> ignore)
{
    std::vector<std::string> paths;
    lazy_crawl(path, &paths, ignore);

    Files result;
    int i = 0;
    for (auto path : paths)
    {
        result.push_back(read_file(path));
        std::cout << "[Spider] Processed " << ++i << "/" << paths.size() << " files\n";
    }
    
    return result;
}

bool string_nin_strings(std::string str, std::vector<std::string> arr)
{
    for(auto i : arr)
        if(str.find(i) != std::string::npos)
            return false;
    return true;
}

void FileSystem::dump_files(Files new_files, Files old_files, std::vector<std::string> ignore_list)
{
    //Asert files sorted alphabetically
    //This way dirs always come before files
    static auto comparator = [&](File x, File y) {
        return x.path < y.path;
    };
    sort(new_files.begin(), new_files.end(), comparator);
    sort(old_files.begin(), old_files.end(), comparator);

    //dump each file
    auto old_i = old_files.begin();
    auto new_i = new_files.begin();

    Files markedForRemoval;

    while (old_i != old_files.end() && new_i != new_files.end())
        if (old_i->path == new_i->path) //Is the same file
        {
            if(string_nin_strings(new_i->path, ignore_list))
                dump_file(*new_i);
            old_i++;
            new_i++;
        }
        else if (old_i->path > new_i->path) //New file added, we should delete it later
        {
            if(string_nin_strings(new_i->path, ignore_list))
                dump_file(*new_i);
            new_i++;
        } else {    //File was deleted
            //Remove
            markedForRemoval.push_back(*old_i);
            old_i++;
        }

    while(new_i != new_files.end()){    //Files left to add
            if(string_nin_strings(new_i->path, ignore_list))
                dump_file(*new_i);
            new_i++;
    }
    while(old_i != old_files.end()){    //Files left to remove
            markedForRemoval.push_back(*old_i);
            old_i++;
    }

    for(int i = markedForRemoval.size() - 1; i >= 0; i--)
    if(markedForRemoval[i].isDir == false)
        if (remove(markedForRemoval[i].path.c_str()))
        {
            int err = errno;
            std::cout << "[ERROR] Cannot delete file " + markedForRemoval[i].path + " : Code " << err << std::endl;
        } else;
    else
    {   //Must clean up dir first
        std::vector<std::string> junk, ignore;
        lazy_crawl(markedForRemoval[i].path, &junk, ignore);
        sort(junk.begin(), junk.end());

        for(int j = junk.size() - 1; j >= 0; j--)
        if (remove(junk[j].c_str()))
            {
                int err = errno;
                std::cout << "[ERROR] Cannot delete file " + junk[j] + " : Code " << err << std::endl;
            } else;

        if (rmdir(markedForRemoval[i].path.c_str()))
        {
            int err = errno;
            std::cout << "[ERROR] Cannot delete dir " + markedForRemoval[i].path + " : Code " << err << std::endl;
        } else;
    }


}