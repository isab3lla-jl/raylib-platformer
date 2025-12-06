//Character Sprite Handler

#include "raylib.h"
#include <stdlib.h>

// ----------------------------------------------------------------------
// Types and Structures Definition
// ----------------------------------------------------------------------
typedef struct {
    Texture2D texture;
    int frameCount;
    int currentFrame;
    float frameSpeed;
    float timer;
    int frameWidth;
    int frameHeight;
    bool loops;
} Animation;

typedef enum {
    PLAYER_IDLE = 0,
    PLAYER_WALK,
    PLAYER_JUMP,
    PLAYER_SLIDE,
    PLAYER_HURT,
    PLAYER_DEATH
} PlayerState;

//Global Animations
Animation idle;
Animation walk;
Animation jump;
Animation wall_slide;
Animation hurt;
Animation death;

PlayerState playerState = PLAYER_IDLE;
bool facingRight = true;

// ----------------------------------------------------------------------
// Animation Functions
// ----------------------------------------------------------------------
void LoadPlayerAnimations()
{

    // -------- IDLE --------
    idle.texture = LoadTexture("resources/character/idle.png");
    idle.frameCount = 4;
    idle.frameSpeed = 8;
    idle.currentFrame = 0;
    idle.timer = 0;
    idle.frameWidth = idle.texture.width / idle.frameCount;
    idle.frameHeight = idle.texture.height;
    idle.loops = true;

    // -------- WALK --------
    walk.texture = LoadTexture("resources/character/walk.png");
    walk.frameCount = 6;
    walk.frameSpeed = 10;
    walk.currentFrame = 0;
    walk.timer = 0;
    walk.frameWidth = walk.texture.width / walk.frameCount;
    walk.frameHeight = walk.texture.height;
    walk.loops = true;

    // -------- JUMP --------
    jump.texture = LoadTexture("resources/character/jump.png");
    jump.frameCount = 8;
    jump.frameSpeed = 12;
    jump.currentFrame = 0;
    jump.timer = 0;
    jump.frameWidth = jump.texture.width / jump.frameCount;
    jump.frameHeight = jump.texture.height;
    jump.loops = true;
    
    // -------- WALL SLIDE --------
    wall_slide.texture = LoadTexture("resources/character/wall_slide.png");
    wall_slide.frameCount = 6;
    wall_slide.frameSpeed = 10;
    wall_slide.currentFrame = 0;
    wall_slide.timer = 0;
    wall_slide.frameWidth = wall_slide.texture.width / wall_slide.frameCount;
    wall_slide.frameHeight = wall_slide.texture.height;
    wall_slide.loops = true;
    
    // -------- HURT --------
    hurt.texture = LoadTexture("resources/character/hurt.png");
    hurt.frameCount = 4;
    hurt.frameSpeed = 8;
    hurt.currentFrame = 0;
    hurt.timer = 0;
    hurt.frameWidth = hurt.texture.width / hurt.frameCount;
    hurt.frameHeight = hurt.texture.height;
    hurt.loops = true;
    
    // -------- DEATH --------
    death.texture = LoadTexture("resources/character/death.png");
    death.frameCount = 8;
    death.frameSpeed = 12;
    death.currentFrame = 0;
    death.timer = 0;
    death.frameWidth = death.texture.width / death.frameCount;
    death.frameHeight = death.texture.height;
    death.loops = false;
}
void UnloadPlayerAnimations()
{
    UnloadTexture(idle.texture);
    UnloadTexture(walk.texture);
    UnloadTexture(jump.texture);
    UnloadTexture(wall_slide.texture);
    UnloadTexture(hurt.texture);
    UnloadTexture(death.texture);
}
void UpdateAnimation(Animation *anim, float delta)
{

    if (!anim->loops && anim->currentFrame == anim->frameCount - 1) {
        return; 
    }

    anim->timer += delta;

    if (anim->timer >= 1.0f / anim->frameSpeed) {
        anim->timer = 0;
        anim->currentFrame++;

        if (anim->currentFrame >= anim->frameCount) {
            if (anim->loops) {
                anim->currentFrame = 0; // Loop
            } else {
                anim->currentFrame = anim->frameCount - 1; // Bloquear en el último frame
            }
        }
    }
}
void ResetAnimation(Animation *anim)
{
    anim->currentFrame = 0;
    anim->timer = 0;
}
bool IsAnimationFinished(Animation *anim) //Death animation
{
    return (anim->currentFrame == anim->frameCount - 1);
}
void DrawAnimation(Animation *anim, Vector2 pos)
{
    Rectangle src = {
        anim->frameWidth * anim->currentFrame,
        0,
        anim->frameWidth,
        anim->frameHeight
    };

    if (!facingRight)
        src.width = -anim->frameWidth;

    Rectangle dest = {
        pos.x - anim->frameWidth/2,
        pos.y - anim->frameHeight,
        anim->frameWidth,
        anim->frameHeight
    };

    DrawTexturePro(anim->texture, src, dest, (Vector2){0,0}, 0, WHITE);
}
void DrawPlayer(Vector2 pos)
{
    Animation *animToDraw = NULL;

    switch (playerState) {
        case PLAYER_IDLE:
            animToDraw = &idle;
            break;
        case PLAYER_WALK:
            animToDraw = &walk;
            break;
        case PLAYER_JUMP:
            if (playerState == PLAYER_HURT) {
                animToDraw = &hurt;
            } else {
                animToDraw = &jump;
            }
            break;
        case PLAYER_SLIDE:
            animToDraw = &wall_slide;
            break;
        case PLAYER_HURT:
            animToDraw = &hurt;
            break;
        case PLAYER_DEATH:
            animToDraw = &death;
            break;
    }

    Rectangle src = {
        animToDraw->frameWidth * animToDraw->currentFrame,
        0,
        animToDraw->frameWidth,
        animToDraw->frameHeight
    };

    if (!facingRight)
        src.width = -animToDraw->frameWidth;

    Rectangle dest = {
        pos.x - animToDraw->frameWidth / 2,
        pos.y - animToDraw->frameHeight,
        animToDraw->frameWidth,
        animToDraw->frameHeight
    };

    DrawTexturePro(animToDraw->texture, src, dest, (Vector2){0,0}, 0.0f, WHITE);
}