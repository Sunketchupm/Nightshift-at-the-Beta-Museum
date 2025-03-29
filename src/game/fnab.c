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
{F F F F _ _ F _ _ F _ _ F F F F F F F _},
{F F F F _ _ F _ F F F _ F _ _ _ F _ F _},
{F F F F F F F F F _ F F F _ _ _ F _ F _},
{F _ _ _ _ F _ _ F F F _ F _ _ _ F _ F _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ F _},
{F F F _ F F F _ _ _ _ _ W _ _ _ F _ F _},
{F F F _ _ V _ _ _ F W W F _ _ _ F _ F _},
{_ _ _ _ _ V _ _ _ L _ _ _ F F F F _ F _},
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

void add_path_branch(s8 x, s8 y, u8 allowVent) {
    u8 mapdata = get_map_data(x,y);
    u8 cond = (mapdata > 0);
    if (allowVent == FALSE) {
        cond = ((mapdata > 0)&&(mapdata != 2));
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

    bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);
    bzero(&pathBranchStack,sizeof(pathfindingMap[0][0])*PATH_BRANCH_STACK_CT);

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

u8 fnab_cam_index = 0;
u8 fnab_cam_snap_or_lerp = 0;
u8 fnab_office_state = OFFICE_STATE_DESK;
u8 fnab_office_action = OACTION_HIDE;
int fnab_office_statetimer = 0;

u8 camera_interference_timer = 0;

f32 camera_mouse_x = 0.0f;
f32 camera_mouse_y = 0.0f;
u8 camera_mouse_selecting = FALSE;

#define SECURITY_CAMERA_CT 15
struct securityCameraInfo securityCameras[SECURITY_CAMERA_CT] = {
    {.name = NULL},
    {.name = NULL},
    {.name = NULL},
    {.name = NULL},
    {.name = NULL},
    // First 5 cameras are for office POV
    {.name = "TRAMPOLINE EXHIBIT"},
    {.name = "HALL L"},
    {.name = "LAVA EXHIBIT"},
    {.name = "L IS REAL EXHIBIT"},
    {.name = "APPARITION HALL"},
    {.name = "RESTROOMS"},
    {.name = "VENT"},
    {.name = "HALL R"},
};

void bhv_fnab_camera(void) {
    if (o->oBehParams2ndByte == 0) {
        officePovCamera = o;
    }

    if (!securityCameras[o->oBehParams2ndByte].init) {
        securityCameras[o->oBehParams2ndByte].init = TRUE;
        securityCameras[o->oBehParams2ndByte].x = -o->oPosX/20.0f;
        securityCameras[o->oBehParams2ndByte].y = o->oPosZ/20.0f;
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
            
            for (int i = 0; i < 3; i++) {
                fnabCameraPos[i] = approach_f32_asymptotic(fnabCameraPos[i],fnabCameraPosTarget[i],.2f);
                fnabCameraFoc[i] = approach_f32_asymptotic(fnabCameraFoc[i],fnabCameraFocTarget[i],.2f);
            }
        }
    }
}

void fnab_enemy_init(struct fnabEnemy * cfe) {
    cfe->progress = 0.0f;
    cfe->x = 5;
    cfe->y = 0;
    cfe->tx = 9;
    cfe->ty = 11;
    cfe->state = FNABE_WANDER;
    cfe->modelObj = spawn_object(gMarioObject,MODEL_MOTOS,bhvMotos);
 
    cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f;
    cfe->modelObj->oPosZ = (200.0f*cfe->y)+100.0f;
}

void fnab_enemy_step(struct fnabEnemy * cfe) {
    cfe->progress += 0.01f;

    if (cfe->progress >= 1.0f) {
        cfe->progress -= 1.0f;

        if (cfe->state < FNABE_PRIMED) {
            // Wandering around map

            camera_interference_timer = 10;

            u8 tile_landed = 0;
            u16 steps = 1+(random_u16()%3);
            if (get_map_data(cfe->x,cfe->y) == 3) {
                steps = 1;
            }
            for (int i = 0; i < steps; i++) {
                u8 dir = path_find(cfe->tx,cfe->ty,cfe->x,cfe->y,FALSE);
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

    if (fnab_office_state == OFFICE_STATE_CAMERA) {
        u8 static_alpha = 10;
        if (camera_interference_timer > 0) {
            static_alpha = 255;
        }

        //render camera static
        gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, static_alpha);
        create_dl_translation_matrix(MENU_MTX_PUSH, 160, 120, 0);
        gSPDisplayList(gDisplayListHead++, staticscreen_ss_mesh);
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

        create_dl_translation_matrix(MENU_MTX_PUSH, 200, 175, 0);
        gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
        gSPDisplayList(gDisplayListHead++, cammap_cammap_mesh);

        //render camera buttons
        for (int i = 0; i < SECURITY_CAMERA_CT; i++) {
            if (securityCameras[i].name == NULL) {continue;}

            gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
            if (i == fnab_cam_index) {gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);}

            create_dl_translation_matrix(MENU_MTX_PUSH, securityCameras[i].x, securityCameras[i].y, 0);
                create_dl_rotation_matrix(MENU_MTX_PUSH, (securityCameras[i].angle/65535.f)*360.f+180.0f , 0, 0, 1.0f);
                    gSPDisplayList(gDisplayListHead++, cb_cb_mesh);
                gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
            gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
        }

        create_dl_translation_matrix(MENU_MTX_PUSH, camera_mouse_x, camera_mouse_y, 0);
            if (!camera_mouse_selecting) {
                gSPDisplayList(gDisplayListHead++, mouse1_mouse1_mesh);
            } else {
                gSPDisplayList(gDisplayListHead++, mouse2_mouse2_mesh);
            }
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
    }

    print_text_fmt_int(10, 200, "12AM", 0);
    if (fnab_office_state == OFFICE_STATE_DESK) {
        switch(fnab_office_action) {
            case OACTION_CAMERA:
                print_text_fmt_int(10, 10, "CAMERA", 0);
                break;
            case OACTION_HIDE:
                print_text_fmt_int(160, 10, "HIDE", 0);
                break;
            case OACTION_PANEL:
                print_text_fmt_int(240, 10, "BREAKER", 0);
                break;
        }
    }
    if (fnab_office_state == OFFICE_STATE_CAMERA) {
        print_text_fmt_int(10, 5, securityCameras[fnab_cam_index].name, 0);
    }
}

void fnab_init(void) {
    fnab_enemy_init(&testEnemy);
    fnab_cam_snap_or_lerp = 0;
}

#define OACTHRESH 0x600

void fnab_loop(void) {
    if (officePovCamera == NULL) {return;}

    fnab_enemy_step(&testEnemy);

    switch(fnab_office_state) {
        case OFFICE_STATE_DESK:
            fnab_cam_snap_or_lerp = 1;
            if (officePovCamera->oFaceAngleYaw < -OACTHRESH) {
                fnab_office_action = OACTION_PANEL;
            }
            if (officePovCamera->oFaceAngleYaw >= -OACTHRESH && officePovCamera->oFaceAngleYaw <= OACTHRESH) {
                fnab_office_action = OACTION_HIDE;
            }
            if (officePovCamera->oFaceAngleYaw > OACTHRESH) {
                fnab_office_action = OACTION_CAMERA;
            }

            if (gPlayer1Controller->buttonPressed & A_BUTTON) {
                switch(fnab_office_action) {
                    case OACTION_HIDE:
                        fnab_cam_index = 2;
                        fnab_office_statetimer = 0;
                        fnab_office_state = OFFICE_STATE_HIDE;
                        break;
                    case OACTION_CAMERA:
                        fnab_cam_index = 1;
                        fnab_office_statetimer = 0;
                        fnab_office_state = OFFICE_STATE_LEAN_CAMERA;
                        break;
                }
            }
            break;
        case OFFICE_STATE_HIDE:
            if (fnab_office_statetimer == 10) {
                fnab_cam_index = 3;
            }
            if (fnab_office_statetimer > 20) {
                if (gPlayer1Controller->buttonPressed & A_BUTTON) {
                    fnab_office_state = OFFICE_STATE_UNHIDE;
                    fnab_cam_index = 2;
                    fnab_office_statetimer = 0;
                }
            }
            break;
        case OFFICE_STATE_UNHIDE:
            if (fnab_office_statetimer > 10) {
                fnab_office_statetimer = 0;
                fnab_cam_index = 0;
                fnab_office_state = OFFICE_STATE_DESK;
            }
            break;
        case OFFICE_STATE_LEAN_CAMERA:
            if (fnab_office_statetimer > 10) {
                fnab_cam_snap_or_lerp = 0;
                fnab_office_statetimer = 0;
                fnab_cam_index = 5;
                fnab_office_state = OFFICE_STATE_CAMERA;
            }
            break;
        case OFFICE_STATE_CAMERA:
            camera_mouse_x += gPlayer1Controller->rawStickX/15.0f;
            camera_mouse_y += gPlayer1Controller->rawStickY/15.0f;
            camera_mouse_selecting = FALSE;

            for (int i = 0; i < SECURITY_CAMERA_CT; i++) {
                if (securityCameras[i].name == NULL) {continue;}

                f32 xdif = (securityCameras[i].x - camera_mouse_x);
                f32 ydif = (securityCameras[i].y - camera_mouse_y);
                f32 distsqr = xdif*xdif + ydif*ydif;

                if (distsqr < 64.0f) {
                    camera_mouse_selecting = TRUE;
                    if (gPlayer1Controller->buttonPressed & A_BUTTON) {
                        fnab_cam_index = i;
                        camera_interference_timer = 10;
                    }
                }
            }

            if (gPlayer1Controller->buttonPressed & B_BUTTON) {
                fnab_office_state = OFFICE_STATE_LEAVE_CAMERA;
                fnab_cam_index = 1;
                fnab_office_statetimer = 0;
            }
            break;
        case OFFICE_STATE_LEAVE_CAMERA:
            if (fnab_office_statetimer == 2) {
                fnab_cam_snap_or_lerp = 1;
                fnab_cam_index = 0;
            }
            if (fnab_office_statetimer > 10) {
                fnab_office_statetimer = 0;
                fnab_cam_index = 0;
                fnab_office_state = OFFICE_STATE_DESK;
            }
            break;
    }
    fnab_office_statetimer++;

    if (camera_interference_timer > 0) {
        camera_interference_timer--;
    }

}