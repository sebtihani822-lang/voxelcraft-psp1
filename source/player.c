#include "player.h"
#include "world.h"
#include <pspctrl.h>
#include <math.h>

#define DEG2RAD 0.0174532925f
#define GRAVITY -18.0f
#define JUMP_SPEED 7.0f
#define MOVE_SPEED 5.0f
#define TURN_SPEED 90.0f   /* درجة/ثانية عبر D-Pad */

Player g_player;

static int prevButtons = 0;
static int prevCross = 0;

void player_init(void)
{
    g_player.x = WX / 2.0f;
    g_player.z = WZ / 2.0f;
    g_player.y = WY - 2.0f;
    g_player.yaw = 0.0f;
    g_player.pitch = 0.0f;
    g_player.vy = 0.0f;
    g_player.onGround = 0;
    g_player.skin = 0;
    g_player.selectedBlock = B_GRASS;
}

/* هل الصندوق المحيط باللاعب (نقطة واحدة تقريبية لعرض 0.6) يصطدم بشيء صلب؟ */
static int collidesAt(float x, float y, float z)
{
    float half = 0.3f;
    int minX = (int)floorf(x - half);
    int maxX = (int)floorf(x + half);
    int minZ = (int)floorf(z - half);
    int maxZ = (int)floorf(z + half);
    int minY = (int)floorf(y);
    int maxY = (int)floorf(y + 1.6f);
    int ix, iy, iz;
    for (ix = minX; ix <= maxX; ix++)
        for (iy = minY; iy <= maxY; iy++)
            for (iz = minZ; iz <= maxZ; iz++)
                if (world_isSolid(ix, iy, iz)) return 1;
    return 0;
}

/* راي كاست بسيط لإيجاد أقرب بلوك ينظر إليه اللاعب */
static int raycast(float dirx, float diry, float dirz, int *hx, int *hy, int *hz,
                    int *px, int *py, int *pz)
{
    float step = 0.08f;
    float t;
    int lastX = (int)floorf(g_player.x);
    int lastY = (int)floorf(g_player.y + 1.5f);
    int lastZ = (int)floorf(g_player.z);
    for (t = 0.0f; t < 5.0f; t += step) {
        float rx = g_player.x + dirx * t;
        float ry = g_player.y + 1.5f + diry * t;
        float rz = g_player.z + dirz * t;
        int bx = (int)floorf(rx);
        int by = (int)floorf(ry);
        int bz = (int)floorf(rz);
        if (world_isSolid(bx, by, bz)) {
            *hx = bx; *hy = by; *hz = bz;
            *px = lastX; *py = lastY; *pz = lastZ;
            return 1;
        }
        lastX = bx; lastY = by; lastZ = bz;
    }
    return 0;
}

void player_update(float dt)
{
    SceCtrlData pad;
    sceCtrlReadBufferPositive(&pad, 1);

    /* الكاميرا عبر D-Pad */
    if (pad.Buttons & PSP_CTRL_LEFT)  g_player.yaw   -= TURN_SPEED * dt;
    if (pad.Buttons & PSP_CTRL_RIGHT) g_player.yaw   += TURN_SPEED * dt;
    if (pad.Buttons & PSP_CTRL_UP)    g_player.pitch += TURN_SPEED * dt;
    if (pad.Buttons & PSP_CTRL_DOWN)  g_player.pitch -= TURN_SPEED * dt;
    if (g_player.pitch > 85.0f) g_player.pitch = 85.0f;
    if (g_player.pitch < -85.0f) g_player.pitch = -85.0f;

    float yawRad = g_player.yaw * DEG2RAD;
    float fwdX = sinf(yawRad);
    float fwdZ = -cosf(yawRad);
    float rightX = cosf(yawRad);
    float rightZ = sinf(yawRad);

    /* الحركة عبر العصا التناظرية */
    int lx = pad.Lx - 128;
    int ly = pad.Ly - 128;
    float mvX = 0.0f, mvZ = 0.0f;
    if (abs(lx) > 20 || abs(ly) > 20) {
        float fx = -ly / 128.0f;
        float fr = lx / 128.0f;
        mvX = (fwdX * fx + rightX * fr) * MOVE_SPEED;
        mvZ = (fwdZ * fx + rightZ * fr) * MOVE_SPEED;
    }

    float newX = g_player.x + mvX * dt;
    float newZ = g_player.z + mvZ * dt;
    if (!collidesAt(newX, g_player.y, g_player.z)) g_player.x = newX;
    if (!collidesAt(g_player.x, g_player.y, newZ)) g_player.z = newZ;

    /* الجاذبية والقفز */
    g_player.vy += GRAVITY * dt;
    float newY = g_player.y + g_player.vy * dt;
    if (collidesAt(g_player.x, newY, g_player.z)) {
        if (g_player.vy < 0) g_player.onGround = 1;
        g_player.vy = 0.0f;
    } else {
        g_player.y = newY;
        g_player.onGround = 0;
    }

    int crossNow = pad.Buttons & PSP_CTRL_CROSS;
    if (crossNow && !prevCross && g_player.onGround) {
        g_player.vy = JUMP_SPEED;
    }
    prevCross = crossNow;

    /* اتجاه النظر (approximate) لأجل الراي كاست */
    float pitchRad = g_player.pitch * DEG2RAD;
    float lookX = sinf(yawRad) * cosf(pitchRad);
    float lookY = sinf(pitchRad);
    float lookZ = -cosf(yawRad) * cosf(pitchRad);

    int hx, hy, hz, px, py, pz;
    int hit = raycast(lookX, lookY, lookZ, &hx, &hy, &hz, &px, &py, &pz);

    /* L = كسر بلوك , R = وضع بلوك (ضغطة واحدة في كل مرة) */
    int lNow = pad.Buttons & PSP_CTRL_LTRIGGER;
    int rNow = pad.Buttons & PSP_CTRL_RTRIGGER;
    int lPrev = prevButtons & PSP_CTRL_LTRIGGER;
    int rPrev = prevButtons & PSP_CTRL_RTRIGGER;

    if (hit) {
        if (lNow && !lPrev) {
            world_set(hx, hy, hz, B_AIR);
        }
        if (rNow && !rPrev) {
            world_set(px, py, pz, g_player.selectedBlock);
        }
    }

    /* تبديل السكن عبر Triangle */
    int triNow = pad.Buttons & PSP_CTRL_TRIANGLE;
    int triPrev = prevButtons & PSP_CTRL_TRIANGLE;
    if (triNow && !triPrev) {
        g_player.skin = (g_player.skin + 1) % 3;
    }

    /* تبديل نوع البلوك المختار عبر Square */
    int sqNow = pad.Buttons & PSP_CTRL_SQUARE;
    int sqPrev = prevButtons & PSP_CTRL_SQUARE;
    if (sqNow && !sqPrev) {
        g_player.selectedBlock++;
        if (g_player.selectedBlock >= B_COUNT) g_player.selectedBlock = B_GRASS;
        if (g_player.selectedBlock == B_BEDROCK) g_player.selectedBlock = B_GRASS;
    }

    prevButtons = pad.Buttons;
}
