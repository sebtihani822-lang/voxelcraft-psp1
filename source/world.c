#include "world.h"
#include <math.h>
#include <stdlib.h>

unsigned char g_world[WX][WY][WZ];
int g_worldDirty = 1;

int world_inBounds(int x, int y, int z) {
    return (x >= 0 && x < WX && y >= 0 && y < WY && z >= 0 && z < WZ);
}

unsigned char world_get(int x, int y, int z) {
    if (!world_inBounds(x, y, z)) return B_AIR;
    return g_world[x][y][z];
}

void world_set(int x, int y, int z, unsigned char block) {
    if (!world_inBounds(x, y, z)) return;
    g_world[x][y][z] = block;
    g_worldDirty = 1;
}

int world_isSolid(int x, int y, int z) {
    unsigned char b = world_get(x, y, z);
    return (b != B_AIR && b != B_WATER);
}

/* مولد أرقام عشوائي بسيط وثابت (بدون الحاجة لمكتبات خارجية) */
static unsigned int rseed = 987654321u;
static unsigned int lrand(void) {
    rseed = rseed * 1103515245u + 12345u;
    return (rseed >> 16) & 0x7fff;
}

static void placeTree(int x, int gy, int z) {
    int trunkH = 3 + (lrand() % 2);
    int h;
    for (h = 0; h < trunkH; h++) {
        world_set(x, gy + h, z, B_WOOD);
    }
    int top = gy + trunkH;
    int dx, dz, dy;
    for (dy = -1; dy <= 1; dy++) {
        for (dx = -2; dx <= 2; dx++) {
            for (dz = -2; dz <= 2; dz++) {
                if (abs(dx) == 2 && abs(dz) == 2) continue;
                int lx = x + dx, ly = top + dy, lz = z + dz;
                if (world_get(lx, ly, lz) == B_AIR)
                    world_set(lx, ly, lz, B_LEAVES);
            }
        }
    }
    world_set(x, top + 2, z, B_LEAVES);
}

void world_generate(void) {
    int x, y, z;

    for (x = 0; x < WX; x++) {
        for (z = 0; z < WZ; z++) {
            for (y = 0; y < WY; y++) {
                g_world[x][y][z] = B_AIR;
            }
        }
    }

    for (x = 0; x < WX; x++) {
        for (z = 0; z < WZ; z++) {
            float fx = (float)x;
            float fz = (float)z;
            int height = 12 + (int)(3.0f * sinf(fx * 0.35f) +
                                     3.0f * cosf(fz * 0.28f) +
                                     1.5f * sinf((fx + fz) * 0.18f));
            if (height < 3) height = 3;
            if (height > WY - 6) height = WY - 6;

            for (y = 0; y <= height; y++) {
                unsigned char b;
                if (y == 0) b = B_BEDROCK;
                else if (y < height - 4) b = B_STONE;
                else if (y < height) b = B_DIRT;
                else b = (height < SEA_LEVEL + 1) ? B_SAND : B_GRASS;
                g_world[x][y][z] = b;
            }

            if (height < SEA_LEVEL) {
                for (y = height + 1; y <= SEA_LEVEL; y++) {
                    g_world[x][y][z] = B_WATER;
                }
            }
        }
    }

    /* زرع بعض الأشجار فوق العشب */
    for (x = 3; x < WX - 3; x += 1) {
        for (z = 3; z < WZ - 3; z += 1) {
            if ((lrand() % 100) < 4) {
                /* ابحث عن أعلى بلوك عشب في هذا العمود */
                int y;
                for (y = WY - 1; y > 0; y--) {
                    if (g_world[x][y][z] == B_GRASS) {
                        placeTree(x, y + 1, z);
                        break;
                    }
                }
            }
        }
    }

    g_worldDirty = 1;
}
