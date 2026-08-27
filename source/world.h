#ifndef WORLD_H
#define WORLD_H

/* أبعاد العالم (يمكن تكبيرها لاحقا اذا سمحت الذاكرة) */
#define WX 32
#define WY 32
#define WZ 32
#define SEA_LEVEL 10

/* أنواع البلوكات */
enum {
    B_AIR = 0,
    B_GRASS,
    B_DIRT,
    B_STONE,
    B_WOOD,
    B_LEAVES,
    B_SAND,
    B_WATER,
    B_BEDROCK,
    B_COUNT
};

extern unsigned char g_world[WX][WY][WZ];
extern int g_worldDirty;

void world_generate(void);
int  world_inBounds(int x, int y, int z);
unsigned char world_get(int x, int y, int z);
void world_set(int x, int y, int z, unsigned char block);
int  world_isSolid(int x, int y, int z);

#endif
