#include <ultra64.h>
#include "sm64.h"
#include "surface_terrains.h"
#include "geo_commands.h"

#include "make_const_nonconst.h"

// Note: This bin does not use bin IDs, unlike the other segmented bins.
#include "mario/model.inc.c"

#include "bubble/model.inc.c"

#include "walk_smoke/model.inc.c"

#include "burn_smoke/model.inc.c"

#include "small_water_splash/model.inc.c"

#include "water_wave/model.inc.c"

#include "sparkle/model.inc.c"

#include "water_splash/model.inc.c"

#include "white_particle_small/model.inc.c"

#include "sparkle_animation/model.inc.c"

#ifdef S2DEX_TEXT_ENGINE
#include "src/s2d_engine/s2d_config.h"
#include FONT_C_FILE
#endif

#include "fanblade/model.inc.c"
#include "staticscreen/model.inc.c"
#include "cammap/model.inc.c"
#include "mouse2/model.inc.c"
#include "mouse1/model.inc.c"
#include "cb/model.inc.c"
#include "darkness/model.inc.c"
#include "snd/model.inc.c"
#include "radar/model.inc.c"
#include "moff/model.inc.c"
#include "mon/model.inc.c"
#include "warioapp/model.inc.c"
#include "warioapp/anims/data.inc.c"
#include "warioapp/anims/table.inc.c"
#include "fnabdoor/model.inc.c"
#include "dr/model.inc.c"
#include "luigi/model.inc.c"
#include "luigi/anims/data.inc.c"
#include "luigi/anims/table.inc.c"
#include "stanley/model.inc.c"
#include "stanley/anims/data.inc.c"
#include "stanley/anims/table.inc.c"
#include "cn/model.inc.c"
#include "cne/model.inc.c"
#include "cambtn/model.inc.c"
#include "cambtn2/model.inc.c"