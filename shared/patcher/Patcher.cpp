#include "Patcher.h"
#include "diff-match-patch-cpp-stl/diff_match_patch.h"
#include <algorithm>
#include <iostream>

using namespace std;

Patch Patcher::make_patch(File new_file, File old_file)
{
    diff_match_patch<std::string> dmp;
    std::string patch_content = dmp.patch_toText(dmp.patch_make(new_file.content, old_file.content));
    return Patch(new_file.path, patch_content, old_file.st_mode);
}

Patches Patcher::make_patches(Files new_files, Files old_files)
{
    Patches result;
    //Assert files are sorted alphabetically
    static auto comparator = [&](File x, File y) {
        return x.path < y.path;
    };
    sort(new_files.begin(), new_files.end(), comparator);
    sort(old_files.begin(), old_files.end(), comparator);

    auto old_i = old_files.begin();
    auto new_i = new_files.begin();

    while (old_i != old_files.end() && new_i != new_files.end())
        if (old_i->path == new_i->path) //Is the same file
        {
            result.push_back(make_patch(*new_i, *old_i));
            old_i++;
            new_i++;
        }
        else if (old_i->path > new_i->path) //New file added, we should delete it later
        {
            //Do nothing
            new_i++;
        } else {    //File was deleted
            //Add null
            cout << "[Patcher] Making patch for deleted file: " + old_i->path << endl;
            File null_file(old_i->path, "", old_i->st_mode);
            result.push_back(*old_i);
            old_i++;
        }

    while(old_i != old_files.end()){    //File was deleted
            cout << "[Patcher] Making patch for deleted file: " + old_i->path << endl;
            File null_file(old_i->path, "", old_i->st_mode);
            result.push_back(*old_i);
            old_i++;
        }
    
    while(new_i != new_files.end()){ 
            new_i++;
        }

    return result;
}

File Patcher::patch_file(File toPatch, Patch foo)
{
    //Assert path is the same

    string path = foo.path;
    diff_match_patch<std::string> dmp;
    auto patch = dmp.patch_fromText(foo.content);
    string content = dmp.patch_apply(patch, toPatch.content).first;
    return File(path, content, foo.st_mode);
}

Files Patcher::patch_files(Files toPatch, Patches foo)
{
    Files result;
    //Assert files are sorted alphabetically
    static auto comparator = [&](File x, File y) {
        return x.path < y.path;
    };
    sort(toPatch.begin(), toPatch.end(), comparator);
    sort(foo.begin(), foo.end(), comparator);

    auto file = toPatch.begin();
    auto patch = foo.begin();

    while (file != toPatch.end() && patch != foo.end())
    if (file->path == patch->path) //Is the same file
    {
        //Assert same files
        result.push_back(patch_file(*file, *patch));
        file++;
        patch++;
    }
    else if (file->path < patch->path) //
    {
        file++;
    } else {    //Old deleted file
        cout << "[Patcher] Applying patch for deleted file: " + patch->path << endl;
        File null_file(patch->path, "", patch->st_mode);
        result.push_back(*patch);
        patch++;
    }

    while(patch != foo.end())
    {
        cout << "[Patcher] Applying patch for deleted file: " + patch->path << endl;
        File null_file(patch->path, "", patch->st_mode);
        result.push_back(*patch);
        patch++;
    }

    while(file != toPatch.end())
    {
        file++;
    }
    
    return result;
}