#ifndef PS1_GFX_H
#define PS1_GFX_H
#include <sys/types.h>
#include <psxgpu.h>
#include <psxgte.h>
#include "sim/fx.h"

#define OT_LEN 1024
#define GFX_FOCAL 200
#define GFX_RGB(r, g, b) ((uint32_t)(r) << 16 | (uint32_t)(g) << 8 | (uint32_t)(b))
#define SKY_HORIZON GFX_RGB(176, 72, 132)

void gfx_init(void);
void gfx_begin(void);
void gfx_end(void);
int gfx_fps(void);
/* Hblanks used by the last frame's CPU work; about 262 per vsync. */
int gfx_cpu_hblanks(void);

void gfx_set_camera(const Vec3 *eye, const Vec3 *target);
const Vec3 *gfx_eye(void);
const Vec3 *gfx_forward(void);

/* Space for one primitive in this frame's packet buffer, or 0 if it is full. */
void *gfx_alloc(size_t size);
uint32_t *gfx_ot(int index);

/* Transforms n points; sxy holds y<<16|x, sz is depth at 256 units per metre (0 = behind). */
void gfx_transform(const Vec3 *world, int n, int *sxy, int *sz);
/* Projects a direction at infinity. Returns 0 if it is behind the camera. */
int gfx_project_dir(const Vec3 *dir, int *x, int *y);
/* True when four projected corners are in front, on screen and within GPU size limits. */
int gfx_quad_ok(const int *sxy, const int *sz);
int gfx_depth_index(int avg_sz);
/* Flat quad in z-pattern order; back faces are culled when cull is set. */
void gfx_quad(const Vec3 p[4], uint32_t rgb, int cull);
/* Added to gfx_quad depth indices; negative draws nearer. */
void gfx_set_depth_bias(int bias);

/* Gouraud quad given in perimeter order, clipped against a 1.5 m near plane. */
void gfx_quad_clipped(const Vec3 p[4], const uint32_t c[4]);
#define GFX_CLIP_SZ 384 /* corners nearer than this (1.5 m) need gfx_quad_clipped */

void gfx_text(int x, int y, const char *s, uint32_t rgb);
void gfx_text_scaled(int x, int y, const char *s, uint32_t rgb, int scale);

#endif
