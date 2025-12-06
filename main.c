#include "raylib.h"
#include "raymath.h"
#include "animations.c"

#define GRAVITY 850.0f
#define PLAYER_JUMP_SPD 500.0f
#define PLAYER_HOR_SPD 200.0f
#define MAX_ENVIRONMENT_ELEMENTS 30
#define MAX_DANGER_ELEMENTS 10


#define PLAYER_WALL_SLIDE_SPD 75.0f
#define PLAYER_WALL_JUMP_X_SPD 350.0f
#define PLAYER_WALL_JUMP_DURATION 0.15f 

// Variables Globales necesarias para el estado y la animación
Texture2D tilemapTexture;

// ----------------------------------------------------------------------------------
// Types and Structures Definition
// ----------------------------------------------------------------------------------
typedef struct Player {
    Vector2 position;
    float speed;
    float speed_x;
    float wallJumpTimer;
    bool canJump;
    float width;
    float height;
    int lives;
    float hurtTimer;
} Player;

typedef struct EnvElement {
    Rectangle rect;
    int blocking;
    Color color;
    Rectangle tileSourceRec;
} EnvElement;

typedef struct Danger {
    Rectangle rect;
    int blocking;
    Color color;
    Rectangle tileSourceRec;
} Danger;

//Declaring Functions
Rectangle GetPlayerHitbox(Player player);
void LoadResources(void);
void UnloadResources(void);
void ResetPlayer(Player *player);


// ------------------------------------------------------------------------------------
// Program main entry point
// ------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    // --------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib - platformer");

    // Define player
    Player player = { 0 };
    player.position = (Vector2){ 400, 280 };
    player.speed = 0;
    player.speed_x = 0;
    player.wallJumpTimer = 0;
    player.canJump = false;
    player.width = 24.0f; 
    player.height = 32.0f;
    player.lives = 5;
    player.hurtTimer = 0.0f;

    //Tiles { 10, 20, 10, 10 }
    Rectangle tile_safe = { 70, 60, 10, 10 }; //safe
    Rectangle tile_danger = { 70, 50, 10, 10 }; //danger
    Rectangle tile_walls = { 230, 190, 10, 10 }; //walls


    // Define environment elements (platforms)
    EnvElement envElements[MAX_ENVIRONMENT_ELEMENTS] = {
        {{ 0, 0, 1000, 1000 }, 0, DARKBROWN, tile_walls}, //background
        {{ 0, -400, 200, 1000 }, 1, GRAY, tile_walls }, //left wall
        {{ 800, -400, 200, 1000 }, 1, GRAY, tile_walls }, //right wall
        {{ 0, 400, 1000, 200 }, 1, GRAY, tile_walls }, //floor
        {{ 0, -400, 1000, 100 }, 1, GRAY, tile_walls }, //roof
        {{ 300, 200, 150, 10 }, 1, GRAY, tile_safe },
        {{ 600, 200, 150, 10 }, 1, GRAY, tile_safe },
        {{ 250, 300, 100, 10 }, 1, GRAY, tile_safe },
        {{ 300, 30, 150, 10 }, 1, GRAY, tile_safe },
        {{ 650, 300, 100, 10 }, 1, GRAY, tile_safe }
    };
    
    // Define los elementos de Peligro (spikes, lava, etc.)
    Danger dangerElements[MAX_DANGER_ELEMENTS] = {
        {{ 450, 200, 150, 10 }, 0, RED, tile_danger }, 
        { 0 }
    };

    // Define camera
    Camera2D camera = { 0 };
    camera.target = player.position;
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f; 

    SetTargetFPS(60);

    LoadResources();
    // --------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        // ----------------------------------------------------------------------------------
        float deltaTime = GetFrameTime();

        // 1. HURT TIMER LOGIC 
        if (player.hurtTimer > 0) {
            player.hurtTimer -= deltaTime;
            if (player.hurtTimer <= 0) {
                ResetPlayer(&player);
            }
            playerState = PLAYER_HURT;
            player.speed_x = 0; 
        }
        
        //Wall Jump Variables
        bool hitWall = false;
        int wallSide = 0; 

        player.speed += GRAVITY * deltaTime;
        if (player.wallJumpTimer > 0) player.wallJumpTimer -= deltaTime;

        float target_speed_x = 0;
        PlayerState newState = playerState; 
        
        // 2. INPUT Y CÁLCULO DE VELOCIDAD HORIZONTAL (SOLO SI NO ESTÁ HURT)
        if (player.hurtTimer <= 0) {
            if (player.wallJumpTimer <= 0) {
                if (IsKeyDown(KEY_LEFT)) {
                    target_speed_x = -PLAYER_HOR_SPD;
                    facingRight = false;
                    newState = PLAYER_WALK;
                }
                else if (IsKeyDown(KEY_RIGHT)) {
                    target_speed_x = PLAYER_HOR_SPD;
                    facingRight = true;
                    newState = PLAYER_WALK;
                }
                
                player.speed_x = target_speed_x;
            }
        } 


        // Movement
        float old_player_x = player.position.x; 
        player.position.x += player.speed_x * deltaTime; 

        //Death logic
        if (player.lives == 0) {
            playerState = PLAYER_DEATH;
            ResetAnimation(&death);
        }

        // Collisions (Horizontal)
        Rectangle currentHitbox = GetPlayerHitbox(player);

        for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
        {
            EnvElement *element = &envElements[i];

            if (element->blocking && CheckCollisionRecs(currentHitbox, element->rect))
            {

                if (player.position.x > old_player_x)
                {
                    player.position.x = element->rect.x - player.width / 2.0f;
                    wallSide = 1;
                }
                else if (player.position.x < old_player_x)
                {
                    player.position.x = element->rect.x + element->rect.width + player.width / 2.0f;
                    wallSide = -1;
                }
                
                player.speed_x = 0;
                player.wallJumpTimer = 0;
                hitWall = true;

                currentHitbox = GetPlayerHitbox(player);
            }
        }
        
        // 3. COLISIÓN CON PELIGRO (ACTIVA EL TIMER)
        for (int i = 0; i < MAX_DANGER_ELEMENTS; i++) {
            Danger *danger = &dangerElements[i];

            if (player.hurtTimer <= 0 && CheckCollisionRecs(currentHitbox, danger->rect))
            {
                if (player.lives > 0)
                {
                    playerState = PLAYER_HURT;
                    player.hurtTimer = 0.5f; 
                    // ResetAnimation(&hurt);
                    break; 
                }
                else if (playerState != PLAYER_DEATH)
                {
                    playerState = PLAYER_DEATH;
                    ResetAnimation(&death);
                }
            }
        }

        //Wall Jump Logic (SOLO SI NO ESTÁ HURT)
        if (player.hurtTimer <= 0 && IsKeyPressed(KEY_SPACE)) 
        {
            if (player.canJump)
            {
                //Normal Jump
                player.speed = -PLAYER_JUMP_SPD;
                player.canJump = false;
            }
            else if (hitWall && (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT))) 
            {
                player.speed = -PLAYER_JUMP_SPD; 
                
                float jumpDirection = (float)-wallSide;
                
                player.speed_x = jumpDirection * PLAYER_WALL_JUMP_X_SPD; 
                player.wallJumpTimer = PLAYER_WALL_JUMP_DURATION;
                
                hitWall = false; 
                wallSide = 0;
            }
        }
        
        //Wall SLide Logic
        if (hitWall && player.speed > 0)
        {
            bool pressingIntoWall = (wallSide == 1 && IsKeyDown(KEY_RIGHT)) || (wallSide == -1 && IsKeyDown(KEY_LEFT));
            
            if (pressingIntoWall && player.speed > PLAYER_WALL_SLIDE_SPD)
            {
                player.speed = PLAYER_WALL_SLIDE_SPD;
                if (player.hurtTimer <= 0) newState = PLAYER_SLIDE;
            }
        }
        
        //Vertical Movement
        const int SUB_STEPS = 4;
        float subDeltaTime = deltaTime / SUB_STEPS;
        int hitObstacle = 0; 

        for (int step = 0; step < SUB_STEPS; step++) {
            player.position.y += player.speed * subDeltaTime;
            Rectangle playerHitbox = GetPlayerHitbox(player); 

            for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++) {
                EnvElement *element = &envElements[i];

                if (element->blocking && CheckCollisionRecs(playerHitbox, element->rect)) {

                    if (player.speed >= 0) 
                    {
                        hitObstacle = 1;
                        player.speed = 0.0f;
                        player.position.y = element->rect.y;
                        break; 
                    }
                    else if (player.speed < 0) {
                        player.speed = 0.0f;
                        player.position.y = element->rect.y + element->rect.height + player.height;
                        break; 
                    }
                }
            }
    
            if (hitObstacle) break; 
        }

        // State Overrides
        if (player.hurtTimer <= 0)
        {
            if (!player.canJump) {
                if (player.speed < 0) {
                    newState = PLAYER_JUMP;
                }
            } else {
                 if (newState == PLAYER_WALK) {
                 } else if (target_speed_x == 0) {
                     newState = PLAYER_IDLE;
                 }
            }
        } 
        
        playerState = newState;
        
        // Update Animations (Asumiendo que las referencias a fall se eliminarán de animations.c)
        switch (playerState) {
            case PLAYER_IDLE:
                UpdateAnimation(&idle, deltaTime);
                break;
            case PLAYER_WALK:
                UpdateAnimation(&walk, deltaTime);
                break;
            case PLAYER_JUMP:
                UpdateAnimation(&jump, deltaTime);
                break;
            case PLAYER_SLIDE:
                UpdateAnimation(&wall_slide, deltaTime);
                break;
            case PLAYER_HURT:
                UpdateAnimation(&hurt, deltaTime);
                break;
            case PLAYER_DEATH:
                UpdateAnimation(&death, deltaTime);
                break;
        }
        
        player.canJump = (hitObstacle == 1);
        
        if (IsKeyPressed(KEY_R))
        {
            player.position = (Vector2){ 400, 280 };
            player.speed = 0;
            player.speed_x = 0;
            player.wallJumpTimer = 0;
            player.canJump = false;
            player.lives = 5;
            player.hurtTimer = 0.0f; 

            camera.target = player.position;
            camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
            camera.rotation = 0.0f;
            camera.zoom = 1.0f; 
        }
        camera.target = player.position;
        camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
        float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

        for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
        {
            EnvElement *element = &envElements[i];
            minX = fminf(element->rect.x, minX);
            maxX = fmaxf(element->rect.x + element->rect.width, maxX);
            minY = fminf(element->rect.y, minY);
            maxY = fmaxf(element->rect.y + element->rect.height, maxY);
        }

        Vector2 max = GetWorldToScreen2D((Vector2){ maxX, maxY }, camera);
        Vector2 min = GetWorldToScreen2D((Vector2){ minX, minY }, camera);

        if (max.x < screenWidth) camera.offset.x = screenWidth - (max.x - screenWidth/2);
        if (max.y < screenHeight) camera.offset.y = screenHeight - (max.y - screenHeight/2);
        if (min.x > 0) camera.offset.x = screenWidth/2 - min.x;
        if (min.y > 0) camera.offset.y = screenHeight/2 - min.y;
        
        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(DARKBROWN);

            BeginMode2D(camera);

                // Draw environment elements
                for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
                {
                    EnvElement *element = &envElements[i];

                    if (element->blocking)
                    {
                        Rectangle destRec = element->rect;
                        Rectangle sourceRec = element->tileSourceRec;
                        float tileSize = sourceRec.width; 
            
                        int tilesX = (int)ceil(destRec.width / tileSize);
                        int tilesY = (int)ceil(destRec.height / tileSize);

                        for (int y = 0; y < tilesY; y++) {
                            for (int x = 0; x < tilesX; x++) {
                                Vector2 tilePosition = { destRec.x + (x * tileSize), destRec.y + (y * tileSize) };
                    
                                Rectangle drawDestRec = { 
                                    tilePosition.x, tilePosition.y, 
                                    tileSize, tileSize 
                                };

                                DrawTexturePro(tilemapTexture, sourceRec, drawDestRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                            }
                        }
                    }
                    else 
                    {
                        DrawRectangleRec(element->rect, element->color);
                    }
                }
                
                // Draw Danger elements
                for (int i = 0; i < MAX_DANGER_ELEMENTS; i++)
                {
                    Danger *danger = &dangerElements[i];

                    Rectangle destRec = danger->rect;
                    Rectangle sourceRec = danger->tileSourceRec;
                    float tileSize = sourceRec.width; 
        
                    if (destRec.width == 0.0f || destRec.height == 0.0f) continue;

                    int tilesX = (int)ceil(destRec.width / tileSize);
                    int tilesY = (int)ceil(destRec.height / tileSize);

                    for (int y = 0; y < tilesY; y++) {
                        for (int x = 0; x < tilesX; x++) {
                            Vector2 tilePosition = { destRec.x + (x * tileSize), destRec.y + (y * tileSize) };
                
                            Rectangle drawDestRec = { 
                                tilePosition.x, tilePosition.y, 
                                tileSize, tileSize 
                            };

                            DrawTexturePro(tilemapTexture, sourceRec, drawDestRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                        }
                    }
                }


                // Player
                DrawPlayer(player.position);

            EndMode2D();

            // Draw game controls
            DrawRectangle(10, 10, 220, 140, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(10, 10, 220, 140, Fade(BLUE, 0.8f));

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- RIGHT | LEFT: Player movement", 30, 40, 10, DARKGRAY);
            DrawText("- SPACE: Player jump / Wall Jump", 30, 60, 10, DARKGRAY);
            DrawText("- R: Reset game state", 30, 80, 10, DARKGRAY);
            DrawText(TextFormat("- LIVES: %d", player.lives), 30, 90, 10, player.lives > 0 ? LIME : RED);
            DrawText(TextFormat("WALL: %s (Side: %d)", hitWall ? "TRUE" : "FALSE", wallSide), 30, 110, 10, hitWall ? LIME : DARKGRAY);
            DrawText(TextFormat("HURT_T: %.2f", player.hurtTimer), 30, 120, 10, player.hurtTimer > 0 ? ORANGE : DARKGRAY);
            DrawText(TextFormat("SPEED_X: %.2f | JUMP_T: %.2f", player.speed_x, player.wallJumpTimer), 240, 10, 10, BLACK);
            DrawText(TextFormat("STATE: %d", playerState), 240, 20, 10, BLACK);


        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    // --------------------------------------------------------------------------------------
    UnloadResources();
    CloseWindow();
    // --------------------------------------------------------------------------------------

    return 0;
}

//Function Definition
// --------------------------------------------------------------------------------------
Rectangle GetPlayerHitbox(Player player)
{
    float x = player.position.x - player.width / 2.0f;
    float y = player.position.y - player.height;
    
    return (Rectangle){ x, y, player.width, player.height };
}
void LoadResources(void)
{
    tilemapTexture = LoadTexture("resources/tiles/tiles.png");
    LoadPlayerAnimations();
}
void UnloadResources(void)
{
    UnloadTexture(tilemapTexture);
    UnloadPlayerAnimations();
}
void ResetPlayer(Player *player)
{
    player->position = (Vector2){ 400, 280 };
    player->speed = 0;
    player->speed_x = 0;
    player->wallJumpTimer = 0;
    player->canJump = false;
    player->lives--;
    playerState = PLAYER_IDLE; 
}
// --------------------------------------------------------------------------------------