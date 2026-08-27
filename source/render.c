#include "render.h"
#include "world.h"
#include "textures.h"
#include "player.h"
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <stdlib.h>
#include <string.h>

#define BUF_WIDTH 512
#define SCR_WIDTH 480
#define SCR_HEIGHT 272

typedef struct {
    float u, v;
    unsigned int color;
    float x, y, z;
} Vertex;

#define MAX_QUADS 20000
#define MAX_VERTS (MAX_QUADS * 6)
static Vertex g_mesh[MAX_VERTS] __attribute__((aligned(16)));
static int g_vertCount = 0;

static unsigned int __attribute__((aligned(16))) list[262144];

static void pushFace(float x, float y, float z, int face, int tile, unsigned char shade)
{
    if (g_vertCount + 6 > MAX_VERTS) return;
    float u0, v0, u1, v1;
    textures_getTileUV(tile, &u0, &v0, &u1, &v1);
    unsigned int col = (0xFF << 24) | (shade << 16) | (shade << 8) | shade;

    float x0=x, x1=x+1, y0=y, y1=y+1, z0=z, z1=z+1;
    Vertex v[4];

    switch (face) {
        case 0: /* +X */
            v[0]=(Vertex){u0,v1,col,x1,y0,z0}; v[1]=(Vertex){u1,v1,col,x1,y0,z1};
            v[2]=(Vertex){u1,v0,col,x1,y1,z1}; v[3]=(Vertex){u0,v0,col,x1,y1,z0};
            break;
        case 1: /* -X */
            v[0]=(Vertex){u0,v1,col,x0,y0,z1}; v[1]=(Vertex){u1,v1,col,x0,y0,z0};
            v[2]=(Vertex){u1,v0,col,x0,y1,z0}; v[3]=(Vertex){u0,v0,col,x0,y1,z1};
            break;
        case 2: /* +Y (top) */
            v[0]=(Vertex){u0,v1,col,x0,y1,z0}; v[1]=(Vertex){u1,v1,col,x1,y1,z0};
            v[2]=(Vertex){u1,v0,col,x1,y1,z1}; v[3]=(Vertex){u0,v0,col,x0,y1,z1};
            break;
        case 3: /* -Y (bottom) */
            v[0]=(Vertex){u0,v1,col,x0,y0,z1}; v[1]=(Vertex){u1,v1,col,x1,y0,z1};
            v[2]=(Vertex){u1,v0,col,x1,y0,z0}; v[3]=(Vertex){u0,v0,col,x0,y0,z0};
            break;
        case 4: /* +Z */
            v[0]=(Vertex){u0,v1,col,x1,y0,z1}; v[1]=(Vertex){u1,v1,col,x0,y0,z1};
            v[2]=(Vertex){u1,v0,col,x0,y1,z1}; v[3]=(Vertex){u0,v0,col,x1,y1,z1};
            break;
        default: /* -Z */
            v[0]=(Vertex){u0,v1,col,x0,y0,z0}; v[1]=(Vertex){u1,v1,col,x1,y0,z0};
            v[2]=(Vertex){u1,v0,col,x1,y1,z0}; v[3]=(Vertex){u0,v0,col,x0,y1,z0};
            break;
    }

    g_mesh[g_vertCount++] = v[0];
    g_mesh[g_vertCount++] = v[1];
    g_mesh[g_vertCount++] = v[2];
    g_mesh[g_vertCount++] = v[0];
    g_mesh[g_vertCount++] = v[2];
    g_mesh[g_vertCount++] = v[3];
}

static void getFaceTile(unsigned char block, int face, int *tile)
{
    switch (block) {
        case B_GRASS:
            if (face == 2) *tile = T_GRASS_TOP;
            else if (face == 3) *tile = T_DIRT;
            else *tile = T_GRASS_SIDE;
            break;
        case B_DIRT: *tile = T_DIRT; break;
        case B_STONE: *tile = T_STONE; break;
        case B_WOOD: *tile = (face == 2 || face == 3) ? T_WOOD_TOP : T_WOOD_SIDE; break;
        case B_LEAVES: *tile = T_LEAVES; break;
        case B_SAND: *tile = T_SAND; break;
        case B_WATER: *tile = T_WATER; break;
        default: *tile = T_BEDROCK; break;
    }
}

static void rebuildMesh(void)
{
    int x, y, z, f;
    static const int dx[6] = {1,-1,0,0,0,0};
    static const int dy[6] = {0,0,1,-1,0,0};
    static const int dz[6] = {0,0,0,0,1,-1};
    static const unsigned char shade[6] = {200,200,255,140,220,220};

    g_vertCount = 0;
    for (x = 0; x < WX; x++) {
        for (y = 0; y < WY; y++) {
            for (z = 0; z < WZ; z++) {
                unsigned char b = g_world[x][y][z];
                if (b == B_AIR) continue;
                for (f = 0; f < 6; f++) {
                    int nx = x+dx[f], ny = y+dy[f], nz = z+dz[f];
                    unsigned char nb = world_get(nx, ny, nz);
                    int neighborSolid = (nb != B_AIR);
                    if (b == B_WATER && nb == B_WATER) neighborSolid = 1;
                    if (!neighborSolid) {
                        int tile;
                        getFaceTile(b, f, &tile);
                        pushFace((float)x, (float)y, (float)z, f, tile, shade[f]);
                    }
                }
            }
        }
    }
    g_worldDirty = 0;
}

void render_init(void)
{
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CULL_FACE);
    sceGuClearColor(0xFFCC8855);
    sceGuClearDepth(0);
    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    textures_buildBlockAtlas();
    textures_buildSkinAtlas();
}

static void setupCamera(void)
{
    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(70.0f, (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 60.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumRotateX(-g_player.pitch * 0.0174532925f);
    sceGumRotateY(-g_player.yaw * 0.0174532925f);
    ScePspFVector3 trans = { -g_player.x, -(g_player.y + 1.5f), -g_player.z };
    sceGumTranslate(&trans);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void render_beginFrame(void)
{
    sceGuStart(GU_DIRECT, list);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
    setupCamera();
}

void render_drawWorld(void)
{
    if (g_worldDirty) rebuildMesh();

    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, ATLAS_SIZE, ATLAS_SIZE, ATLAS_SIZE, g_blockAtlas);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);

    sceGumDrawArray(GU_TRIANGLES,
        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        g_vertCount, 0, g_mesh);
}

void render_drawPlayerModel(void)
{
    sceGuTexMode(GU_PSM_8888, 0, 0, 0);
    sceGuTexImage(0, SKIN_ATLAS_W, SKIN_ATLAS_H, SKIN_ATLAS_W, g_skinAtlas);
    sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);

    float u0 = (float)(g_player.skin * TILE_SIZE) / (float)SKIN_ATLAS_W;
    float u1 = u0 + (float)TILE_SIZE / (float)SKIN_ATLAS_W;

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    ScePspFVector3 t = { 0.35f, -0.35f, -0.6f };
    sceGumTranslate(&t);
    ScePspFVector3 s = { 0.15f, 0.3f, 0.15f };
    sceGumScale(&s);

    unsigned int col = 0xFFFFFFFF;
    static Vertex hand[6];
    hand[0]=(Vertex){u0,1,col,0,0,0}; hand[1]=(Vertex){u1,1,col,1,0,0};
    hand[2]=(Vertex){u1,0,col,1,1,0}; hand[3]=(Vertex){u0,1,col,0,0,0};
    hand[4]=(Vertex){u1,0,col,1,1,0}; hand[5]=(Vertex){u0,0,col,0,1,0};

    sceGuDisable(GU_DEPTH_TEST);
    sceGumDrawArray(GU_TRIANGLES,
        GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
        6, 0, hand);
    sceGuEnable(GU_DEPTH_TEST);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}

void render_drawHUD(void)
{
    pspDebugScreenSetXY(0, 0);
    pspDebugScreenPrintf("VoxelCraft - Skin:%d", g_player.skin + 1);
    pspDebugScreenSetXY(0, 1);
    pspDebugScreenPrintf("D-Pad: look | Stick: move | X: jump");
    pspDebugScreenSetXY(0, 2);
    pspDebugScreenPrintf("L: break | R: place | Square: change block | Triangle: change skin");
}

void render_endFrame(void)
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}
