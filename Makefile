TARGET = voxelcraft
OBJS = source/main.o source/world.o source/render.o source/textures.o source/player.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -DPSP -Isource
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lpspgum -lpspgu -lm -lpspctrl -lpspdisplay -lpspge -lpspdebug -lpsphprm -lpspaudio

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = VoxelCraft
PSP_EBOOT_ICON =

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
