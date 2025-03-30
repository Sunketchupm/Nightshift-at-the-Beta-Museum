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

#define _ 0, // Wall / Nothing
#define F 1, // Floor  
#define V 2, // Vent
#define W 3, // Visible from Window
#define A 4, // Attack zone

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
{V _ _ _ _ V _ _ _ A _ _ _ A F F F _ F _},
{V V V V V V _ _ _ _ _ _ _ _ _ _ _ _ _ _},
{_ _ _ _ _ V _ _ _ _ _ A _ _ _ _ _ _ _ _},
{_ _ _ _ _ V V V V V V W _ _ _ _ _ _ _ _},
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

f32 camera_mouse_x = 0.0f;
f32 camera_mouse_y = 0.0f;
u8 camera_mouse_selecting = FALSE;

u8 snd_x = -1;
u8 snd_y = -1;
u8 snd_timer = 0;
u8 radar_timer = 0;
u8 vent_flush_timer = 0;

s8 breakerIndex = 0;
u8 breakerCharges[3] = {4,4,2};
u8 breakerChargesMax[3] = {4,4,2};
f32 breakerFixing = 0.0f;
u8 breakerDoFix = FALSE;

struct enemyInfo motosInfo = {
    .homeX = 5,
    .homeY = 0,
    .canVent = FALSE,
    .modelBhv = bhvMotos,
    .modelId = MODEL_MOTOS,
    .frequency = 0.02f,
    .tableAttackChance = .1f,

    .choice = {FNABE_PRIMED_LEFT,FNABE_PRIMED_LEFT,FNABE_PRIMED_RIGHT},

    .anim[ANIMSLOT_NORMAL] = 6,
    .anim[ANIMSLOT_WINDOW] = 8,
    .anim[ANIMSLOT_VENT] = 0,
    .anim[ANIMSLOT_JUMPSCARE] = 6,

    .jumpscareScale = .6f,
};

struct enemyInfo bullyInfo = {
    .homeX = 7,
    .homeY = 0,
    .canVent = TRUE,
    .modelBhv = bhvBetaBully,
    .modelId = MODEL_BETABULLY,
    .frequency = 0.03f,
    .tableAttackChance = .9f,

    .choice = {FNABE_PRIMED_VENT,FNABE_PRIMED_VENT,FNABE_PRIMED_RIGHT},

    .anim[ANIMSLOT_NORMAL] = 0,
    .anim[ANIMSLOT_WINDOW] = 0,
    .anim[ANIMSLOT_VENT] = 0,
    .anim[ANIMSLOT_JUMPSCARE] = 3,

    .jumpscareScale = .55f,
};

struct fnabEnemy enemyList[ENEMY_COUNT];

#define SECURITY_CAMERA_CT 20
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
    {.name = "SPACEWORLD CASTLE"},
    {.name = "CARTRIDGE ROOM"},
    {.name = "CLOSET"},
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

void fnab_enemy_init(struct fnabEnemy * cfe, struct enemyInfo * info) {
    cfe->active = TRUE;
    cfe->progress = 0.0f;
    cfe->x = info->homeX;
    cfe->y = info->homeY;
    cfe->tx = info->homeX;
    cfe->ty = info->homeY;
    cfe->state = FNABE_WANDER;
    cfe->modelObj = spawn_object(gMarioObject,info->modelId,info->modelBhv);
    cfe->info = info;
    cfe->animFrameHold = random_u16();
 
    cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f;
    cfe->modelObj->oPosZ = (200.0f*cfe->y)+100.0f;
}

void fnab_enemy_set_target(struct fnabEnemy * cfe) {
    //set new target based on state
    switch(cfe->state) {
        case FNABE_WANDER:;
            u8 random_x;
            u8 random_y;
            u8 emergency_break = 0;
            do {
                random_x = cfe->x + 5 - random_u16()%10;
                random_y = cfe->y + 5 -random_u16()%10;
                emergency_break++;
            } while (get_map_data(random_x,random_y)==0 && emergency_break < 10);
            cfe->tx = random_x;
            cfe->ty = random_y;
            break;
        case FNABE_ATTACK:;
            u8 attack_location_index = random_u16()%3;
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
    }
}

void fnab_enemy_step(struct fnabEnemy * cfe) {
    if (!cfe->active) {return;}

    if (cfe->state == FNABE_JUMPSCARE) {
        //play_sound(SOUND_OBJ2_SMALL_BULLY_ATTACKED, gGlobalSoundSource);
        cfe->modelObj->oPosX = officePovCamera->oPosX + sins(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f;
        cfe->modelObj->oPosY = officePovCamera->oPosY + -45.0f + cfe->jumpscareYoffset; // + sins(officePovCamera->oFaceAnglePitch) * -5.0f;
        cfe->modelObj->oPosZ = officePovCamera->oPosZ + coss(officePovCamera->oFaceAngleYaw) * coss(officePovCamera->oFaceAnglePitch) * 75.0f;
        cfe->modelObj->oFaceAngleYaw = obj_angle_to_object(cfe->modelObj,officePovCamera);
        obj_scale(cfe->modelObj,cfe->info->jumpscareScale);
        cfe->modelObj->header.gfx.animInfo.animFrame+=2;
        cfe->jumpscareYoffset *= .7f;
        return;
    }

    cfe->progress += cfe->info->frequency;
    if (cfe->state == FNABE_FLUSHED) {
        cfe->progress += cfe->info->frequency*2.0f;
    }

    if (cfe->state < FNABE_PRIMED) {
        // Wandering around map
        if (cfe->progress >= 1.0f) {
            cfe->progress -= 1.0f;

            camera_interference_timer = 10;

            u8 tile_landed = 0;
            u16 steps = 1+(random_u16()%3);
            if (get_map_data(cfe->x,cfe->y) == 3) {
                steps = 1;
                light_interference_timer = 10;
            }
            for (int i = 0; i < steps; i++) {
                cfe->animFrameHold = random_u16();

                u8 dir = path_find(cfe->tx,cfe->ty,cfe->x,cfe->y,cfe->info->canVent);
                bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);

                cfe->x -= dirOffset[dir][0];
                cfe->y -= dirOffset[dir][1];

                //if touching another enemy, back up
                for (int i = 0; i < ENEMY_COUNT; i++) {
                    if (enemyList[i].active) {
                        if (cfe != &enemyList[i] && enemyList[i].x == cfe->x && enemyList[i].y == cfe->y) {
                            cfe->x += dirOffset[dir][0];
                            cfe->y += dirOffset[dir][1];
                            cfe->state = FNABE_WANDER;
                            fnab_enemy_set_target(cfe);
                        }
                    }
                }

                tile_landed = get_map_data(cfe->x,cfe->y);

                if (tile_landed == 1) {
                    cfe->ventFlushX = cfe->x;
                    cfe->ventFlushY = cfe->y;
                }

                //what to do when at destination
                if (dir >= MAPDIR_NO_PATH) {
                    switch(cfe->state) {
                        case FNABE_WANDER:
                            if (random_u16()%2==0) {
                                //1/2 chance to start attacking
                                cfe->state = FNABE_ATTACK;
                            }
                            break;
                        case FNABE_DISTRACTED:
                        case FNABE_FLUSHED:
                            cfe->state = FNABE_WANDER;
                            break;
                        case FNABE_ATTACK:
                            //weird, but needed
                            cfe->state = FNABE_WANDER;
                            break;
                    }

                    fnab_enemy_set_target(cfe);
                }

                if (tile_landed == 4) {
                    cfe->state = cfe->attackLocation;
                    cfe->progress = 0.0f;
                }

                f32 zoff = 0.0f;
                if (cfe->state == FNABE_PRIMED_LEFT || cfe->state == FNABE_PRIMED_RIGHT) {
                    zoff = 70.0f;
                }

                cfe->modelObj->oFaceAngleYaw = (dir*0x4000) + 0x8000;
                cfe->modelObj->oPosX = (200.0f*cfe->x)+100.0f;
                cfe->modelObj->oPosZ = (-200.0f*cfe->y)-100.0f+zoff;
                cfe->modelObj->oPosY = find_floor_height(cfe->modelObj->oPosX,500.0f,cfe->modelObj->oPosZ);
            }
            if (tile_landed >= 3) {
                cfe->modelObj->oFaceAngleYaw = obj_angle_to_object(cfe->modelObj,officePovCamera); //stare at the player
                light_interference_timer = 10;
            }
            //set tile animation
            switch(tile_landed) {
                case 1:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_NORMAL]);
                    break;
                case 2:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_VENT]);
                    break;
                case 3:
                case 4:
                    obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_WINDOW]);
                    break;
            }
        }
    } else {
        // About to attack
        if (cfe->progress > 5.0f) { //waittime til kill

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

            //if player is under desk
            if (fnab_office_state == OFFICE_STATE_HIDE || fnab_office_state == OFFICE_STATE_UNHIDE) {
                if (cfe->attackLocation == FNABE_PRIMED_VENT) {
                    canjumpscare = FALSE; //cant jumpscare if being stared at
                } else {
                    f32 saving_roll = random_float();
                    if (saving_roll > cfe->info->tableAttackChance) {
                        cfe->x = cfe->info->homeX;
                        cfe->y = cfe->info->homeY;
                        cfe->state = FNABE_WANDER;
                        cfe->progress = 1.0f;
                        light_interference_timer = 10;
                        canjumpscare = FALSE;
                    } else {
                        canjumpscare = TRUE;
                    }
                }
            }
            if (fnab_office_state == OFFICE_STATE_JUMPSCARED) {
                //prevent double jumpscaring
                canjumpscare = FALSE;
            }

            if (canjumpscare) {
                if (fnab_office_state == OFFICE_STATE_CAMERA) {
                    fnab_cam_snap_or_lerp = 0;
                    fnab_cam_index = 1;
                }
                fnab_office_state = OFFICE_STATE_JUMPSCARED;
                fnab_office_statetimer = 0;
                cfe->state = FNABE_JUMPSCARE;

                cfe->jumpscareYoffset = -75.f;
                obj_init_animation_with_sound_notshit(cfe->modelObj,cfe->info->anim[ANIMSLOT_JUMPSCARE]);
            }
        }
    }

    cfe->modelObj->header.gfx.animInfo.animFrame = cfe->animFrameHold%cfe->modelObj->header.gfx.animInfo.curAnim->loopEnd;
}

void print_breaker_status(u16 x, u16 y) {
    print_text_fmt_int(x, y, "DOORS", 0);
    print_text_fmt_int(x, y-20, "AUDIO", 0);
    print_text_fmt_int(x, y-40, "UTILITY", 0);

    for (int i = 0; i<3; i++) {
        for (int j = 0; j<breakerCharges[i]; j++) {
            print_text_fmt_int(x+90+j*4, y-i*20, "I", 0);
        }
    }
}

void fnab_render_2d(void) {

    //GAME OVER RENDER
    if (fnab_office_state == OFFICE_STATE_JUMPSCARED && fnab_office_statetimer > 30) {
        //render camera static
        gDPSetEnvColor(gDisplayListHead++, 255, 255, 255, 255);
        create_dl_translation_matrix(MENU_MTX_PUSH, 160, 120, 0);
        gSPDisplayList(gDisplayListHead++, staticscreen_ss_mesh);
        gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

        print_text_fmt_int(110, 110, "GAME OVER", 0);

        return;
    }

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

    print_text_fmt_int(10, 230, "12AM", 0);
    if (fnab_office_state == OFFICE_STATE_DESK) {
        switch(fnab_office_action) {
            case OACTION_CAMERA:
                print_text_fmt_int(10, 10, "CAMERA", 0);
                break;
            case OACTION_HIDE:
                print_text_fmt_int(150, 10, "HIDE", 0);
                break;
            case OACTION_PANEL:
                print_text_fmt_int(220, 10, "BREAKER", 0);
                break;
        }
    }
    if (fnab_office_state == OFFICE_STATE_CAMERA) {
        print_breaker_status(190,220);
        print_text_fmt_int(10, 5, securityCameras[fnab_cam_index].name, 0);

        gSPDisplayList(gDisplayListHead++, dl_ia_text_begin);

        //shadow
        gDPSetEnvColor(gDisplayListHead++, 0, 0, 0, 255);
        print_generic_string_ascii(230,30-1,"Z - Play Sound");
        print_generic_string_ascii(230,50-1,"R - Radar");
        print_generic_string_ascii(230,70-1,"L - Flush Vents");

        if (get_map_data(-camera_mouse_x/10.f,-camera_mouse_y/10.f)) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,30,"Z - Play Sound");

        if (radar_timer == 0) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,50,"R - Radar");

        if (vent_flush_timer == 0) {
            gDPSetEnvColor(gDisplayListHead++, 0, 255, 0, 255);
        } else {
            // cant play sound
            gDPSetEnvColor(gDisplayListHead++, 255, 0, 0, 255);
        }
        print_generic_string_ascii(230,70,"L - Flush Vents");

        gSPDisplayList(gDisplayListHead++, dl_ia_text_end);
    }
    if (fnab_office_state == OFFICE_STATE_BREAKER && fnab_office_statetimer > 10) {
        print_text_fmt_int(90, 150-breakerIndex*20, "^", 0);
        print_breaker_status(110,150);

        if (breakerDoFix) {
            for (int i = 0; i<7;i++) {
                char * printChar = ".";
                if (breakerFixing >= i) {
                    printChar = "^";
                }
                print_text_fmt_int(90+i*20, 80, printChar, 0);
            }
        }
    }
}

void fnab_init(void) {
    darknessObject = spawn_object(gMarioObject,MODEL_DARKNESS,bhvStaticObject);
    darknessObject->oPosX = 0.0f;
    darknessObject->oPosY = 0.0f;
    darknessObject->oPosZ = 0.0f;

    monitorScreenObject = spawn_object(gMarioObject,MODEL_MOFF,bhvStaticObject);
    monitorScreenObject->oPosX = 0.0f;
    monitorScreenObject->oPosY = 0.0f;
    monitorScreenObject->oPosZ = 0.0f;

    camera_mouse_x = -50.0f;
    camera_mouse_y = -50.0f;

    fnab_enemy_init(&enemyList[ENEMY_MOTOS],&motosInfo);
    fnab_enemy_init(&enemyList[ENEMY_BULLY],&bullyInfo);

    fnab_cam_snap_or_lerp = 0;

    for (int i = 0; i < 3; i++) {
        breakerCharges[i] = breakerChargesMax[i];
    }
}

#define OACTHRESH 0x600

void fnab_loop(void) {
    if (officePovCamera == NULL) {return;}

    for (int i = 0; i<ENEMY_COUNT; i++) {
        fnab_enemy_step(&enemyList[i]);
    }

    monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MOFF];
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
                    case OACTION_PANEL:
                        fnab_cam_index = 4;
                        fnab_office_statetimer = 0;
                        fnab_office_state = OFFICE_STATE_BREAKER;
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
            monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MON];
            if (fnab_office_statetimer > 10) {
                fnab_cam_snap_or_lerp = 0;
                fnab_office_statetimer = 0;
                fnab_cam_index = fnab_cam_last_index;
                fnab_office_state = OFFICE_STATE_CAMERA;
            }
            break;
        case OFFICE_STATE_CAMERA:
            monitorScreenObject->header.gfx.sharedChild = gLoadedGraphNodes[MODEL_MON];
            camera_mouse_x += gPlayer1Controller->rawStickX/15.0f;
            camera_mouse_y += gPlayer1Controller->rawStickY/15.0f;
            camera_mouse_selecting = FALSE;

            camera_mouse_x = CLAMP(camera_mouse_x,-190,0);
            camera_mouse_y = CLAMP(camera_mouse_y,-150,0);


            for (int i = 0; i < SECURITY_CAMERA_CT; i++) {
                if (securityCameras[i].name == NULL) {continue;}

                f32 xdif = (securityCameras[i].x - camera_mouse_x);
                f32 ydif = (securityCameras[i].y - camera_mouse_y);
                f32 distsqr = xdif*xdif + ydif*ydif;

                if (distsqr < 64.0f) {
                    camera_mouse_selecting = TRUE;
                    if (gPlayer1Controller->buttonPressed & A_BUTTON) {
                        fnab_cam_index = i;
                        fnab_cam_last_index = i;
                        camera_interference_timer = 10;
                    }
                }
            }

            // place sound
            if ((gPlayer1Controller->buttonPressed & Z_TRIG)&&(snd_timer==0)&&(breakerCharges[1]>0)) {
                breakerCharges[1]--;

                snd_x = -camera_mouse_x/10.0f;
                snd_y = -camera_mouse_y/10.0f;
                if (get_map_data(snd_x,snd_y)>0) {
                    snd_timer = 30;
                    for (int i = 0; i<ENEMY_COUNT; i++) {
                        struct fnabEnemy * ce = &enemyList[i];

                        if (!ce->active) {continue;}

                        f32 xdif = snd_x - ce->x;
                        f32 ydif = snd_y - ce->y;
                        f32 dsqrd = xdif*xdif + ydif*ydif;
                        if (dsqrd < 20.0f) {
                            ce->tx = snd_x;
                            ce->ty = snd_y;
                            ce->state = FNABE_DISTRACTED;
                        }
                    }
                } else {
                    //err
                }
            }

            //activate vent flush
            if ((gPlayer1Controller->buttonPressed & L_TRIG)&&(vent_flush_timer==0)&&(breakerCharges[2]>0)) {
                breakerCharges[2]--;
                vent_flush_timer = 200;

                for (int i = 0; i<ENEMY_COUNT; i++) {
                    struct fnabEnemy * ce = &enemyList[i];

                    if (!ce->active) {continue;}
                    if (get_map_data(ce->x,ce->y) == 2 || ce->state == FNABE_PRIMED_VENT) {
                        ce->state = FNABE_DISTRACTED;
                        ce->tx = ce->ventFlushX;
                        ce->ty = ce->ventFlushY;
                    }
                }
            }

            //activate radar
            if ((gPlayer1Controller->buttonPressed & R_TRIG)&&(radar_timer==0)&&(breakerCharges[2]>0)) {
                breakerCharges[2]--;
                radar_timer = 200;
            }

            if (gPlayer1Controller->buttonPressed & B_BUTTON) {
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
            if (fnab_office_statetimer > 10) {
                fnab_office_statetimer = 0;
                fnab_cam_index = 0;
                fnab_office_state = OFFICE_STATE_DESK;
            }
            break;
        case OFFICE_STATE_BREAKER:
            if (fnab_office_statetimer > 10) {

                if (!breakerDoFix) {
                    handle_menu_scrolling(MENU_SCROLL_VERTICAL, &breakerIndex, 0, 2);
                }

                if (!breakerDoFix&&(gPlayer1Controller->buttonPressed & A_BUTTON)) {
                    breakerDoFix = TRUE;
                    breakerFixing = 0.0f;
                }

                if (gPlayer1Controller->buttonPressed & B_BUTTON) {
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
            break;
    }
    fnab_office_statetimer++;

    if (camera_interference_timer > 0) {
        camera_interference_timer--;
    }

    darknessObject->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
    if (light_interference_timer > 0) {
        darknessObject->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
        if (light_interference_timer < 5 && (random_u16()%2==0)) {
            darknessObject->header.gfx.node.flags |= GRAPH_RENDER_INVISIBLE;
        }
        light_interference_timer --;
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
        breakerFixing += .03f;
        if (breakerFixing >= 7.f) {
            breakerDoFix = FALSE;
            breakerCharges[breakerIndex] = breakerChargesMax[breakerIndex];
        }
    }
}