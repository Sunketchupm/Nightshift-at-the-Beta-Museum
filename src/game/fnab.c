#include "types.h"

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