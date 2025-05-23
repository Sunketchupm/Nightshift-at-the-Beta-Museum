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
    FNABE_CART_ATTACK,
    
    FNABE_PRIMED,
    FNABE_PRIMED_LEFT,
    FNABE_PRIMED_RIGHT,
    FNABE_PRIMED_VENT,
    FNABE_TABLE_ATTACK,

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
    PERSONALITY_BULLY,
    PERSONALITY_WARIO,
    PERSONALITY_LUIGI,
    PERSONALITY_STANLEY,
};

enum MapTile {
    TILE_WALL = 0,
    TILE_FLOOR,
    TILE_VENT,
    TILE_WINDOW,
    TILE_ATTACK
};

enum TableAttackType {
    TABLE_ALWAYS_KILL = 0,
    TABLE_NEVER_KILL,
    TABLE_STANLEY
};

struct enemyInfo {
    u8 homeX;
    u8 homeY;
    u8 homeDir;
    u8 canVent;
    u8 maxSteps;
    const BehaviorScript* modelBhv;
    u16 modelId;
    f32 frequency;
    enum TableAttackType tableAttackType;

    u8 choice[3];
    u8 anim[4];

    f32 jumpscareScale;

    u8 personality;
};

struct FnabEnemy {
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
    u8 canVent;
    u16 stepCounter;
    u16 animFrameHold;
    f32 progress;
    f32 jumpscareYoffset;
    u8 tableAttackState;
    u8 forceJumpscare;
    u16 tableKillTimer;
    u8 tableAttackCount;
    struct Object * modelObj;
    struct enemyInfo * info;
};

struct pathBranch {
    u8 active;
    u8 x;
    u8 y;
};

enum MapDirection {
    MAPDIR_UP,
    MAPDIR_RIGHT,
    MAPDIR_DOWN,
    MAPDIR_LEFT,
    MAPDIR_NO_PATH,
    MAPDIR_ARRIVED
};

extern Vec3f fnabCameraPos;
extern Vec3f fnabCameraFoc;

extern u8 n64_mouse_enabled;
extern f32 n64_mouse_x;
extern f32 n64_mouse_y;

enum officeState {
    OFFICE_STATE_DESK,
    OFFICE_STATE_HIDE,
    OFFICE_STATE_UNHIDE,
    OFFICE_STATE_LEAN_CAMERA,
    OFFICE_STATE_CAMERA,
    OFFICE_STATE_LEAVE_CAMERA,
    OFFICE_STATE_BREAKER,
    OFFICE_STATE_TABLE_ATTACK,
    OFFICE_STATE_JUMPSCARED,
    OFFICE_STATE_WON
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

enum nightId {
    NIGHT_1,
    NIGHT_2,
    NIGHT_3,
    NIGHT_4,
    NIGHT_5,
    NIGHT_CUSTOM,
    NIGHT_ENDLESS
};

void fnab_loop(void);
void fnab_init(void);
void fnab_render_2d(void);
void bhv_fnab_camera(void);
void bhv_fnab_door(void);
void bhv_stanley_title(void);
void fnab_mouse_render(void);

s32 fnab_main_menu(void);
void fnab_main_menu_render(void);
void fnab_main_menu_init(void);
void bhv_background_blargg_loop(void);

#endif