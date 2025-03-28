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

#define _ 0, // Wall / Nothing
#define F 1, // Floor  
#define V 2, // Vent
#define W 3, // Visible from Window
#define L 4, // Left door (Inverted in array)

u8 fnabMap[20][20] = {
{F F F F _ F F F _ F F F _ _ _ _ _ _ _ _},
{F F F F _ F F F _ F F F _ F _ F _ _ _ _},
{F F F F _ _ F F _ F F _ _ F F F F _ _ _},
{F F F F V V F F F F F V V _ _ _ F _ _ _},
{F F F F _ _ F _ _ F _ _ F F F F F _ _ _},
{F F F F _ _ F _ F F F _ F _ _ _ F _ _ _},
{F F F F F F F F F _ F F F _ _ _ F _ _ _},
{F _ _ _ _ F _ _ F F F _ F _ _ _ F _ _ _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ _ _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ _ _},
{F F F _ _ V _ _ _ F W W F _ _ _ F _ _ _},
{_ _ _ _ _ V _ _ _ L _ _ _ F F F F _ _ _},
{_ _ _ _ _ V _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ V _ _ _ _ _ V _ _ _ _ _ _ _ _},
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
#undef L

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

u8 get_map_data(s8 x, s8 y) {
    if ((x<MAP_SIZE&&x>=0)&&(y<MAP_SIZE&&y>=0)) {
        return pathfindingMap[y][x];
    }
    return 0;
}

void add_path_branch(s8 x, s8 y) {
    if (get_map_data(x,y) > 0) {
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

u8 path_find(s8 xs, s8 ys, s8 xd, s8 yd) {
    if (xs == xd && ys == yd) {return MAPDIR_ARRIVED;}

    bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);
    bzero(&pathBranchStack,sizeof(pathfindingMap[0][0])*PATH_BRANCH_STACK_CT);

    pathBranchCount = 0;
    add_path_branch(xs,ys);

    // Flood fill from start to destination
    while(pathBranchCount > 0) {
        for (int i = 0; i < PATH_BRANCH_STACK_CT; i++) {
            struct pathBranch * cpb = &pathBranchStack[i];
            if (cpb->active) {
                s8 x = cpb->x; 
                s8 y = cpb->y;

                for (int d = 0; d < 4; d++) {
                    if (x+dirOffset[d][0] == xd && y+dirOffset[d][1] == yd) {return d;}
                    add_path_branch(x+dirOffset[d][0],y+dirOffset[d][1]);
                }
                
                // Remove current active path branch and mark as solid on pathfinding map
                cpb->active = 0;
                pathfindingMap[y][x] = 0;
                pathBranchCount --;
            }
        }
    }
    return MAPDIR_NO_PATH;
}

f32 deltaTime = 1.0f;
Vec3f fnabCameraPos;
Vec3f fnabCameraFoc;
struct Object * officePovCamera = NULL;

void bhv_fnab_camera(void) {
    officePovCamera = o;

    o->oFaceAngleYaw += -gPlayer1Controller->rawStickX*7;

    random_u16();

    if (o->oFaceAngleYaw > o->oMoveAngleYaw + 0x1200) {
        o->oFaceAngleYaw = o->oMoveAngleYaw + 0x1200;
    }
    if (o->oFaceAngleYaw < o->oMoveAngleYaw + -0x1200) {
        o->oFaceAngleYaw = o->oMoveAngleYaw + -0x1200;
    }

    vec3f_copy(fnabCameraPos,&o->oPosVec);
    fnabCameraFoc[0] = o->oPosX + sins(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
    fnabCameraFoc[1] = o->oPosY + sins(o->oFaceAnglePitch) * -5.0f;
    fnabCameraFoc[2] = o->oPosZ + coss(o->oFaceAngleYaw) * coss(o->oFaceAnglePitch) * 5.0f;
}

void fnab_enemy_init(struct fnabEnemy * cfe) {
    cfe->progress = 0.0f;
    cfe->x = 1;
    cfe->y = 1;
    cfe->tx = 9;
    cfe->ty = 11;
    cfe->state = FNABE_WANDER;
    cfe->modelObj = spawn_object(gMarioObject,MODEL_MOTOS,bhvMotos);
 
    cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f;
    cfe->modelObj->oPosZ = (-200.0f*cfe->y)-100.0f;
}

void fnab_enemy_step(struct fnabEnemy * cfe) {
    cfe->progress += 0.01f;

    if (cfe->progress >= 1.0f) {
        cfe->progress -= 1.0f;

        if (cfe->state < FNABE_PRIMED) {
            // Wandering around map

            u8 tile_landed = 0;
            u16 steps = 1+(random_u16()%3);
            if (get_map_data(cfe->x,cfe->y) == 3) {
                steps = 1;
            }
            for (int i = 0; i < steps; i++) {
                u8 dir = path_find(cfe->tx,cfe->ty,cfe->x,cfe->y);
                bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);

                cfe->x -= dirOffset[dir][0];
                cfe->y -= dirOffset[dir][1];

                tile_landed = get_map_data(cfe->x,cfe->y);

                f32 model_x_offset = 0.0f;
                switch(tile_landed) {
                    case 4:
                        cfe->state = FNABE_PRIMED_LEFT;
                        model_x_offset = 50.0f;
                        break;
                }

                cfe->modelObj->oFaceAngleYaw = (dir*0x4000) + 0x8000;
                cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f+model_x_offset;
                cfe->modelObj->oPosZ = (-200.0f*cfe->y)-100.0f;
            }
            if (tile_landed >= 3) {
                cfe->modelObj->oFaceAngleYaw = obj_angle_to_object(cfe->modelObj,officePovCamera); //stare at the player
            }
        } else {
            // About to attack
        }
    }
}

struct fnabEnemy testEnemy;

void fnab_render_2d(void) {

}

void fnab_init(void) {
    fnab_enemy_init(&testEnemy);
}

void fnab_loop(void) {
    fnab_enemy_step(&testEnemy);
}