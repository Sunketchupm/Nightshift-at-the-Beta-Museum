#ifndef FNAB_H
#define FNAB_H

#include "types.h"

struct fnabEnemyProperties {
    u8 homex;
    u8 homeY;
};

enum fnabEnemyState {
    FNABE_IDLE,
    FNABE_WANDER,
    FNABE_ATTACK,
    FNABE_DISTRACTED,
    FNABE_FLUSHED,
    
    FNABE_PRIMED,
    FNABE_PRIMED_LEFT,
    FNABE_PRIMED_RIGHT,
    FNABE_PRIMED_VENT,

    FNABE_JUMPSCARE
};

enum fnabEnemyId {
    ENEMY_MOTOS,
    ENEMY_BULLY,
    ENEMY_WARIO,
    ENEMY_LUIGI,
    ENEMY_STANLEY,
    ENEMY_COUNT
};

enum animSlot {
    ANIMSLOT_NORMAL,
    ANIMSLOT_VENT,
    ANIMSLOT_WINDOW,
    ANIMSLOT_JUMPSCARE
};

enum personality {
    PERSONALITY_DEFAULT,
    PERSONALITY_WARIO,
};

struct enemyInfo {
    u8 homeX;
    u8 homeY;
    u8 homeDir;
    u8 canVent;
    u8 maxSteps;
    BehaviorScript * modelBhv;
    u16 modelId;
    f32 frequency;
    f32 tableAttackChance;

    u8 choice[3];
    u8 anim[4];

    f32 jumpscareScale;

    u8 personality;
};

struct fnabEnemy {
    u8 active;
    s8 x;
    s8 y;
    s8 ventFlushX;
    s8 ventFlushY;
    s8 tx;
    s8 ty;
    u8 state;
    u8 attackLocation;
    u8 difficulty;//outta 20
    f32 progress;
    u16 animFrameHold;
    f32 jumpscareYoffset;
    struct Object * modelObj;
    struct enemyInfo * info;
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
    OFFICE_STATE_LEAVE_CAMERA,
    OFFICE_STATE_BREAKER,
    OFFICE_STATE_JUMPSCARED
};

enum sct {
    SC_TYPE_UNINIT,
    SC_TYPE_CAMERA,
    SC_TYPE_DOOR,
};

struct securityCameraInfo {
    u8 init;
    u8 type;
    u8 doorStatus;
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
void bhv_fnab_door(void);

#endif