void scroll_gfx_mat_mon_static() {
	Gfx *mat = segmented_to_virtual(mat_mon_static);

	shift_s(mat, 9, PACK_TILESIZE(0, 73));
	shift_t(mat, 9, PACK_TILESIZE(0, 21));

};

void scroll_actor_geo_mon() {
	scroll_gfx_mat_mon_static();
};
