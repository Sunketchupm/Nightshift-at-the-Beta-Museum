#ifndef FNAB_H
#define FNAB_H

#include "types.h"

struct fnabEnemyProperties {
    u8 homex;
    u8 homey;
};

struct fnabEnemy {
    s8 x;
    s8 y;
    f32 progress;
    struct Object * modelObj;
};

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
    MAPDIR_NO_PATH,
    MAPDIR_ARRIVED
};

void fnab_loop(void);
void fnab_init(void);

#endif