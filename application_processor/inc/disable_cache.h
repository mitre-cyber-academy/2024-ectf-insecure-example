#ifndef __cache_disable__
#define __cache_disable__


// ICC control
#include "icc.h"
#define ICC MXC_ICC0

void disable_cache() {
    // Disable cache..?
    MXC_ICC_Disable(ICC);
}

#endif