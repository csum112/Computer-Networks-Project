#include "bundler.h"
#include <memory.h>
#include <iostream>
#include <sstream>
//Cereal defines

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/complex.hpp>


Bundle pack(Files foo)
{
    std::sort(foo.begin(), foo.end(), [](File x, File y) {return x.path < y.path;});
    std::stringstream ss;
    {
        cereal::BinaryOutputArchive archive(ss);
        archive(CEREAL_NVP (foo));
    }
    return ss.str();
}


Files unpack(Bundle buffer)
{
    Files foo;
    std::stringstream ss;
    ss.str(buffer);
    {
        cereal::BinaryInputArchive archive(ss);
        archive(foo);
    }
    return foo;
}

