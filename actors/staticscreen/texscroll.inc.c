void scroll_gfx_mat_staticscreen_static() {
	Gfx *mat = segmented_to_virtual(mat_staticscreen_static);

	shift_s(mat, 10, PACK_TILESIZE(0, 73));
	shift_t(mat, 10, PACK_TILESIZE(0, 24));

};

void scroll_actor_dl_staticscreen() {
	scroll_gfx_mat_staticscreen_static();
};
