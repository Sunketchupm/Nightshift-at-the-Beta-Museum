#ifndef FNAB_H
#define FNAB_H

#include "types.h"

struct fnabEnemyProperties {
    u8 homex;
    u8 homey;
};

enum fnabEnemyState {
    FNABE_WANDER,
    FNABE_ATTACK,
    
    FNABE_PRIMED,
    FNABE_PRIMED_LEFT,
};

struct fnabEnemy {
    s8 x;
    s8 y;
    s8 tx;
    s8 ty;
    u8 state;
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

extern Vec3f fnabCameraPos;
extern Vec3f fnabCameraFoc;

enum officeState {
    OFFICE_STATE_DESK,
    OFFICE_STATE_HIDE,
    OFFICE_STATE_UNHIDE,
    OFFICE_STATE_LEAN_CAMERA,
    OFFICE_STATE_CAMERA,
    OFFICE_STATE_LEAVE_CAMERA
};

struct securityCameraInfo {
    u8 init;
    f32 x;
    f32 y;
    s16 angle;
    char * name;
};

enum officeAction {
    OACTION_CAMERA,
    OACTION_HIDE,
    OACTION_PANEL
};

void fnab_loop(void);
void fnab_init(void);
void fnab_render_2d(void);
void bhv_fnab_camera(void);

#endif