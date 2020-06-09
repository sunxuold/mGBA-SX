
extern void gba_render_half(uint16_t* Src, uint32_t DestX, uint32_t DestY, uint32_t SrcPitch, uint32_t DestPitch);	

static inline void refreshscreen();
static inline void gba_upscale_aspect(uint16_t *to, uint16_t *from,uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
static inline void gba_upscale_aspect_subpixel(uint16_t *to, uint16_t *from, uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
static inline void gba_upscale_aspect_bilinear(uint16_t *to, uint16_t *from,uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch);
static inline void scale166x_fast(uint32_t* dst, uint32_t* src);	
static inline void scale166x_pseudobilinear(uint32_t* dst, uint32_t* src);


