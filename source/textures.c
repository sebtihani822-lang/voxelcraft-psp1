#include "textures.h"
#include <stdlib.h>

unsigned int g_blockAtlas[ATLAS_SIZE * ATLAS_SIZE] __attribute__((aligned(16)));
unsigned int g_skinAtlas[SKIN_ATLAS_W * SKIN_ATLAS_H] __attribute__((aligned(16)));

/* اللون بصيغة 0xAABBGGRR (ترتيب القنوات المستعمل غالبا مع GU_COLOR_8888 على PSP) */
#define RGBA(r,g,b,a) ( ((unsigned int)(a)<<24) | ((unsigned int)(b)<<16) | ((unsigned int)(g)<<8) | (unsigned int)(r) )

static unsigned int rseed2 = 13371337u;
static unsigned int qrand(void) {
    rseed2 = rseed2 * 1103515245u + 12345u;
    return (rseed2 >> 16) & 0x7fff;
}

static void atlasPut(int tileX, int tileY, int px, int py, unsigned int color) {
    int x = tileX * TILE_SIZE + px;
    int y = tileY * TILE_SIZE + py;
    if (x < 0 || x >= ATLAS_SIZE || y < 0 || y >= ATLAS_SIZE) return;
    g_blockAtlas[y * ATLAS_SIZE + x] = color;
}

static void fillTile(int tileIdx, unsigned int base) {
    int tx = tileIdx % TILES_PER_ROW;
    int ty = tileIdx / TILES_PER_ROW;
    int x, y;
    for (y = 0; y < TILE_SIZE; y++)
        for (x = 0; x < TILE_SIZE; x++)
            atlasPut(tx, ty, x, y, base);
}

static void speckleTile(int tileIdx, unsigned int base, unsigned int spot, int chance) {
    fillTile(tileIdx, base);
    int tx = tileIdx % TILES_PER_ROW;
    int ty = tileIdx / TILES_PER_ROW;
    int x, y;
    for (y = 0; y < TILE_SIZE; y++)
        for (x = 0; x < TILE_SIZE; x++)
            if ((int)(qrand() % 100) < chance)
                atlasPut(tx, ty, x, y, spot);
}

void textures_buildBlockAtlas(void)
{
    unsigned int c;
    int tx, ty, x, y;

    /* عشب - الأعلى */
    speckleTile(T_GRASS_TOP, RGBA(86,170,60,255), RGBA(70,150,48,255), 22);

    /* عشب - الجانب (تراب تحت وشريط عشب فوق) */
    fillTile(T_GRASS_SIDE, RGBA(134,96,67,255));
    tx = T_GRASS_SIDE % TILES_PER_ROW; ty = T_GRASS_SIDE / TILES_PER_ROW;
    for (x = 0; x < TILE_SIZE; x++) {
        for (y = 0; y < 4; y++) atlasPut(tx, ty, x, y, RGBA(86,170,60,255));
        atlasPut(tx, ty, x, 4, RGBA(70,150,48,255));
    }

    /* تراب */
    speckleTile(T_DIRT, RGBA(134,96,67,255), RGBA(112,80,56,255), 18);

    /* حجر */
    speckleTile(T_STONE, RGBA(128,128,128,255), RGBA(105,105,105,255), 25);

    /* خشب - الجانب (خطوط رأسية) */
    fillTile(T_WOOD_SIDE, RGBA(115,84,53,255));
    tx = T_WOOD_SIDE % TILES_PER_ROW; ty = T_WOOD_SIDE / TILES_PER_ROW;
    for (x = 0; x < TILE_SIZE; x++)
        for (y = 0; y < TILE_SIZE; y++)
            if ((x % 4) == 0)
                atlasPut(tx, ty, x, y, RGBA(90,64,38,255));

    /* خشب - الأعلى (حلقات) */
    fillTile(T_WOOD_TOP, RGBA(150,112,72,255));
    tx = T_WOOD_TOP % TILES_PER_ROW; ty = T_WOOD_TOP / TILES_PER_ROW;
    for (x = 0; x < TILE_SIZE; x++)
        for (y = 0; y < TILE_SIZE; y++) {
            int dx = x - 8, dy = y - 8;
            int d = dx*dx + dy*dy;
            if ((d/6) % 2 == 0) atlasPut(tx, ty, x, y, RGBA(115,84,53,255));
        }

    /* أوراق شجر */
    speckleTile(T_LEAVES, RGBA(58,120,42,255), RGBA(40,95,30,255), 30);

    /* رمل */
    speckleTile(T_SAND, RGBA(219,201,146,255), RGBA(200,182,128,255), 15);

    /* ماء (شفاف قليلا) */
    fillTile(T_WATER, RGBA(64,120,200,180));

    /* صخر القاعدة */
    speckleTile(T_BEDROCK, RGBA(50,50,55,255), RGBA(30,30,34,255), 35);

    (void)c;
}

/* رسم مربع صغير داخل أطلس السكنات */
static void skinPut(int x, int y, unsigned int color) {
    if (x < 0 || x >= SKIN_ATLAS_W || y < 0 || y >= SKIN_ATLAS_H) return;
    g_skinAtlas[y * SKIN_ATLAS_W + x] = color;
}

/* كل سكن يحتل عمود بعرض 16 بكسل داخل الأطلس (0..15 / 16..31 / 32..47) */
static void buildOneSkin(int skinIndex, unsigned int skinColor, unsigned int shirtColor, unsigned int trimColor) {
    int baseX = skinIndex * TILE_SIZE;
    int x, y;
    for (y = 0; y < SKIN_ATLAS_H; y++) {
        for (x = 0; x < TILE_SIZE; x++) {
            unsigned int c;
            if (y < 6) {
                /* رأس */
                c = skinColor;
                if ((x == 4 || x == 11) && (y == 2 || y == 3)) c = RGBA(20,20,20,255); /* عينين */
            } else if (y < 12) {
                /* جسم */
                c = shirtColor;
                if (x == 0 || x == 15) c = trimColor;
            } else {
                /* أرجل */
                c = (x < 8) ? RGBA(40,40,60,255) : RGBA(50,50,72,255);
            }
            skinPut(baseX + x, y, c);
        }
    }
}

void textures_buildSkinAtlas(void)
{
    /* ثلاث سكنات أصلية بألوان مختلفة (فارس أحمر / مستكشف أزرق / حارس أخضر) */
    buildOneSkin(0, RGBA(235,195,160,255), RGBA(180,40,40,255),  RGBA(120,20,20,255));
    buildOneSkin(1, RGBA(235,195,160,255), RGBA(40,90,200,255), RGBA(20,50,140,255));
    buildOneSkin(2, RGBA(235,195,160,255), RGBA(50,150,70,255), RGBA(25,100,45,255));
}

void textures_getTileUV(int tile, float *u0, float *v0, float *u1, float *v1)
{
    int tx = tile % TILES_PER_ROW;
    int ty = tile / TILES_PER_ROW;
    *u0 = (float)(tx * TILE_SIZE) / (float)ATLAS_SIZE;
    *v0 = (float)(ty * TILE_SIZE) / (float)ATLAS_SIZE;
    *u1 = (float)((tx + 1) * TILE_SIZE) / (float)ATLAS_SIZE;
    *v1 = (float)((ty + 1) * TILE_SIZE) / (float)ATLAS_SIZE;
}
