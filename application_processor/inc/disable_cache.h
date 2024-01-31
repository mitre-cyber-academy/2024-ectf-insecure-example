// ICC control
#include "icc.h"
#define ICC MXC_ICC0

void disable_cache() {
    // Disable cache..?
    MXC_ICC_Disable(ICC);
}