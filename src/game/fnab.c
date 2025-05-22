#include "types.h"
#include "fnab.h"
#include "object_helpers.h"
#include "behavior_data.h"
#include "model_ids.h"
#include "object_list_processor.h"
#include "audio/external.h"
#include "engine/math_util.h"
#include "game_init.h"
#include "object_helpers.h"
#include "actors/group0.h"
#include "ingame_menu.h"
#include "engine/surface_collision.h"
#include "level_update.h"
#include "texscroll.h"
#include "seq_ids.h"
#include "audio/external.h"
#include "save_file.h"

extern void print_text_fmt_int(s32 x, s32 y, const char *str, s32 n);
extern void print_text(s32 x, s32 y, const char *str);

#define _ TILE_WALL, // Wall / Nothing
#define F TILE_FLOOR, // Floor  
#define V TILE_VENT, // Vent
#define W TILE_WINDOW, // Visible from Window
#define A TILE_ATTACK, // Attack zone

u8 fnabMap[20][20] = {
{F F F F _ F F F _ F F F _ _ _ _ _ _ _ _},
{F _ F F _ F F F _ F _ _ _ F _ F _ _ _ _},
{F F F F _ _ F F _ F F _ _ F F F F _ _ _},
{F _ F F V V F F F F F V V _ _ _ F _ _ _},
{F F F F _ _ F _ _ F _ _ F F F F F F F _},
{F _ F F _ _ F _ F F F _ F _ _ _ F _ F _},
{F F F F F F F F F _ F F F _ _ _ F _ F _},
{F _ _ _ _ F _ _ F F F _ F _ _ _ F _ F _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ F _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ F _},
{F F F _ _ V _ _ _ W W W W _ _ _ F _ F _},
{V _ _ _ _ V _ _ _ A _ _ _ A W W W _ F _},
{V V V V V V _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ V _ _ _ _ _ A _ _ _ _ _ _ _ _},
{_ _ _ _ _ V V V V V V V _ _ _ _ _ _ _ _},
{_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _},
};

#undef _
#undef W
#undef V
#undef W
#undef A

#define __ (0 << 0) // Used for walls for alignment
#define C0 (0 << 0) // Used for tiles that can't be seen, also for alignment
#define C1 (1 << 5) // Trampoline exhibit
#define C2 (1 << 6) // Hall L
#define C3 (1 << 7) // Lava exhibit
#define C4 (1 << 8) // L is real exhibit
#define C5 (1 << 9) // APPARITION HALL
#define C6 (1 << 10) // RESTROOMS
#define C7 (1 << 11) // SOUTH VENT 1
#define C8 (1 << 12) // HALL R
#define C9 (1 << 13) // SPACEWORLD CASTLE
// Using hexadecimal here to keep each camera just 2 letters in length
#define CA (1 << 14) // CARTRIDGE ROOM
#define CB (1 << 15) // CLOSET
#define CC (1 << 20) // NORTH VENT
#define CD (1 << 22) // SOUTH VENT 2

// Combinations of cameras
#define D1 (C9 | CA)
#define D2 (CB | CD)
#define D3 (C3 | CB)
#define D4 (CC | CB)
#define D5 (C4 | CC)
#define D6 (C2 | C8)
#define D7 (C1 | CB)
#define D8 (C1 | C4)
#define D9 (C1 | CC)

#define E1 (C1 | C4 | CC)
#define E2 (CC | C3 | CB)
#define E3 (C1 | C4 | CC)

u32 fnabCameraMap[20][20] = {
{C9, C9, C9, C9, __, C3, C3, C3, __, C4, C4, C4, __, __, __, __, __, __, __, __,},
{C9, __, C9, C9, __, C3, C3, D3, __, C4, __, __, __, C6, __, C6, __, __, __, __,},
{C9, C9, C9, C9, __, __, C3, E2, __, E3, D5, __, __, C6, C6, C6, C6, __, __, __,},
{C9, __, C9, C9, C9, CC, D4, D4, D9, E1, D5, D5, CC, __, __, __, C0, __, __, __,},
{C9, C9, C9, C9, __, __, CB, __, __, E3, __, __, D6, C2, C2, C2, C2, C5, C5, __,},
{C9, __, C9, C9, __, __, D7, __, C1, C1, C1, __, D6, __, __, __, C2, __, C5, __,},
{C9, C9, C9, C9, C9, D2, D7, C1, C1, __, C1, C1, C8, __, __, __, C2, __, C5, __,},
{C9, __, __, __, __, D2, __, __, C1, C1, C1, __, C8, __, __, __, C2, __, C5, __,},
{D1, CA, CA, __, CB, D2, CB, __, __, __, __, __, C8, __, __, __, C2, __, C5, __,},
{D1, CA, CA, __, CB, D2, CB, __, __, __, __, __, C8, __, __, __, C2, __, C5, __,},
{D1, CA, CA, __, __, CD, __, __, __, C0, C8, C8, C8, __, __, __, C2, __, C5, __,},
{C9, __, __, __, __, CD, __, __, __, C0, __, __, __, C0, C0, C0, C2, __, C5, __,},
{C9, C0, C0, C0, C0, CD, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, CD, __, __, __, __, __, C0, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, C0, C7, C7, C7, C7, C7, C7, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
{__, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __, __,},
};

#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
#undef C8
#undef C9
#undef CA
#undef CB
#undef CC
#undef CD
#undef D1
#undef D2
#undef D3
#undef D4
#undef D5
#undef D6
#undef D7
#undef D8
#undef D9
#undef E1
#undef E2
#undef E3

#define MAP_SIZE 20
#define PATH_BRANCH_STACK_CT 200

s8 dirOffset[6][2] = {
    {0,-1},
    {1,0},
    {0,1},
    {-1,0},
    {0,0}, //MAPDIR_NO_PATH (If no path found, stop moving)
    {0,0} //MAPDIR_ARRIVED already here
};

u8 pathfindingMap[MAP_SIZE][MAP_SIZE];
// Path branches prevent the need for a recursive function
struct pathBranch pathBranchStack[PATH_BRANCH_STACK_CT];
u8 pathBranchCount = 0;

enum MapTile get_map_data(s8 x, s8 y) {
    if ((x < MAP_SIZE && x >= 0)&&(y < MAP_SIZE && y >= 0)) {
        return pathfindingMap[y][x];
    }
    return TILE_WALL;
}

void add_path_branch(s8 x, s8 y, u8 allowVent) {
    enum MapTile mapdata = get_map_data(x,y);
    u8 cond = (mapdata > TILE_WALL);
    if (allowVent == FALSE) {
        cond = ((mapdata > TILE_WALL) && (mapdata != TILE_VENT));
    }

    if (cond) {
        u8 pathBranchIndex = 0;
        while (pathBranchStack[pathBranchIndex].active == 1) {
            pathBranchIndex++;
        }
        // Found non-active slot
        pathBranchStack[pathBranchIndex].active = 1;
        pathBranchStack[pathBranchIndex].x = x;
        pathBranchStack[pathBranchIndex].y = y;

        pathBranchCount++;
    }
}

u8 path_find(s8 xs, s8 ys, s8 xd, s8 yd, u8 allowVent) {
    if (xs == xd && ys == yd) {return MAPDIR_ARRIVED;}

    bcopy(&fnabMap, &pathfindingMap, MAP_SIZE * MAP_SIZE);
    bzero(&pathBranchStack, sizeof(pathfindingMap[0][0]) * PATH_BRANCH_STACK_CT);

    pathBranchCount = 0;
    add_path_branch(xs,ys,allowVent);

    // Flood fill from start to destination
    while(pathBranchCount > 0) {
        for (int i = 0; i < PATH_BRANCH_STACK_CT; i++) {
            struct pathBranch * cpb = &pathBranchStack[i];
            if (cpb->active) {
                s8 x = cpb->x; 
                s8 y = cpb->y;

                for (int d = 0; d < 4; d++) {
                    if (x+dirOffset[d][0] == xd && y+dirOffset[d][1] == yd) {return d;}
                    add_path_branch(x+dirOffset[d][0],y+dirOffset[d][1],allowVent);
                }
                
                // Remove current active path branch and mark as solid on pathfinding map
                cpb->active = 0;
                pathfindingMap[y][x] = 0;
                pathBranchCount--;
            }
        }
    }
    return MAPDIR_NO_PATH;
}

void render_camera_static(u8 alpha) {
    gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, alpha);
    create_dl_translation_matrix(MENU_MTX_PUSH, 160, 120, 0);
    gSPDisplayList(gDisplayListHead++, staticscreen_ss_mesh);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

Vec3f fnabCameraPos;
Vec3f fnabCameraFoc;
struct Object * officePovCamera = NULL;
struct Object * darknessObject = NULL;
struct Object * monitorScreenObject = NULL;

u8 fnab_cam_index = 0;
u8 fnab_cam_last_index = 5;
u8 fnab_cam_snap_or_lerp = 0;
u8 fnab_office_state = OFFICE_STATE_DESK;
u8 fnab_office_action = OACTION_HIDE;
int fnab_office_statetimer = 0;

u8 camera_interference_timer = 0;
u8 light_interference_timer = 0;
u8 force_static_timer = 0;
u8 force_static_opacity = 255;

f32 camera_mouse_x = 0.0f;
f32 camera_mouse_y = 0.0f;
u8 camera_mouse_selecting = FALSE;

u8 n64_mouse_enabled = FALSE;
u8 n64_mouse_selecting = FALSE;
u8 n64_mouse_camera_flick_state = 0;
f32 n64_mouse_x = 160.0f;
f32 n64_mouse_y = 120.0f;

#define IF_N64_MOUSE_ENABLED(is, isNot) (n64_mouse_enabled ? (is) : (isNot))

u8 snd_x = -1;
u8 snd_y = -1;
u8 snd_timer = 0;
u8 radar_timer = 0;
u8 vent_flush_timer = 0;

s8 breakerIndex = 0;
u8 breakerCharges[3] = {3,3,2};
u8 breakerChargesMax[3] = {3,3,2};
f32 breakerFixing = 0.0f;
u8 breakerDoFix = FALSE;
u8 cartridgeTilt = FALSE;

u16 fnab_clock = 0;
u8 fnab_night_id = 0;

u8 fnab_call_played = FALSE;

// Todo: Find some way to not need an external variable
f32 wario_timer = 0.0f;

struct enemyInfo motosInfo = {
    .homeX = 6,
    .homeY = 0,
    .homeDir = 0,
    .canVent = FALSE,
    .modelBhv = bhvMotos,
    .modelId = MODEL_MOTOS,
    .frequency = 0.02f,
    .tableAttackType = ENEMY_MOTOS,
    .maxSteps = 3,

    .choice = {FNABE_PRIMED_LEFT,FNABE_PRIMED_LEFT,FNABE_PRIMED_RIGHT},

    .anim[ANIMSLOT_NORMAL] = 6,
    .anim[ANIMSLOT_WINDOW] = 8,
    .anim[ANIMSLOT_VENT] = 0,
    .anim[ANIMSLOT_JUMPSCARE] = 6,

    .jumpscareScale = .6f,
    .personality = PERSONALITY_DEFAULT,
};

struct enemyInfo bullyInfo = {
    .homeX = 7,
    .homeY = 0,
    .homeDir = 0,
    .canVent = TRUE,
    .modelBhv = bhvBetaBully,
    .modelId = MODEL_BETABULLY,
    .frequency = 0.03f,
    .tableAttackType = ENEMY_BULLY,
    .maxSteps = 3,

    .choice = {FNABE_PRIMED_VENT,FNABE_PRIMED_VENT,FNABE_PRIMED_RIGHT},

    .anim[ANIMSLOT_NORMAL] = 0,
    .anim[ANIMSLOT_WINDOW] = 0,
    .anim[ANIMSLOT_VENT] = 0,
    .anim[ANIMSLOT_JUMPSCARE] = 3,

    .jumpscareScale = .55f,
    .personality = PERSONALITY_DEFAULT,
};

struct enemyInfo warioInfo = {
    .homeX = 18,
    .homeY = 11,
    .homeDir = MAPDIR_DOWN,
    .canVent = FALSE,
    .modelBhv = bhvWarioApp,
    .modelId = MODEL_WARIOAPP,
    .frequency = 0.1f,
    .tableAttackType = ENEMY_WARIO,
    .maxSteps = 4,

    .choice = {FNABE_PRIMED_RIGHT,FNABE_PRIMED_RIGHT,FNABE_PRIMED_RIGHT},

    .anim[ANIMSLOT_NORMAL] = 0,
    .anim[ANIMSLOT_WINDOW] = 0,
    .anim[ANIMSLOT_VENT] = 0,
    .anim[ANIMSLOT_JUMPSCARE] = 1,

    .jumpscareScale = .15f,
    .personality = PERSONALITY_WARIO,
};

struct enemyInfo luigiInfo = {
    .homeX = 11,
    .homeY = 0,
    .homeDir = 0,
    .canVent = TRUE,
    .modelBhv = bhvBetaLuigi,
    .modelId = MODEL_BETA_LUIGI,
    .frequency = 0.006f,
    .tableAttackType = ENEMY_LUIGI,
    .maxSteps = 1,

    .choice = {FNABE_PRIMED_LEFT,FNABE_PRIMED_RIGHT,FNABE_PRIMED_VENT},

    .anim[ANIMSLOT_NORMAL] = 0,
    .anim[ANIMSLOT_WINDOW] = 0,
    .anim[ANIMSLOT_VENT] = 1,
    .anim[ANIMSLOT_JUMPSCARE] = 2,

    .jumpscareScale = .6f,
    .personality = PERSONALITY_LUIGI,
};

struct enemyInfo stanleyInfo = {
    .homeX = 3,
    .homeY = 1,
    .homeDir = 1,
    .canVent = TRUE,
    .modelBhv = bhvStanley,
    .modelId = MODEL_STANLEY,
    .frequency = 0.09f,
    .tableAttackType = ENEMY_STANLEY,
    .maxSteps = 1,

    .choice = {FNABE_PRIMED_LEFT,FNABE_PRIMED_RIGHT,FNABE_PRIMED_VENT},

    .anim[ANIMSLOT_NORMAL] = 0,
    .anim[ANIMSLOT_WINDOW] = 0,
    .anim[ANIMSLOT_VENT] = 2,
    .anim[ANIMSLOT_JUMPSCARE] = 1,

    .jumpscareScale = .3f,
    .personality = PERSONALITY_STANLEY,
};

u8 nightEnemyDifficulty[7][ENEMY_COUNT] = {
    {5,4,0,0,0}, //NIGHT 1
    {6,6,10,0,0}, //NIGHT 2
    {8,8,0,7,0}, //NIGHT 3
    {11,11,11,11,0}, //NIGHT 4
    {0,0,0,0,12}, //NIGHT 5

    {0,0,0,0,0}, // CUSTOM NIGHT; will be written to
    {1,1,1,1,1}, // ENDLESS NIGHT, slowly increases overtime
};

s32 is_b3313_night(void) {
    if (nightEnemyDifficulty[NIGHT_CUSTOM][0] != 0x0B) {return FALSE;}
    if (nightEnemyDifficulty[NIGHT_CUSTOM][1] != 0x03) {return FALSE;}
    if (nightEnemyDifficulty[NIGHT_CUSTOM][2] != 0x03) {return FALSE;}
    if (nightEnemyDifficulty[NIGHT_CUSTOM][3] != 0x01) {return FALSE;}
    if (nightEnemyDifficulty[NIGHT_CUSTOM][4] != 0x03) {return FALSE;}
    return TRUE;
}

struct FnabEnemy enemyList[ENEMY_COUNT];

#define SECURITY_CAMERA_CT 25
struct securityCameraInfo securityCameras[SECURITY_CAMERA_CT] = {
    {.name = NULL}, // Office (No action)
    {.name = NULL}, // Office (Starting cameras)
    {.name = NULL}, // Office (Starting hiding)
    {.name = NULL}, // Office (Hiding)
    {.name = NULL}, // Office (Breaker)
    {.name = "TRAMPOLINE EXHIBIT"},
    {.name = "HALL L"},
    {.name = "LAVA EXHIBIT"},
    {.name = "L IS REAL EXHIBIT"},
    {.name = "APPARITION HALL"}, // Camera index 9
    {.name = "RESTROOMS"},
    {.name = "SOUTH VENT 1"},
    {.name = "HALL R"},
    {.name = "SPACEWORLD CASTLE"},
    {.name = "CARTRIDGE ROOM"},
    {.name = "CLOSET"},
    {.name = NULL}, // Door
    {.name = NULL}, // Door
    {.name = NULL}, // Door
    {.name = NULL}, // Door
    {.name = "NORTH VENT"},
    {.name = NULL}, // Door
    {.name = "SOUTH VENT 2"},
};

void bhv_fnab_door(void) {
    if (!securityCameras[o->oBehParams2ndByte].init) {
        securityCameras[o->oBehParams2ndByte].init = TRUE;
        securityCameras[o->oBehParams2ndByte].x = -o->oPosX/20.0f;
        securityCameras[o->oBehParams2ndByte].y = o->oPosZ/20.0f;
        securityCameras[o->oBehParams2ndByte].type = SC_TYPE_DOOR;
        securityCameras[o->oBehParams2ndByte].doorStatus = 0;
    } else {
        if (fnab_clock == 1) {
            securityCameras[o->oBehParams2ndByte].doorStatus = 0;
        }
    }

    //door routine
    switch(securityCameras[o->oBehParams2ndByte].doorStatus) {
        case 0:
            o->oTimer = 0;
            break;
        case 1:
            if (o->oTimer > 450) {
                play_sound(SOUND_GENERAL_STAR_DOOR_OPEN, gGlobalSoundSource);
                securityCameras[o->oBehParams2ndByte].doorStatus = 2;
            }
            break;
        case 2:
            if (o->oTimer > 1500) {
                securityCameras[o->oBehParams2ndByte].doorStatus = 0;
            }
            break;
    }

    u8 mapx = (u8)(o->oPosX/200.0f);
    u8 mapy = (u8)(o->oPosZ/-200.0f);

    //door behavior
    switch(securityCameras[o->oBehParams2ndByte].doorStatus) {
        case 0:
        case 2:
            fnabMap[mapy][mapx] = 1;//open
            if (GET_BPARAM1(o->oBehParams) == 1) {
                fnabMap[mapy][mapx] = 3;//visible from window door
            }

            o->oPosY += 12.0f;
            if (o->oPosY > 190.0f) {
                o->oPosY = 190.0f;
            }
            break;
        case 1:
            fnabMap[mapy][mapx] = 0;//close
    
            o->oPosY -= 15.0f;
            if (o->oPosY < 0.0f) {
                o->oPosY = 0.0f;
            }
            break;
    }
    bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);
}

void bhv_fnab_camera(void) {
    if (o->oBehParams2ndByte == 0) {
        officePovCamera = o;
    }

    if (!securityCameras[o->oBehParams2ndByte].init && securityCameras[o->oBehParams2ndByte].name) {
        securityCameras[o->oBehParams2ndByte].init = TRUE;
        securityCameras[o->oBehParams2ndByte].x = -o->oPosX/20.0f;
        securityCameras[o->oBehParams2ndByte].y = o->oPosZ/20.0f;
        securityCameras[o->oBehParams2ndByte].type = SC_TYPE_CAMERA;
    }
    securityCameras[o->oBehParams2ndByte].angle = o->oFaceAngleYaw;

    if (o->oBehParams2ndByte == fnab_cam_index) {

        //camera rotation controls
        if (securityCameras[o->oBehParams2ndByte].name) {
            if (gPlayer1Controller->buttonDown & L_CBUTTONS) {
                o->oFaceAngleYaw += 0x100;
            }
            if (gPlayer1Controller->buttonDown & R_CBUTTONS) {
                o->oFaceAngleYaw -= 0x100;
            }
        }

        if (o->oBehParams2ndByte == 0) {
            o->oFaceAngleYaw += -gPlayer1Controller->rawStickX*7;
            if (n64_mouse_enabled && n64_mouse_camera_flick_state != 2) {
                if (n64_mouse_x > 320.0f-80.0f) {
                    o->oFaceAngleYaw -= 0x250;
                }
                if (n64_mouse_x < 0.0f+80.0f) {
                    o->oFaceAngleYaw += 0x250;
                }
            }
        }

        if (o->oFaceAngleYaw > o->oMoveAngleYaw + 0x1300) {
            o->oFaceAngleYaw = o->oMoveAngleYaw + 0x1300;
        }
        if (o->oFaceAngleYaw < o->oMoveAngleYaw + -0x1300) {
            o->oFaceAngleYaw = o->oMoveAngleYaw + -0x1300;
        }

        if (fnab_cam_snap_or_lerp == 0) {
            vec3f_copy(fnabCameraPos,&o->oPosVec);
            fnabCameraFoc[0] = o->oPosX + sins(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
            fnabCameraFoc[1] = o->oPosY + sins(o->oFaceAnglePitch) * -5.0f;
            fnabCameraFoc[2] = o->oPosZ + coss(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
        } else {
            Vec3f fnabCameraPosTarget;
            Vec3f fnabCameraFocTarget;

            vec3f_copy(fnabCameraPosTarget,&o->oPosVec);
            fnabCameraFocTarget[0] = o->oPosX + sins(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
            fnabCameraFocTarget[1] = o->oPosY + sins(o->oFaceAnglePitch) * -5.0f;
            fnabCameraFocTarget[2] = o->oPosZ + coss(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
            
            f32 lerpspeed = .2f;
            if (fnab_office_state == OFFICE_STATE_JUMPSCARED) {
                lerpspeed = .3f;
            }

            for (int i = 0; i < 3; i++) {
                fnabCameraPos[i] = approach_f32_asymptotic(fnabCameraPos[i],fnabCameraPosTarget[i],lerpspeed);
                fnabCameraFoc[i] = approach_f32_asymptotic(fnabCameraFoc[i],fnabCameraFocTarget[i],lerpspeed);
            }
        }
    }
}

void bhv_stanley_title(void) {
    if (random_u16()%20>2 || (o->oTimer%90>60)) {
        o->header.gfx.animInfo.animFrame=6;
    }
}

u8 is_seen_on_camera(struct FnabEnemy* cfe) {
    return (fnabCameraMap[cfe->y][cfe->x] & (1 << fnab_cam_index)) != 0;
}

void fnab_enemy_set_target(struct FnabEnemy* cfe) {
    //set new target based on state
    switch(cfe->state) {
        case FNABE_WANDER:
            u8 random_x;
            u8 random_y;
            u8 emergency_break = 0;
            do {
                random_x = cfe->x + 3 - random_u16()%6;
                random_y = cfe->y + 3 -random_u16()%6;
                emergency_break++;
            } while (get_map_data(random_x,random_y) == TILE_WALL && emergency_break < 10);
            if (emergency_break < 10) {
                cfe->tx = random_x;
                cfe->ty = random_y;
            }
            break;
        case FNABE_ATTACK:
            u8 attack_location_index = random_u16() % (cfe->canVent ? 3 : 2);
            cfe->attackLocation = cfe->info->choice[attack_location_index];
            switch(cfe->attackLocation) {
                case FNABE_PRIMED_LEFT:
                    cfe->tx = 9;
                    cfe->ty = 11;
                    break;
                case FNABE_PRIMED_RIGHT:
                    cfe->tx = 13;
                    cfe->ty = 11;
                    break;
                case FNABE_PRIMED_VENT:
                    cfe->tx = 11;
                    cfe->ty = 13;
                    break;
            }
            break;
        case FNABE_CART_ATTACK:
            cfe->tx = 1;
            cfe->ty = 9;
            break;
    }
}

void fnab_enemy_successful_defense(struct FnabEnemy* cfe) {
    cfe->x = cfe->info->homeX;
    cfe->y = cfe->info->homeY;
    cfe->state = FNABE_WANDER;
    cfe->progress = 1.0f;
    cfe->stepCounter = 0;
    cfe->tableAttackState = 0;
    cfe->forceJumpscare = FALSE;
    cfe->tableKillTimer = 0;
    cfe->tableAttackCount++;
    cfe->modelObj->oFaceAnglePitch = 0;
    cfe->modelObj->oFaceAngleYaw = 0;
    cfe->modelObj->oFaceAngleRoll = 0;
    force_static_opacity = 255;
    force_static_timer = 10;
    light_interference_timer = 10;
    fnab_office_state = OFFICE_STATE_HIDE;
    obj_scale(cfe->modelObj, 1.0f);
}

u8 fnab_enemy_table_attack(struct FnabEnemy* cfe) {
    switch (cfe->info->tableAttackType) {
        case ENEMY_MOTOS: // The moto originally had a 10% chance to kill so here they will never kill
            fnab_enemy_successful_defense(cfe);
            return FALSE;
            break;
        case ENEMY_BULLY: // The bully originally had a 90% chance to kill so here they will always kill
            return TRUE;
            break;
        case ENEMY_WARIO: // Wario always killed
            return TRUE;
            break;
        case ENEMY_LUIGI: // ?
            return TRUE;
            break;
        case ENEMY_STANLEY:
            //print_text(n64_mouse_x, n64_mouse_y, "0");
            //print_text_fmt_int(10, 20, "%d", n64_mouse_x);
            //print_text_fmt_int(10, 0, "%d", n64_mouse_y);
            force_static_timer = 1;
            force_static_opacity = 20;
            light_interference_timer = 1;
            obj_scale(cfe->modelObj, 0.25f);
            cfe->animFrameHold = random_u16() % 8;
            if (cfe->stepCounter == 0) {
                cfe->stepCounter = MAX(((u16)(60 - (2.5f * cfe->difficulty)) +
                                        (random_u16() % (30 - cfe->difficulty)))
                                        - (3 * cfe->tableAttackCount), 5);
                u8 oldState = cfe->tableAttackState;
                do {
                    cfe->tableAttackState = random_u16() % 3;
                } while (cfe->tableAttackState == oldState);
                // Move stanley to the position
                obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_NORMAL]);
                #define TABLE_POS_STATE(id, x, y, z, pitch, yaw, roll) \
                    case id: \
                        cfe->modelObj->oPosX = x; \
                        cfe->modelObj->oPosY = y; \
                        cfe->modelObj->oPosZ = z; \
                        cfe->modelObj->oFaceAnglePitch = pitch; \
                        cfe->modelObj->oFaceAngleYaw = yaw; \
                        cfe->modelObj->oFaceAngleRoll = roll; \
                        break;
                switch (cfe->tableAttackState) {
                    TABLE_POS_STATE(0, 2314, 95, -2450, 32767, 32767, 0);
                    TABLE_POS_STATE(1, 2396, 15, -2450, 0, 0, 18000);
                    TABLE_POS_STATE(2, 2230, 62, -2450, 0, 0, -21746);
                }
                #undef TABLE_POS_STATE
            }

            #define TABLE_ATTACK_STATE(id, minX, maxX, minY, maxY, direction, anti_direction) \
                case id: \
                    if ((!n64_mouse_enabled && (gPlayer1Controller->buttonDown & direction) && !(gPlayer1Controller->buttonDown & anti_direction)) \
                        || (n64_mouse_x > minX && n64_mouse_x < maxX && n64_mouse_y > minY && n64_mouse_y < maxY)) { \
                        cfe->stepCounter--; \
                        if (cfe->tableKillTimer > 0) {\
                            cfe->tableKillTimer--; \
                        } \
                    } else { \
                        if (cfe->tableKillTimer < 255) { \
                            cfe->tableKillTimer += 2; \
                        } \
                    } \
                    /* print_text_fmt_int((minX + maxX) * 0.5, (minY + maxY) * 0.5, "%d", cfe->tableAttackState); */ \
                    break;
            switch (cfe->tableAttackState) {
                TABLE_ATTACK_STATE(0, 120, 190, 125, 195, U_CBUTTONS, (R_CBUTTONS | L_CBUTTONS));
                TABLE_ATTACK_STATE(1, 225, 290, 20, 130, R_CBUTTONS, (U_CBUTTONS | L_CBUTTONS));
                TABLE_ATTACK_STATE(2, 30, 100, 105, 175, L_CBUTTONS, (U_CBUTTONS | R_CBUTTONS));
            }
            //print_text_fmt_int(60, 30, "%d", cfe->tableKillTimer);
            #undef TABLE_ATTACK_STATE

            if (cfe->progress >= 8 + (0.2f * cfe->difficulty)) {
                fnab_enemy_successful_defense(cfe);
            } else if (cfe->tableKillTimer >= (150 - (2 * cfe->difficulty) - (5 * cfe->tableAttackCount))) {
                //print_text(60, 50, "Failed");
                return TRUE;
            }
            break;
        default:
            return FALSE;
    }
    return FALSE;
}

void fnab_enemy_step(struct FnabEnemy* cfe) {
    if (!cfe->active) {return;}

    //JUMPSCARE BEHAVIOR
    if (cfe->state == FNABE_JUMPSCARE) {
        if (cfe->info->personality == PERSONALITY_STANLEY) {
            if (fnab_office_statetimer < 2) {
                cfe->modelObj->header.gfx.animInfo.animFrame = 1;
            }
            cfe->modelObj->oPosX = approach_f32_asymptotic(cfe->modelObj->oPosX, officePovCamera->oPosX + sins(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f,.3f);
            cfe->modelObj->oPosY = approach_f32_asymptotic(cfe->modelObj->oPosY, officePovCamera->oPosY + -45.0f + cfe->jumpscareYoffset,.3f);
            cfe->modelObj->oPosZ = approach_f32_asymptotic(cfe->modelObj->oPosZ, officePovCamera->oPosZ + coss(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f,.3f);
        } else {
            cfe->modelObj->oPosX = officePovCamera->oPosX + sins(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f;
            cfe->modelObj->oPosY = officePovCamera->oPosY + -45.0f + cfe->jumpscareYoffset; // + sins(officePovCamera->oFaceAnglePitch) * -5.0f;
            cfe->modelObj->oPosZ = officePovCamera->oPosZ + coss(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f;
        }
        //play_sound(SOUND_OBJ2_SMALL_BULLY_ATTACKED, gGlobalSoundSource);

        cfe->modelObj->oFaceAnglePitch = 0;
        cfe->modelObj->oFaceAngleYaw = obj_angle_to_object(cfe->modelObj,officePovCamera);
        cfe->modelObj->oFaceAngleRoll = 0;
        obj_scale(cfe->modelObj,cfe->info->jumpscareScale);
        cfe->modelObj->header.gfx.animInfo.animFrame+=2;
        cfe->jumpscareYoffset *= .7f;
        return;
    }

    //WARIO PERSONALITY
    if (cfe->info->personality == PERSONALITY_WARIO) {
        if (cfe->state == FNABE_IDLE) {
            if (fnab_cam_index == 9 && camera_interference_timer == 0 && wario_timer > 30) {
                wario_timer -= 3;
            }
            if ((gGlobalTimer % 30 == 0) && wario_timer > 200) {
                play_sound(SOUND_PEACH_THANK_YOU_MARIO, gGlobalSoundSource);
            }
            if (gGlobalTimer % 4 == 0) {
                wario_timer += 1.0f; //+ ((f32)cfe->difficulty / 20.0f);
                if (wario_timer >= 255) {
                    wario_timer = 255;
                    cfe->state = FNABE_ATTACK;
                    fnab_enemy_set_target(cfe);
                    play_sound(SOUND_PEACH_DEAR_MARIO, gGlobalSoundSource);
                }
            }
            cfe->modelObj->oOpacity = (s32)wario_timer;
        }
    }
    
    //DEFAULT PERSONALITY
    if (cfe->info->personality != PERSONALITY_WARIO) {
        if (cfe->state == FNABE_IDLE) {
            if (fnab_night_id == 0 && fnab_clock < 1800*2) {
                //do nothing
            } else {
                cfe->state = FNABE_WANDER;
                fnab_enemy_set_target(cfe);
            }
        }
    }

    //PER FRAME PROGRESS
    if (cfe->info->personality == PERSONALITY_LUIGI && is_seen_on_camera(cfe)) {
        // don't move
    } else {
        cfe->progress += cfe->info->frequency * .7f; //make game slower by 70% og so the player has more time to actually strategize
    }

    if (cfe->state == FNABE_FLUSHED) {
        cfe->progress += cfe->info->frequency*5.0f;
    }

    u8 modeldir = cfe->info->homeDir;
    if (cfe->state < FNABE_PRIMED) {
        // Wandering around map
        if (cfe->progress >= 1.0f) {
            cfe->progress -= 1.0f;

            enum MapTile tile_landed = 0;
            enum MapTile start_tile = 0;
            u16 steps = cfe->info->maxSteps; //1+(random_u16()%cfe->info->maxSteps);
            if (cfe->state == FNABE_IDLE || (random_u16()%20)+1>cfe->difficulty) {
                steps = 0;
            }
            if (steps > 0) {
                if (is_seen_on_camera(cfe)) {
                    camera_interference_timer = 6;
                }
                if (get_map_data(cfe->x,cfe->y) == TILE_ATTACK) {
                    steps = 1;
                    light_interference_timer = 10;
                }
            }

            for (int i = 0; i < steps; i++) {
                cfe->animFrameHold = random_u16();

                enum MapDirection dir = path_find(cfe->tx,cfe->ty,cfe->x,cfe->y,cfe->canVent);
                modeldir = dir;

                cfe->x -= dirOffset[dir][0];
                cfe->y -= dirOffset[dir][1];

                start_tile = get_map_data(cfe->x,cfe->y);

                //if touching another enemy on last turn, go to old position
                //stanley can phase through enemies now (otherwise he gets fucking stuck LMAO)
                if (i == steps-1 && cfe->info->personality != PERSONALITY_STANLEY) { //only on last step
                    for (int i = 0; i < ENEMY_COUNT; i++) {
                        if (enemyList[i].active) {
                            if (cfe != &enemyList[i] && enemyList[i].x == cfe->x && enemyList[i].y == cfe->y) {
                                cfe->x += dirOffset[dir][0];
                                cfe->y += dirOffset[dir][1];
                            }
                        }
                    }
                }

                tile_landed = get_map_data(cfe->x,cfe->y);

                if (tile_landed == TILE_FLOOR) {
                    cfe->ventFlushX = cfe->x;
                    cfe->ventFlushY = cfe->y;
                }

                //what to do when at destination
                if (dir >= MAPDIR_NO_PATH) {
                    switch(cfe->state) {
                        case FNABE_WANDER:
                            if (TRUE || random_u16()%3==0) {
                                //1/3 chance to start attacking
                                cfe->state = FNABE_ATTACK;
                            }
                            if (cfe->info->personality == PERSONALITY_LUIGI && !cartridgeTilt) {
                                cfe->state = FNABE_CART_ATTACK;
                            }
                            break;
                        case FNABE_DISTRACTED:
                        case FNABE_FLUSHED:
                            cfe->progress = 0.0f;
                            cfe->state = FNABE_WANDER;
                            break;
                        case FNABE_ATTACK:
                            //weird, but needed
                            cfe->state = FNABE_WANDER;
                            break;
                        case FNABE_CART_ATTACK:
                            //cfe->state = FNABE_WANDER;
                            if (dir == MAPDIR_ARRIVED) {
                                cartridgeTilt = TRUE;
                                breakerFixing = FALSE;
                                for (int i = 0; i<3; i++) {
                                    breakerCharges[i] = 0;
                                }
                                if (cartridgeTilt) {
                                    cfe->state = FNABE_ATTACK;
                                }
                            }
                            break;
                    }

                    fnab_enemy_set_target(cfe);
                }

                if (tile_landed == TILE_ATTACK && cfe->x == cfe->tx && cfe->state == FNABE_ATTACK) {
                    cfe->state = cfe->attackLocation;
                    cfe->progress = 0.0f;
                }
            }

            // Also static on tile arrival
            if (is_seen_on_camera(cfe) && cfe->info->personality != PERSONALITY_WARIO) {
                camera_interference_timer = 6;
            }

            bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);
            //UPDATE MODEL
            f32 zoff = 0.0f;
            if (cfe->state == FNABE_PRIMED_LEFT || cfe->state == FNABE_PRIMED_RIGHT) {
                zoff = 70.0f;
            }

            if (steps > 0) {
                if (cfe->info->personality == PERSONALITY_WARIO) {
                    play_sound(SOUND_OBJ_MAD_PIANO_CHOMPING, gGlobalSoundSource);
                }
                cfe->modelObj->oFaceAngleYaw = (modeldir*0x4000) + 0x8000;
            }
            cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f;
            cfe->modelObj->oPosZ = (-200.0f*cfe->y)-100.0f+zoff;
            cfe->modelObj->oPosY = find_floor_height(cfe->modelObj->oPosX,500.0f,cfe->modelObj->oPosZ);
            if (tile_landed >= TILE_WINDOW) {
                cfe->modelObj->oFaceAngleYaw = obj_angle_to_object(cfe->modelObj,officePovCamera); //stare at the player
                light_interference_timer = 10;
            }
            //set tile animation
            switch(tile_landed) {
                case TILE_FLOOR:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_NORMAL]);
                    break;
                case TILE_VENT:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_VENT]);
                    break;
                case TILE_WINDOW:
                case TILE_ATTACK:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_WINDOW]);
                    if (cfe->attackLocation == FNABE_PRIMED_VENT) {
                        obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_VENT]);
                    }
                    break;
                default:
                    break;
            }
            //vent queue
            if (start_tile == TILE_FLOOR && tile_landed == TILE_VENT) {
                play_sound(SOUND_ACTION_METAL_STEP, gGlobalSoundSource);
            }

            if (++cfe->stepCounter > 5 && (cfe->info->canVent && !cfe->canVent)) {
                cfe->canVent = TRUE;
            }
        }
    } else {
        // About to attack
        if (cfe->state != FNABE_TABLE_ATTACK && fnab_office_state == OFFICE_STATE_HIDE && fnab_cam_index == 3) {
            cfe->state = FNABE_TABLE_ATTACK;
            cfe->tableAttackState = 0;
            cfe->stepCounter = 0;
            cfe->progress = 0;
            cfe->tableKillTimer = 0;
            fnab_office_state = OFFICE_STATE_TABLE_ATTACK;
        } else if (cfe->state == FNABE_TABLE_ATTACK && !cfe->forceJumpscare) {
            cfe->forceJumpscare = fnab_enemy_table_attack(cfe);
        } else if (cfe->progress > 5.0f || cfe->forceJumpscare) { //waittime til kill
            u8 canjumpscare = TRUE;
            switch(cfe->attackLocation) {
                case FNABE_PRIMED_LEFT:
                    if (officePovCamera->oFaceAngleYaw < 0) {
                        canjumpscare = FALSE;
                    }
                break;
                case FNABE_PRIMED_RIGHT:
                    if (officePovCamera->oFaceAngleYaw >= 0) {
                        canjumpscare = FALSE;
                    }
                break;
            }

            //camera = unconditional jumpscare
            if (fnab_office_state == OFFICE_STATE_CAMERA) {
                canjumpscare = TRUE;
            }

            if (cfe->info->personality == PERSONALITY_STANLEY) {
                canjumpscare = TRUE;
                //stanley can bypass being stared at
            }

            //if player is under desk
            if (fnab_office_state == OFFICE_STATE_HIDE || fnab_office_state == OFFICE_STATE_UNHIDE) {
                if (cfe->attackLocation == FNABE_PRIMED_VENT) {
                    canjumpscare = FALSE; //cant jumpscare if being stared at

                    if (cfe->info->personality == PERSONALITY_STANLEY) {
                        canjumpscare = TRUE;
                        //stanley can bypass being stared at
                    }
                }
            }
            if (fnab_office_state == OFFICE_STATE_JUMPSCARED || fnab_office_state == OFFICE_STATE_WON) {
                //prevent double jumpscaring, or interrputing a well deserved win
                canjumpscare = FALSE;
                cfe->forceJumpscare = FALSE;
            }

            if (canjumpscare || cfe->forceJumpscare) {
                if (fnab_office_state == OFFICE_STATE_CAMERA) {
                    fnab_cam_snap_or_lerp = 0;
                    fnab_cam_index = 1;
                }
                fnab_office_state = OFFICE_STATE_JUMPSCARED;
                fnab_office_statetimer = 0;
                cfe->state = FNABE_JUMPSCARE;

                cfe->jumpscareYoffset = -75.f;
                obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_JUMPSCARE]);
                cfe->modelObj->header.gfx.animInfo.animFrame = 1;

                if (cfe->info->personality == PERSONALITY_WARIO) {
                    play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_JUMPSCARE2), 0);
                } else {
                    play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_JUMPSCARE1), 0);
                }
            }
        }
    }

    if (cfe->info->personality != PERSONALITY_WARIO) {
        cfe->modelObj->header.gfx.animInfo.animFrame = cfe->animFrameHold%cfe->modelObj->header.gfx.animInfo.curAnim->loopEnd;
    }
}

void fnab_enemy_init(struct FnabEnemy * cfe, struct enemyInfo * info, u8 difficulty) {
    if (difficulty == 0) {
        cfe->active = FALSE;
        return;
    }
    cfe->active = TRUE;
    cfe->progress = 1.0f;
    cfe->x = info->homeX;
    cfe->y = info->homeY;
    cfe->tx = info->homeX;
    cfe->ty = info->homeY;
    cfe->stepCounter = 0;
    cfe->tableKillTimer = 0;
    cfe->tableAttackState = 0;
    cfe->forceJumpscare = FALSE;
    cfe->tableAttackCount = 0;

    if (is_b3313_night()) {
        info = &stanleyInfo;
        //hijack info struct to make em like stanley
    }

    cfe->state = FNABE_IDLE;
    cfe->difficulty = difficulty;
    cfe->modelObj = spawn_object(gMarioObject,info->modelId,info->modelBhv);
    cfe->modelObj->oFaceAngleYaw = (info->homeDir*0x4000) + 0x8000;
    cfe->info = info;
    cfe->animFrameHold = random_u16();
    cfe->canVent = cfe->info->canVent;

    wario_timer = 0.0f;
}

void print_breaker_status(u16 x, u16 y) {
    print_text_fmt_int(x, y, "DOORS", 0);
    print_text_fmt_int(x, y-20, "AUDIO", 0);
    print_text_fmt_int(x, y-40, "UTILITY", 0);
    if (fnab_office_state == OFFICE_STATE_BREAKER) {
        print_text_fmt_int(x, y-60, "REBOOT ALL", 0);
    }

    for (int i = 0; i<3; i++) {
        for (int j = 0; j<breakerCharges[i]; j++) {
            print_text_fmt_int(x+90+j*4, y-i*20, "I", 0);
        }
    }
}

char * clockstrings[] = {
    "12AM",
    "1 AM",
    "2 AM",
    "3 AM",
    "4 AM",
    "5 AM",
    "6 AM"
};

s32 mouse_click_button(f32 x, f32 y, f32 width) {
    if (!n64_mouse_enabled) {return 0;}

    if (n64_mouse_x < x) {return 0;}
    if (n64_mouse_y > y + 16.f) {return 0;}
    if (n64_mouse_x > x + width) {return 0;}
    if (n64_mouse_y < y) {return 0;}

    n64_mouse_selecting = TRUE;
    if (gPlayer2Controller->buttonPressed & A_BUTTON) {
        return 2; //clicked
    }
    return 1; //hover
}

s32 mouse_click_button_vertical(f32 x, f32 y, f32 height) {
    if (!n64_mouse_enabled) {return 0;}

    if (n64_mouse_x < x) {return 0;}
    if (n64_mouse_y > y + height) {return 0;}
    if (n64_mouse_x > x + 16.f) {return 0;}
    if (n64_mouse_y < y) {return 0;}

    n64_mouse_selecting = TRUE;
    if (gPlayer2Controller->buttonDown & A_BUTTON) {
        return 2; //clicked
    }
    return 1; //hover
}

void fnab_mouse_loop(void) {
    if (n64_mouse_selecting && (gPlayer2Controller->buttonPressed & A_BUTTON)) {
        gPlayer1Controller->buttonPressed |= A_BUTTON;
    }

    n64_mouse_selecting = FALSE;

    n64_mouse_x += gPlayer2Controller->rawStickX;
    n64_mouse_y += gPlayer2Controller->rawStickY;

    n64_mouse_x = CLAMP(n64_mouse_x,10.0f,310.0f);
    n64_mouse_y = CLAMP(n64_mouse_y,10.0f,230.0f);
}

void fnab_camera_flick_handler(f32 x) {
    if (n64_mouse_camera_flick_state == 1) {
        n64_mouse_camera_flick_state = 2;
    }
    if (mouse_click_button_vertical(x,145.0f,80.0f)>0 && n64_mouse_camera_flick_state == 0) {
        n64_mouse_camera_flick_state = 1;
    }
    if (mouse_click_button_vertical(x,145.0f,80.0f)==0 && n64_mouse_camera_flick_state == 2) {
        n64_mouse_camera_flick_state = 0;
    }
}

void fnab_mouse_render(void) {
    if (n64_mouse_enabled) {
        gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
        create_dl_translation_matrix(MENU_MTX_PUSH, n64_mouse_x, n64_mouse_y, 0);
        if (!n64_mouse_selecting) {
            gSPDisplayList(gDisplayListHead++, mouse1_mouse1_mesh);
        } else {
            gSPDisplayList(gDisplayListHead++, mouse2_mouse2_mesh);
        }
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
        gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    }
}

s32 exit_button(void) {
    return (mouse_click_button(270.0f,5.0f,40.0f)==2);
}
s32 exit_button_hover(void) {
    return (mouse_click_button(270.0f,5.0f,40.0f));
}

void exit_render(void) {
    if (n64_mouse_enabled) {
        print_text_fmt_int(270, 5, "EXIT", 0);
    }
}

void fnab_render_2d(void) {
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);

    //WIN RENDER
    if (fnab_office_state == OFFICE_STATE_WON) {
        render_camera_static(255);
        print_text_fmt_int(100, 110, "NIGHT COMPLETE", 0);

        return;
    }

    //GAME OVER RENDER
    if (fnab_office_state == OFFICE_STATE_JUMPSCARED && fnab_office_statetimer > 30) {
        render_camera_static(255);
        if (fnab_office_statetimer > 60) {
            print_text_fmt_int(110, 110, "GAME OVER", 0);
        }

        return;
    }

    if (fnab_office_state == OFFICE_STATE_CAMERA) {
        u8 static_alpha = 10;
        if (camera_interference_timer > 0 || cartridgeTilt == TRUE) {
            static_alpha = 255;
        }
        render_camera_static(static_alpha);

        create_dl_translation_matrix(MENU_MTX_PUSH, 200, 175, 0);
        gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
        gSPDisplayList(gDisplayListHead++, cammap_cammap_mesh);

        //render camera buttons
        for (int i = 0; i < SECURITY_CAMERA_CT; i++) {
            if (securityCameras[i].init == FALSE) {continue;}

            if (securityCameras[i].type == SC_TYPE_CAMERA) {
                gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
                if (i == fnab_cam_index) {gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);}

                create_dl_translation_matrix(MENU_MTX_PUSH, securityCameras[i].x, securityCameras[i].y, 0);
                    create_dl_rotation_matrix(MENU_MTX_PUSH, (securityCameras[i].angle/65535.f)*360.f+180.0f , 0, 0, 1.0f);
                        gSPDisplayList(gDisplayListHead++, cb_cb_mesh);
                    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
                gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
            } else {
                switch(securityCameras[i].doorStatus) {
                    case 0:
                        gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
                        break;
                    case 1:
                        gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
                        break;
                    case 2:
                        gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
                        break;
                }
                create_dl_translation_matrix(MENU_MTX_PUSH, securityCameras[i].x, securityCameras[i].y, 0);
                    gSPDisplayList(gDisplayListHead++, dr_dr_mesh);
                gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
            }
        }

        create_dl_translation_matrix(MENU_MTX_PUSH, camera_mouse_x, camera_mouse_y, 0);
            if (!camera_mouse_selecting) {
                gSPDisplayList(gDisplayListHead++, mouse1_mouse1_mesh);
            } else {
                gSPDisplayList(gDisplayListHead++, mouse2_mouse2_mesh);
            }
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

        if (snd_timer > 0) {
            create_dl_translation_matrix(MENU_MTX_PUSH, -10+(snd_x*-10), snd_y*-10, 0);
            gSPDisplayList(gDisplayListHead++, snd_snd_mesh);
            gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
        }

        if (radar_timer > 100) {
            for (int i = 0; i < ENEMY_COUNT; i++) {
                if (enemyList[i].active) {
                    create_dl_translation_matrix(MENU_MTX_PUSH, -10+(enemyList[i].x*-10), enemyList[i].y*-10, 0);
                    gSPDisplayList(gDisplayListHead++, radar_radar_mesh);
                    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
                }
            }
        }

        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    }

    if (fnab_night_id != NIGHT_ENDLESS) {
        u8 clocktime = fnab_clock/1800;
        print_text_fmt_int(10, 230, clockstrings[clocktime], 0);
    } else {
        print_text_fmt_int(10, 230, "SCORE-%d", fnab_clock/30);
    }
    if (fnab_office_state == OFFICE_STATE_DESK) {
        switch(fnab_office_action) {
            case OACTION_CAMERA:
                if (n64_mouse_enabled) {
                    create_dl_translation_matrix(MENU_MTX_PUSH, 0.0f, 145.0f, 0);
                    gSPDisplayList(gDisplayListHead++, cambtn_cambtn_mesh);
                    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
                } else {
                    print_text_fmt_int(10, 10, "CAMERA", 0);
                }
                break;
            case OACTION_HIDE:
                //print_text_fmt_int(150, 10, "HIDE", 0);
                break;
            case OACTION_PANEL:
                if (n64_mouse_enabled) {
                    create_dl_translation_matrix(MENU_MTX_PUSH, 292.0f, 145.0f, 0);
                    gSPDisplayList(gDisplayListHead++, cambtn2_cambtn2_mesh);
                    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
                } else {
                    print_text_fmt_int(220, 10, "BREAKER", 0);
                }
                break;
        }
    }

    if (fnab_office_state == OFFICE_STATE_CAMERA) {
        if (n64_mouse_enabled) {
            create_dl_translation_matrix(MENU_MTX_PUSH, 0.0f, 145.0f, 0);
            gSPDisplayList(gDisplayListHead++, cambtn_cambtn_mesh);
            gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
        }

        print_breaker_status(190,220);
        print_text_fmt_int(10, 5, securityCameras[fnab_cam_index].name, 0);

        gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);

        char * play_sound_string = "Z - Play Sound";
        char * radar_string = "R - Radar";
        char * vent_string = "L - Flush Vents";
        if (n64_mouse_enabled) {
            play_sound_string = "RMB - Play Sound";
            radar_string = "Radar";
            vent_string = "Flush Vents";
        }

        //shadow
        gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 255);
        print_generic_string_ascii(230,30-1,play_sound_string);
        print_generic_string_ascii(230,50-1,radar_string);
        print_generic_string_ascii(230,70-1,vent_string);

        if (get_map_data(-camera_mouse_x/10.f,-camera_mouse_y/10.f)) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,30,play_sound_string);

        if (radar_timer == 0) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,50,radar_string);

        if (vent_flush_timer == 0) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,70,vent_string);

        gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    }
    if (fnab_office_state == OFFICE_STATE_BREAKER && fnab_office_statetimer > 7) {
        print_text_fmt_int(90, 150-breakerIndex*20, "^", 0);
        print_breaker_status(110,150);

        if (n64_mouse_enabled) {
            create_dl_translation_matrix(MENU_MTX_PUSH, 292.0f, 145.0f, 0);
            gSPDisplayList(gDisplayListHead++, cambtn2_cambtn2_mesh);
            gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
        }

        if (breakerDoFix) {
            for (int i = 0; i<7;i++) {
                char * printChar = ".";
                if (breakerFixing >= i) {
                    printChar = "^";
                }
                print_text_fmt_int(90+i*20, 70, printChar, 0);
            }
        }
    }

    if (force_static_timer > 0) {
        render_camera_static(force_static_opacity);
        force_static_timer--;
    }

    //fnab_mouse_render();
}

void fnab_init(void) {
    officePovCamera = NULL;
    
    fnab_cam_index = 0;
    fnab_cam_last_index = 5;
    fnab_cam_snap_or_lerp = 0;
    fnab_office_state = OFFICE_STATE_DESK;
    fnab_office_action = OACTION_HIDE;
    fnab_office_statetimer = 0;
    
    camera_interference_timer = 0;
    light_interference_timer = 0;
    
    snd_timer = 0;
    radar_timer = 0;
    vent_flush_timer = 0;
    
    breakerIndex = 0;
    breakerDoFix = FALSE;
    cartridgeTilt = FALSE;
    
    fnab_clock = 0;

    camera_mouse_x = -50.0f;
    camera_mouse_y = -50.0f;

    fnab_call_played = FALSE;

    if (fnab_night_id <= NIGHT_5) {
        breakerChargesMax[0] = 3;
        breakerChargesMax[1] = 3;
        breakerChargesMax[2] = 2;
    } else {
        breakerChargesMax[0] = 4;
        breakerChargesMax[1] = 4;
        breakerChargesMax[2] = 4;
    }

    for (int i = 0; i < 3; i++) {
        breakerCharges[i] = breakerChargesMax[i];
    }

    darknessObject = spawn_object(gMarioObject,MODEL_DARKNESS,bhvStaticObject);
    darknessObject->oPosX = 0.0f;
    darknessObject->oPosY = 0.0f;
    darknessObject->oPosZ = 0.0f;

    monitorScreenObject = spawn_object(gMarioObject,MODEL_MOFF,bhvStaticObject);
    monitorScreenObject->oPosX = 0.0f;
    monitorScreenObject->oPosY = 0.0f;
    monitorScreenObject->oPosZ = 0.0f;

    fnab_enemy_init(&enemyList[ENEMY_MOTOS],&motosInfo, nightEnemyDifficulty[fnab_night_id][ENEMY_MOTOS]);
    fnab_enemy_init(&enemyList[ENEMY_BULLY],&bullyInfo, nightEnemyDifficulty[fnab_night_id][ENEMY_BULLY]);
    fnab_enemy_init(&enemyList[ENEMY_WARIO],&warioInfo, nightEnemyDifficulty[fnab_night_id][ENEMY_WARIO]);
    fnab_enemy_init(&enemyList[ENEMY_LUIGI],&luigiInfo, nightEnemyDifficulty[fnab_night_id][ENEMY_LUIGI]);
    fnab_enemy_init(&enemyList[ENEMY_STANLEY],&stanleyInfo, nightEnemyDifficulty[fnab_night_id][ENEMY_STANLEY]);
}

#define OACTHRESH 0x600

int old_custom_night_highscore = 0;
void fnab_loop(void) {
    if (officePovCamera == NULL) {return;}

    fnab_mouse_loop();

    for (int i = 0; i<ENEMY_COUNT; i++) {
        fnab_enemy_step(&enemyList[i]);
    }

    if (!fnab_call_played) {
        fnab_call_played = TRUE;
        switch(fnab_night_id) {
            case 0:
                play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_NIGHT1CALL), 0);
                break;
            case 1:
                play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_NIGHT2CALL), 0);
                break;
            case 2:
                play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_NIGHT3CALL), 0);
                break;
            case 4:
                play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_NIGHT5CALL), 0);
                break;
        }
    }
    //play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_TITLE), 0);

    monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MOFF];
    switch(fnab_office_state) {
        case OFFICE_STATE_DESK:
            fnab_cam_snap_or_lerp = 1;
            if (officePovCamera->oFaceAngleYaw < -OACTHRESH) {
                fnab_office_action = OACTION_PANEL;
                fnab_camera_flick_handler(300.0f);
            }
            if (officePovCamera->oFaceAngleYaw >= -OACTHRESH && officePovCamera->oFaceAngleYaw <= OACTHRESH) {
                fnab_office_action = OACTION_HIDE;
            }
            if (officePovCamera->oFaceAngleYaw > OACTHRESH) {
                fnab_office_action = OACTION_CAMERA;
                fnab_camera_flick_handler(0.0f);
            }

            if ((gPlayer1Controller->buttonDown & Z_TRIG) || (n64_mouse_y < 30.0f)) {
                fnab_cam_index = 2;
                fnab_office_statetimer = 0;
                fnab_office_state = OFFICE_STATE_HIDE;
            }

            if ((gPlayer1Controller->buttonPressed & A_BUTTON) || n64_mouse_camera_flick_state==1) {
                switch(fnab_office_action) {
                    case OACTION_CAMERA:
                        fnab_cam_index = 1;
                        fnab_office_statetimer = 0;
                        fnab_office_state = OFFICE_STATE_LEAN_CAMERA;
                        break;
                    case OACTION_PANEL:
                        play_sound(SOUND_GENERAL_OPEN_IRON_DOOR, gGlobalSoundSource);
                        fnab_cam_index = 4;
                        fnab_office_statetimer = 0;
                        fnab_office_state = OFFICE_STATE_BREAKER;
                        break;
                }
            }
            break;
        case OFFICE_STATE_HIDE:
            play_sound(SOUND_ENV_BOAT_ROCKING1, gGlobalSoundSource);
            if (fnab_office_statetimer == 5) {
                fnab_cam_index = 3;
            }
            if (fnab_office_statetimer > 10) {
                if (!(gPlayer1Controller->buttonDown & Z_TRIG) && (n64_mouse_y > 30.0f)) {
                    fnab_office_state = OFFICE_STATE_UNHIDE;
                    fnab_cam_index = 2;
                    fnab_office_statetimer = 0;
                }
            }
            break;
        case OFFICE_STATE_UNHIDE:
            if (fnab_office_statetimer > 5) {
                fnab_office_statetimer = 0;
                fnab_cam_index = 0;
                fnab_office_state = OFFICE_STATE_DESK;
            }
            break;
        case OFFICE_STATE_LEAN_CAMERA:
            play_sound(SOUND_ENV_WATERFALL1, gGlobalSoundSource);
            monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MON];
            if (fnab_office_statetimer > 7) {
                fnab_cam_snap_or_lerp = 0;
                fnab_office_statetimer = 0;
                fnab_cam_index = fnab_cam_last_index;
                fnab_office_state = OFFICE_STATE_CAMERA;
            }
            break;
        case OFFICE_STATE_CAMERA:
            fnab_camera_flick_handler(0.0f);
            monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MON];

            camera_mouse_selecting = FALSE;

            camera_mouse_x += gPlayer1Controller->rawStickX/15.0f;
            camera_mouse_y += gPlayer1Controller->rawStickY/15.0f;

            camera_mouse_x = CLAMP(camera_mouse_x,-190,0);
            camera_mouse_y = CLAMP(camera_mouse_y,-150,0);

            if (n64_mouse_enabled) {
                camera_mouse_x = n64_mouse_x - 200.0f;
                camera_mouse_y = n64_mouse_y - 175.0f;
            }

            for (int i = 0; i < SECURITY_CAMERA_CT; i++) {
                if (securityCameras[i].init == FALSE) {continue;}

                f32 xdif = (securityCameras[i].x - camera_mouse_x);
                f32 ydif = (securityCameras[i].y - camera_mouse_y);
                f32 distsqr = xdif*xdif + ydif*ydif;

                if (distsqr < 64.0f) {
                    camera_mouse_selecting = TRUE;
                    n64_mouse_selecting = TRUE;
                    if (gPlayer1Controller->buttonPressed & A_BUTTON) {
                        if (securityCameras[i].type == SC_TYPE_CAMERA) {
                            fnab_cam_index = i;
                            fnab_cam_last_index = i;
                            camera_interference_timer = 6;
                            play_sound(SOUND_MENU_CLICK_CHANGE_VIEW, gGlobalSoundSource);
                        } else {
                            if (breakerCharges[0]>0&&securityCameras[i].doorStatus == 0) {
                                breakerCharges[0]--;
                                securityCameras[i].doorStatus = 1;
                                play_sound(SOUND_GENERAL_STAR_DOOR_CLOSE, gGlobalSoundSource);
                            }
                        }
                    }
                }
            }

            // place sound
            if (((gPlayer1Controller->buttonPressed & Z_TRIG)||(gPlayer2Controller->buttonPressed & B_BUTTON))&&(snd_timer==0)&&(breakerCharges[1]>0)) {
                snd_x = -camera_mouse_x/10.0f;
                snd_y = -camera_mouse_y/10.0f;
                if (get_map_data(snd_x,snd_y) > TILE_WALL) {
                    breakerCharges[1]--;

                    u16 rand_sound = random_u16()%3;
                    switch(rand_sound) {
                        case 0:
                            play_sound(SOUND_MARIO_HAHA, gGlobalSoundSource);
                            break;
                        case 1:
                            play_sound(SOUND_MARIO_UH, gGlobalSoundSource);
                            break;
                        case 2:
                            play_sound(SOUND_MARIO_YAHOO, gGlobalSoundSource);
                            break;
                    }

                    snd_timer = 30;
                    for (int i = 0; i<ENEMY_COUNT; i++) {
                        struct FnabEnemy * ce = &enemyList[i];

                        if (!ce->active) {continue;}
                        if (ce->state == FNABE_IDLE || ce->info->personality == PERSONALITY_LUIGI) {continue;}

                        f32 xdif = snd_x - ce->x;
                        f32 ydif = snd_y - ce->y;
                        f32 dsqrd = xdif*xdif + ydif*ydif;
                        if (dsqrd < 20.0f) {
                            ce->tx = snd_x;
                            ce->ty = snd_y;
                            ce->state = FNABE_DISTRACTED;
                            ce->progress += 5.0f;
                        }
                    }
                } else {
                    //err
                }
            }

            //activate vent flush
            if (((gPlayer1Controller->buttonPressed & L_TRIG)||(mouse_click_button(230,70,50.f)==2))&&(vent_flush_timer==0)&&(breakerCharges[2]>0)) {
                breakerCharges[2]--;
                vent_flush_timer = 200;
                play_sound(SOUND_OBJ_FLAME_BLOWN, gGlobalSoundSource);
                camera_interference_timer = 15;

                for (int i = 0; i<ENEMY_COUNT; i++) {
                    struct FnabEnemy * ce = &enemyList[i];

                    if (!ce->active) {continue;}
                    if (get_map_data(ce->x,ce->y) == TILE_VENT || ce->state == FNABE_PRIMED_VENT) {
                        ce->state = FNABE_WANDER;
                        ce->canVent = FALSE;
                        ce->stepCounter = 0;
                        //ce->tx = ce->ventFlushX;
                        //ce->ty = ce->ventFlushY;

                        ce->x = ce->ventFlushX;
                        ce->y = ce->ventFlushY;

                        fnab_enemy_set_target(ce);
                    }
                }
            }

            //activate radar
            if (((gPlayer1Controller->buttonPressed & R_TRIG)||(mouse_click_button(230,50,50.f)==2))&&(radar_timer==0)&&(breakerCharges[2]>0)) {
                breakerCharges[2]--;
                play_sound(SOUND_GENERAL_BOWSER_KEY_LAND, gGlobalSoundSource);
                radar_timer = 200;
            }

            if ((gPlayer1Controller->buttonPressed & B_BUTTON)||n64_mouse_camera_flick_state==1) {
                fnab_office_state = OFFICE_STATE_LEAVE_CAMERA;
                fnab_cam_index = 1;
                fnab_office_statetimer = 0;
            }

            break;
        case OFFICE_STATE_LEAVE_CAMERA:
            monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MON];
            if (fnab_office_statetimer == 2) {
                fnab_cam_snap_or_lerp = 1;
                fnab_cam_index = 0;
            }
            if (fnab_office_statetimer > 7) {
                fnab_office_statetimer = 0;
                fnab_cam_index = 0;
                fnab_office_state = OFFICE_STATE_DESK;
            }
            break;
        case OFFICE_STATE_BREAKER:
            fnab_camera_flick_handler(300.0f);
            if (fnab_office_statetimer > 7) {

                if (!breakerDoFix) {
                    handle_menu_scrolling(MENU_SCROLL_VERTICAL, &breakerIndex, 0, 3);

                    for (int i = 0; i < 4; i++) {
                        if (mouse_click_button(90.f,150.0f-(i*20.f),100.f)) {
                            breakerIndex = i;
                        }
                    }
                }

                if (!breakerDoFix&&(gPlayer1Controller->buttonPressed & A_BUTTON)) {
                    breakerDoFix = TRUE;
                    breakerFixing = 0.0f;
                    play_sound(SOUND_GENERAL_BOWSER_KEY_LAND, gGlobalSoundSource);
                }

                if ((gPlayer1Controller->buttonPressed & B_BUTTON)||n64_mouse_camera_flick_state==1) {
                    play_sound(SOUND_GENERAL_CLOSE_IRON_DOOR, gGlobalSoundSource);
                    fnab_office_state = OFFICE_STATE_DESK;
                    fnab_cam_index = 0;
                    fnab_office_statetimer = 0;
                }
            }
            break;
        case OFFICE_STATE_JUMPSCARED:
            if (0 == random_u16()%4) {
                cur_obj_shake_screen(SHAKE_POS_LARGE);
            }
            if (fnab_office_statetimer > 1) {
                fnab_cam_index = 0;
                fnab_cam_snap_or_lerp = 1;
                officePovCamera->oFaceAngleYaw = 0;
            }
            if (fnab_office_statetimer > 30) {
                play_sound(SOUND_ENV_WATERFALL1, gGlobalSoundSource);
            }
            if (fnab_office_statetimer == 120) {
                if (fnab_night_id == NIGHT_CUSTOM) {
                    gSaveBuffer.files[0]->customHi = old_custom_night_highscore;
                }
                if (fnab_night_id == NIGHT_ENDLESS) {
                    // save endless score
                    if (gSaveBuffer.files[0]->endlessHi < fnab_clock/30) {
                        gSaveBuffer.files[0]->endlessHi = fnab_clock/30;
                        gSaveFileModified = TRUE;
                        save_file_do_save(gCurrSaveFileNum - 1);
                    }
                }
                fade_into_special_warp(WARP_SPECIAL_MARIO_HEAD_REGULAR, 0); // reset game
            }
            break;
        case OFFICE_STATE_WON:
            if (fnab_office_statetimer == 120) {
                if (fnab_night_id == NIGHT_CUSTOM) {
                    gSaveFileModified = TRUE;
                    save_file_do_save(gCurrSaveFileNum - 1);
                }
                if (fnab_night_id + 1 > gSaveBuffer.files[0]->curNightProgress) {
                    // You can play the next night
                    gSaveBuffer.files[0]->curNightProgress = fnab_night_id + 1;
                    gSaveFileModified = TRUE;
                    save_file_do_save(gCurrSaveFileNum - 1);
                }
                fade_into_special_warp(WARP_SPECIAL_MARIO_HEAD_REGULAR, 0); // reset game
            }
            break;
    }
    fnab_office_statetimer++;

    if (camera_interference_timer > 0) {
        camera_interference_timer--;
        if (fnab_office_state == OFFICE_STATE_CAMERA) {
            play_sound(SOUND_ENV_WATERFALL1, gGlobalSoundSource);
        }
    }

    darknessObject->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
    if (light_interference_timer > 0) {
        darknessObject->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
        if (light_interference_timer < 5 && (random_u16()%2==0)) {
            darknessObject->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
        }
        light_interference_timer --;

        if (fnab_office_state != OFFICE_STATE_CAMERA) {
            play_sound(SOUND_AIR_AMP_BUZZ, gGlobalSoundSource);
        }
    }

    if (snd_timer > 0) {
        snd_timer --;
    }
    if (radar_timer > 0) {
        radar_timer--;
    }
    if (vent_flush_timer > 0) {
        vent_flush_timer --;
    }
    if (breakerDoFix == TRUE) {
        breakerFixing += 0.0075f; // Rebooting all takes half speed
        if (breakerIndex < 3) {
            breakerFixing += 0.0075f;
        }
        if (breakerFixing >= 7.f) {
            play_sound(SOUND_GENERAL_BOWSER_KEY_LAND, gGlobalSoundSource);
            breakerDoFix = FALSE;
            if (breakerIndex == 3) {
                for (u8 i = 0; i < 3; i++) {
                    breakerCharges[i] = breakerChargesMax[i];
                }
            } else {
                breakerCharges[breakerIndex] = breakerChargesMax[breakerIndex];
            }
        }
    }

    if (cartridgeTilt) {
        for (u8 i = 0; i < 3; i++) {
            breakerCharges[i] = 0;
        }
        breakerFixing = FALSE;
    }

    fnab_clock++;
    //fnab_clock += 1800;
    if (fnab_night_id != NIGHT_ENDLESS) {
        if (fnab_clock >= 1800*6 && fnab_office_state != OFFICE_STATE_WON) {
            fnab_office_state = OFFICE_STATE_WON;
            fnab_office_statetimer = 0;
            play_music(SEQ_PLAYER_ENV, SEQUENCE_ARGS(15, SEQ_WIN), 0);
        }
    } else {
        if (fnab_clock % 200 == 0) {
            u16 random_pick = random_u16()%ENEMY_COUNT;
            enemyList[random_pick].difficulty++;
        }
    }
}


/* MAIN MENU*/

u8 main_menu_state = 0;
s8 main_menu_index = 0;
u8 menu_seen_warning = FALSE;
u8 menu_a_hold_timer = 0;

void fnab_custom_night_button_loop(int x, int y, u8 * difficulty_changer) {
     if (mouse_click_button(x-16,y,16.0f) == 2) {
        (*difficulty_changer) = CLAMP((*difficulty_changer)-1,0,20);
     }
     if (mouse_click_button(x+16,y,16.0f) == 2) {
        (*difficulty_changer) = CLAMP((*difficulty_changer)+1,20,20);
     }
}

void fnab_main_menu_init(void) {
    main_menu_state = 3;
    if (menu_seen_warning) {
        main_menu_state = 0;
    }
    main_menu_index = 0;

    menu_seen_warning = TRUE;
}

s32 start_custom_night(void) {
    int thisNightHighscore = 0;
    for (int i = 0; i < 5; i++) {
        thisNightHighscore += nightEnemyDifficulty[NIGHT_CUSTOM][i];
    }
    //revert to this if fail
    old_custom_night_highscore = gSaveBuffer.files[0]->customHi;
    if (gSaveBuffer.files[0]->customHi < thisNightHighscore) {
        gSaveBuffer.files[0]->customHi = thisNightHighscore;
    }

    fnab_night_id = NIGHT_CUSTOM;
    return 1;
}

s32 fnab_main_menu(void) {
    fnab_mouse_loop();

    menu_a_hold_timer++;
    if (!(gPlayer1Controller->buttonDown & A_BUTTON)) {
        menu_a_hold_timer=0;
    }
    switch(main_menu_state) {
        case 0: // MAIN
            for (int i = 0; i < 5; i++) {
                if (mouse_click_button(35.f,100.0f-(i*20.f),150.f)) {
                    main_menu_index = i;
                }
            }

            handle_menu_scrolling(MENU_SCROLL_VERTICAL, &main_menu_index, 0, 4);
            if (gPlayer1Controller->buttonPressed & (A_BUTTON|START_BUTTON)) {
                switch(main_menu_index) {
                    case 0:
                        main_menu_state = 2;
                        break;
                    case 1:
                        if (gSaveBuffer.files[0]->curNightProgress >= NIGHT_CUSTOM) {
                            main_menu_state = 4;
                            main_menu_index = 0;
                        }
                        break;
                    case 2:
                        if (gSaveBuffer.files[0]->curNightProgress >= NIGHT_CUSTOM) {
                            fnab_night_id = NIGHT_ENDLESS;
                            return 1;
                        }
                        break;
                    case 3:
                        main_menu_state = 1;
                        break;
                    case 4:
                        main_menu_state = 5;
                        break;
                }
            }
            break;
        case 3: // EPILEPSY SCREEN
            if (n64_mouse_enabled) {
                if (mouse_click_button(20,10,200.f)==2) {
                    n64_mouse_enabled = FALSE;
                    n64_mouse_x = 160.0f;
                    n64_mouse_y = 120.0f;
                    main_menu_state = 0;
                }
                if (mouse_click_button(20,30,200.f)==2) {
                    main_menu_state = 0;
                }
            } else {
                if ((gPlayer1Controller->buttonPressed & (A_BUTTON|START_BUTTON|B_BUTTON))||exit_button()) {
                    main_menu_state = 0;
                }      
            }
            break;
        case 1: // CREDITS
        case 5: // CHANGELOG
            if ((gPlayer1Controller->buttonPressed & (A_BUTTON|START_BUTTON|B_BUTTON))||exit_button()) {
                main_menu_state = 0;
            }
            break;
        case 2: // NIGHT SELECT
            handle_menu_scrolling(MENU_SCROLL_VERTICAL, &main_menu_index, 0, 4);

            for (int i = 0; i < 5; i++) {
                if (mouse_click_button(35.f,150.0f-(i*20.f),150.f)) {
                    main_menu_index = i;
                }
            }

            if ((gPlayer1Controller->buttonPressed & (A_BUTTON|START_BUTTON))&&!exit_button_hover()) {
                if (gSaveBuffer.files[0]->curNightProgress >= main_menu_index) {
                    fnab_night_id = main_menu_index;
                    return 1;
                }
            }
            if ((gPlayer1Controller->buttonPressed & (B_BUTTON))||exit_button())  {
                main_menu_state = 0;
                main_menu_index = 0;
            }
            break;
        case 4: // CUSTOM NIGHT         
            handle_menu_scrolling(MENU_SCROLL_VERTICAL, &main_menu_index, 0, 5);
            if ((gPlayer1Controller->buttonPressed & R_TRIG)||exit_button()) {
                main_menu_state = 0;
                return 0;
            }

            if (n64_mouse_enabled) {
                fnab_custom_night_button_loop(80, 115 ,&nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_MOTOS]  );
                fnab_custom_night_button_loop(150, 115,&nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_BULLY]  );
                fnab_custom_night_button_loop(220, 115,&nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_WARIO]  );
                fnab_custom_night_button_loop(115, 35 ,&nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_LUIGI]  );
                fnab_custom_night_button_loop(185, 35 ,&nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_STANLEY]);

                if (mouse_click_button(40, 10, 140.0f) == 2) {
                    return start_custom_night();
                }
                break;
            }

            if ((main_menu_index ==5) && (gPlayer1Controller->buttonPressed & A_BUTTON)) {
                return start_custom_night();
            }

            if (menu_a_hold_timer > 15 || (gPlayer1Controller->buttonPressed & A_BUTTON)) {
                if (nightEnemyDifficulty[NIGHT_CUSTOM][main_menu_index] < 20) {
                    nightEnemyDifficulty[NIGHT_CUSTOM][main_menu_index] ++;
                }
            }
            if (gPlayer1Controller->buttonPressed & B_BUTTON) {
                if (nightEnemyDifficulty[NIGHT_CUSTOM][main_menu_index] > 0) {
                    nightEnemyDifficulty[NIGHT_CUSTOM][main_menu_index] --;
                }
            }
            break;
    }

    scroll_textures();
    return 0;
}

void fnab_custom_night_render_buttons(int x, int y) {
    if (n64_mouse_enabled) {
        print_text_fmt_int(x-16, y, "-",0);
        print_text_fmt_int(x+16, y, "+",0);
    }
}

void fnab_main_menu_render(void) {
    create_dl_ortho_matrix();

    //render camera static
    gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    render_camera_static(20);

    switch(main_menu_state) {
        case 0:
            print_text_fmt_int(10, 220, "NIGHTSHIFT AT THE", 0);
            print_text_fmt_int(10, 200, "BETA MUSEUM", 0);

            print_text_fmt_int(10, 100-(20*main_menu_index), "-", 0);
            print_text_fmt_int(35, 100, "START", 0);
            if (gSaveBuffer.files[0]->curNightProgress >= NIGHT_CUSTOM) {
                print_text_fmt_int(35, 100-20, "CUSTOM - HI %d", gSaveBuffer.files[0]->customHi);
                print_text_fmt_int(35, 100-40, "ENDLESS - HI %d", gSaveBuffer.files[0]->endlessHi);
            } else {
                print_text_fmt_int(35, 100-20, "???", 0);
                print_text_fmt_int(35, 100-40, "???", 0);
            }
            print_text_fmt_int(35, 100-60, "CREDITS", 0);
            print_text_fmt_int(35, 100-80, "CHANGELOG", 0);
            break;
        case 1: // CREDITS
            gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
            print_generic_string_ascii(15,220,"Hack by: Rovertronic\n\
\n\
Music used: B3313 OST\n\
\n\
Voice 1: Kaze\n\
Voice 2: Rovertronic\n\
Voice 3: Cheezepin\n\
\n\
Mario & Luigi Models: frijolesdotz64\n\
Beta font recreation: Simpson55\n\
Motos model: Arthurtilly\n\
\n\
April Fools btw : )");
            gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            exit_render();
            break;

        case 2: // NIGHT SELECT
            print_text_fmt_int(10, 150-(20*main_menu_index), "-", 0);
            for (int i = 0; i<5; i++) {
                if (gSaveBuffer.files[0]->curNightProgress < i) {
                    print_text_fmt_int(35, 150-(20*i), "???", i+1);
                } else {
                    print_text_fmt_int(35, 150-(20*i), "NIGHT %d", i+1);
                }
            }
            exit_render();
            break;

        case 3: // EPILEPSY WARNING
            if (n64_mouse_enabled) {
                print_text_fmt_int(20, 10, "USE CONTROLLER",0);
                print_text_fmt_int(20, 30, "USE MOUSE",0);

            gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
            print_generic_string_ascii(25,200,"EPILEPSY WARNING!\n\
\n\
This Super Mario 64 ROM hack\n\
contains flashing lights, loud sounds,\n\
and jumpscares.");
            gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            } else {
                gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
                gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
                print_generic_string_ascii(25,200,"EPILEPSY WARNING!\n\
\n\
This Super Mario 64 ROM hack\n\
contains flashing lights, loud sounds,\n\
and jumpscares.\n\
\n\
Press START to continue.");
            gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            }
            break;
        case 4: //CUSTOM NIGHT
            gDPSetRenderMode(gDisplayListHead++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
            create_dl_translation_matrix(MENU_MTX_PUSH, 160, 120, 0);
            if (is_b3313_night()) {
                gSPDisplayList(gDisplayListHead++, cne_cne_mesh);
            } else {
                gSPDisplayList(gDisplayListHead++, cn_cn_mesh);
            }
            gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

            print_text_fmt_int(80, 115, "%x",  nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_MOTOS]  );
            print_text_fmt_int(150, 115, "%x", nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_BULLY]  );
            print_text_fmt_int(220, 115, "%x", nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_WARIO]  );
            print_text_fmt_int(115, 35, "%x",  nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_LUIGI]  );
            print_text_fmt_int(185, 35, "%x",  nightEnemyDifficulty[NIGHT_CUSTOM][ENEMY_STANLEY]);

            fnab_custom_night_render_buttons(80, 115 );
            fnab_custom_night_render_buttons(150, 115);
            fnab_custom_night_render_buttons(220, 115);
            fnab_custom_night_render_buttons(115, 35 );
            fnab_custom_night_render_buttons(185, 35 );

            int thisNightHighscore = 0;
            for (int i = 0; i < 5; i++) {
                thisNightHighscore += nightEnemyDifficulty[NIGHT_CUSTOM][i];
            }

            if (is_b3313_night()) {
                print_text_fmt_int(40, 10, "START - B3313 NIGHT",0);
            } else {
                print_text_fmt_int(40, 10, "START - SCORE %d", thisNightHighscore);
                if (thisNightHighscore >= 60) {

                }
            }

            if (!n64_mouse_enabled) {
                gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
                gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
                print_generic_string_ascii(25,200,"A - Increase, B - Decrease, R - Go Back");
                gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            }

            if (!n64_mouse_enabled) {
                int x = 0;
                int y = 0;
                switch(main_menu_index) {
                    case ENEMY_MOTOS:
                        x = 80;
                        y = 115;
                        break;
                    case ENEMY_BULLY:
                        x = 150;
                        y = 115;
                        break;
                    case ENEMY_WARIO:
                        x = 220;
                        y = 115;
                        break;
                    case ENEMY_LUIGI:
                        x = 115;
                        y = 35;
                        break;
                    case ENEMY_STANLEY:
                        x = 185;
                        y = 35;
                        break;
                    case 5:
                        x = 30;
                        y = 10;
                        break;
                }
                x -= 16;
                print_text_fmt_int(x, y, "^", 0);
            }
            exit_render();
            break;
        case 5: // CUSTOM NIGHT
            gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);
            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
            print_generic_string_ascii(15,220,"Changelog v1.1 5-9-25\n\
\n\
- Added N64 mouse support\n\
- Nights now unlock sequentially\n\
- Game can now save progress\n\
- Added custom night\n\
- Added endless night\n\
- Added a secret night\n\
- Made night 5 easier\n\
- Improved Stanley jumpscare\n\
- Improved Stanley AI\n\
- Fixed visual bugs on Luna PJ64\n\
- Fixed visual bugs on N64 hardware\n\
- Map has been tweaked and improved");
            gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
            exit_render();
            break;
    }

    //fnab_mouse_render();
};