#include "types.h"
#include "fnab.h"

#define _ 0, // Wall / Nothing
#define F 1, // Floor  
#define V 2, // Vent

u8 fnabMap[20][20] = {
{F F F F _ F F F _ F F F _ _ _ _ _ _ _ _},
{F F F F _ F F F _ F F F _ F _ F _ _ _ _},
{F F F F _ _ F F _ F F _ _ F F F F _ _ _},
{F F F F V V F F F F F V V _ _ _ F _ _ _},
{F F F F _ _ F _ _ F _ _ F F F F F _ _ _},
{F F F F _ _ F _ F F F _ F _ _ _ F _ _ _},
{F F F F F F F F F F F F F _ _ _ F _ _ _},
{F _ _ _ _ F _ _ F F F _ F _ _ _ F _ _ _},
{F F F _ F F F _ _ _ _ _ F _ _ _ F _ _ _},
{F F F _ F F F _ _ _ _ _ F _ _ _ F _ _ _},
{F F F _ _ V _ _ _ F F F F _ _ _ F _ _ _},
{_ _ _ _ _ V _ _ _ F _ _ _ F F F F _ _ _},
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

#define MAP_SIZE 20
#define PATH_BRANCH_STACK_CT 30

s8 dirOffset[4][2] = {
    {0,-1},
    {1,0},
    {0,1},
    {-1,0}
};

u8 pathfindingMap[MAP_SIZE][MAP_SIZE];
// Path branches prevent the need for a recursive function
struct pathBranch pathBranchStack[PATH_BRANCH_STACK_CT];
u8 pathBranchCount = 0;

u8 get_map_data(u8 x, u8 y) {
    if ((x<MAP_SIZE&&x>=0)&&(y<MAP_SIZE&&y>=0)) {
        return pathfindingMap[y][x];
    }
    return 0;
}

void add_path_branch(u8 x, u8 y) {
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

u8 path_find(u8 xs, u8 ys, u8 xd, u8 yd) {
    bcopy(&fnabMap,&pathfindingMap,MAP_SIZE*MAP_SIZE);
    bzero(&pathBranchStack,sizeof(struct pathBranch)*PATH_BRANCH_STACK_CT);

    pathBranchCount = 0;
    add_path_branch(xs,ys);

    // Flood fill from start to destination
    while(pathBranchCount > 0) {
        for (int i = 0; i < PATH_BRANCH_STACK_CT; i++) {
            struct pathBranch * cpb = &pathBranchStack[i];
            if (cpb->active) {
                u8 x = cpb->x; 
                u8 y = cpb->y;
                for (int d = 0; d < 4; d++) {
                    if (x == xd+dirOffset[d][0] && y == yd+dirOffset[d][1]) {
                        return d;
                    }
                    if (get_map_data(x+dirOffset[d][0],y+dirOffset[d][1]) > 0) {
                        add_path_branch(x+dirOffset[d][0],y+dirOffset[d][1]);
                    }
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