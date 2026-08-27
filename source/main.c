#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include "world.h"
#include "render.h"
#include "player.h"

PSP_MODULE_INFO("VoxelCraft", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(-1); /* استعمال أكبر قدر ممكن من الذاكرة المتاحة */

static int exitRequest = 0;

int exitCallback(int arg1, int arg2, void *common)
{
    exitRequest = 1;
    return 0;
}

int callbackThread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

void setupCallbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) sceKernelStartThread(thid, 0, 0);
}

int main(void)
{
    setupCallbacks();

    pspDebugScreenInit();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    world_generate();
    player_init();
    render_init();

    while (!exitRequest) {
        float dt = 1.0f / 60.0f;

        player_update(dt);

        render_beginFrame();
        render_drawWorld();
        render_drawPlayerModel();
        render_endFrame();

        render_drawHUD();
    }

    sceKernelExitGame();
    return 0;
}
