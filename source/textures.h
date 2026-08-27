#ifndef TEXTURES_H
#define TEXTURES_H

#include <pspgu.h>

#define ATLAS_SIZE 128   /* 128x128, شبكة 8x8 خانات مقاس 16x16 */
#define TILE_SIZE  16
#define TILES_PER_ROW (ATLAS_SIZE / TILE_SIZE)

/* الخانات داخل أطلس البلوكات */
enum {
    T_GRASS_TOP = 0,
    T_GRASS_SIDE,
    T_DIRT,
    T_STONE,
    T_WOOD_SIDE,
    T_WOOD_TOP,
    T_LEAVES,
    T_SAND,
    T_WATER,
    T_BEDROCK,
    T_COUNT
};

/* أطلس الشخصية (السكنات) - 3 سكنات بجانب بعض */
#define SKIN_ATLAS_W 64
#define SKIN_ATLAS_H 64
#define SKIN_COUNT 3

extern unsigned int g_blockAtlas[ATLAS_SIZE * ATLAS_SIZE] __attribute__((aligned(16)));
extern unsigned int g_skinAtlas[SKIN_ATLAS_W * SKIN_ATLAS_H] __attribute__((aligned(16)));

void textures_buildBlockAtlas(void);
void textures_buildSkinAtlas(void);
void textures_getTileUV(int tile, float *u0, float *v0, float *u1, float *v1);

#endif
