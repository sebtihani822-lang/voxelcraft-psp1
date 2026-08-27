#ifndef PLAYER_H
#define PLAYER_H

typedef struct {
    float x, y, z;
    float yaw, pitch;
    float vy;
    int onGround;
    int skin;          /* 0..2 : السكن المختار */
    int selectedBlock;  /* البلوك المختار للوضع */
} Player;

extern Player g_player;

void player_init(void);
void player_update(float dt);

#endif
