#include "src/game/envfx_snow.h"

const GeoLayout fnabdoor_geo[] = {
	GEO_CULLING_RADIUS(1000),
	GEO_OPEN_NODE(),
		GEO_DISPLAY_LIST(LAYER_OPAQUE, fnabdoor_fnabdoor_mesh_layer_1),
	GEO_CLOSE_NODE(),
	GEO_END(),
};
