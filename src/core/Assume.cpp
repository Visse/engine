#include "core/Assume.h"

#include <cstdio>
#include <cstdlib>
#include <csignal>

namespace Core 
{    
    void AssumptionFailed(const char* expression, const char* file, int line, const char* function)
    {
        std::fprintf( stderr, "Assumption \"%s\" failed in file %s:%i in function %s\n", expression, file, line, function );
        std::raise( SIGABRT );
        std::exit(-1);
    }
}
