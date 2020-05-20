#ifndef PATCHER_H
#define PATCHER_H
#include "../filesystem/filesystem.h"
class Patcher
{
public:
    File patch_file(File toPatch, Patch foo);
    Patch make_patch(File new_file, File old_file);
    Files patch_files(Files toPatch, Patches foo);
    Patches make_patches(Files new_files, Files old_files);
    
};
#endif