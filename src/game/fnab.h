#ifdef FNAB_H
#define FNAB_H

#include "types.h"

struct pathBranch {
    u8 active;
    u8 x;
    u8 y;
};

enum mapDirection {
    MAPDIR_UP,
    MAPDIR_RIGHT,
    MAPDIR_DOWN,
    MAPDIR_LEFT,
    MAPDIR_NO_PATH
};

#endif