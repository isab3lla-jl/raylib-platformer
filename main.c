#include "raylib.h"
#include "raymath.h"
#include "animations.c"

#define GRAVITY 850.0f
#define PLAYER_JUMP_SPD 500.0f
#define PLAYER_HOR_SPD 200.0f
#define MAX_ENVIRONMENT_ELEMENTS 50
#define MAX_DANGER_ELEMENTS 20
#define MAX_WIN_ELEMENTS 1 // Nueva constante

#define PLAYER_WALL_SLIDE_SPD 75.0f
#define PLAYER_WALL_JUMP_X_SPD 350.0f
#define PLAYER_WALL_JUMP_DURATION 0.15f 

#define KNOCKBACK_HOR_SPD 400.0f
#define KNOCKBACK_VER_SPD -300.0f
#define HURT_DURATION 0.5f


// ----------------------------------------------------------------------------------
// Types and Structures Definition (Player, EnvElement, Danger, Win, Screen Handler)
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

// Estructura WinElement (usando tu definición previa)
typedef struct Win {
    Rectangle rect;
    int blocking; // 0 para no bloquear movimiento, 1 para bloquear (aunque para victoria, 0 es común)
    Color color;
    Rectangle tileSourceRec;
} Win;

typedef enum {
    TITLE, 
    CREDITS, 
    GAMEPLAY, 
    ENDING 
} GameScreen;

static GameScreen currentScreen = TITLE;

typedef enum {
    STATE_PLAYING = 0,
    STATE_DEAD,
    STATE_WAIT_RESTART
} GameState;

static GameState gameState = STATE_PLAYING;

Texture2D tilemapTexture;

// Variables de estado de finalización
static bool gameOver = false;
static bool win = false;

//Declaring Functions
void LoadResources(void);
void UnloadResources(void);
void ResetPlayer(Player *player);
void ResetGame(Player *player, Camera2D *camera, int screenWidth, int screenHeight); 

Rectangle GetPlayerHitbox(Player player);
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
    player.width = 24.0f; 
    player.height = 32.0f;
    
    // Define camera
    Camera2D camera = { 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f; 

    ResetGame(&player, &camera, screenWidth, screenHeight);

    Rectangle tile_safe = { 70, 60, 10, 10 }; //safe
    Rectangle tile_danger = { 70, 50, 10, 10 }; //danger
    Rectangle tile_walls = { 230, 190, 10, 10 }; //walls
    Rectangle tile_win = { 160, 70, 30, 10 }; // win


    // Define environment elements
    EnvElement envElements[MAX_ENVIRONMENT_ELEMENTS] = {
        //{{ 0, 0, 1000, 1000 }, 0, DARKBROWN, tile_walls}, //background
        {{ 0, -400, 200, 1000 }, 1, GRAY, tile_walls }, //left wall
        {{ 800, -400, 200, 1000 }, 1, GRAY, tile_walls }, //right wall
        {{ 0, 400, 1000, 200 }, 1, GRAY, tile_walls }, //floor
        {{ 0, -400, 1000, 100 }, 1, GRAY, tile_walls }, //roof
        {{ 300, 200, 150, 10 }, 1, GRAY, tile_safe },
        {{ 600, 200, 150, 10 }, 1, GRAY, tile_safe },
        {{ 250, 300, 100, 10 }, 1, GRAY, tile_safe },
        {{ 300, 30, 120, 10 }, 1, GRAY, tile_safe },
        {{ 550, -120, 10, 50 }, 1, GRAY, tile_safe },
        {{ 610, -150, 160, 10 }, 1, GRAY, tile_safe },
    };
    
    // Define danger elements
    Danger dangerElements[MAX_DANGER_ELEMENTS] = {
        {{ 450, 200, 150, 10 }, 0, RED, tile_danger },
        {{ 650, 80, 150, 10 }, 0, RED, tile_danger },
        {{ 460, -50, 10, 150 }, 0, RED, tile_danger },
    };

    // Define win elements (Nuevo)
    Win winElements[MAX_WIN_ELEMENTS] = {
        {{ 700, -180, 50, 30 }, 0, GREEN, tile_win }, 
    };


    SetTargetFPS(60);

    LoadResources();
    // --------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();
        
        // ----------------------------------------------------------------------------------
        // Update
        // ----------------------------------------------------------------------------------
        switch (currentScreen)
        {
            case TITLE:
            {
                if (IsKeyPressed(KEY_SPACE)) {
                    currentScreen = GAMEPLAY;
                }
                if (IsKeyPressed(KEY_C)) {
                    currentScreen = CREDITS;
                }
            } break;

            case GAMEPLAY:
            {
                // ----------------------------------------------------------------------------------
                // Game Logic
                // ----------------------------------------------------------------------------------
                if (gameState == STATE_PLAYING || gameState == STATE_DEAD)
                {   
                    // Hurt Timer Logic
                    if (player.hurtTimer > 0) {
                        player.hurtTimer -= deltaTime;
                        if (player.hurtTimer <= 0) {
                            player.hurtTimer = 0.0f; 
                        }
                        if (playerState != PLAYER_DEATH) {
                            playerState = PLAYER_HURT;
                        }
                    }
                    else if (playerState == PLAYER_HURT) {
                        playerState = PLAYER_IDLE;
                    }

                    // Gravity & Death check
                    if (playerState == PLAYER_DEATH) {
                        if (!IsAnimationFinished(&death)) {
                            player.speed += GRAVITY * deltaTime;
                        } else {
                            gameOver = true;
                            currentScreen = ENDING; 
                            player.speed = 0;
                            player.speed_x = 0;
                        }
                    } else {
                        player.speed += GRAVITY * deltaTime;
                        if (player.wallJumpTimer > 0) player.wallJumpTimer -= deltaTime;
                    }
                    
                    bool hitWall = false;
                    int wallSide = 0; 

                    float target_speed_x = 0;
                    PlayerState newState = playerState; 
                    
                    // Movement
                    if (player.hurtTimer <= 0 && gameState == STATE_PLAYING) {
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
                        } else {
                            target_speed_x = player.speed_x; 
                        }
                    } 
                    else if (playerState == PLAYER_HURT || gameState == STATE_DEAD) {
                        target_speed_x = player.speed_x;
                    } else {
                        player.speed_x = 0;
                    }
                    
                    float old_player_x = player.position.x; 
                    player.position.x += player.speed_x * deltaTime; 

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
                    
                    // Danger collision
                    for (int i = 0; i < MAX_DANGER_ELEMENTS; i++) {
                        Danger *danger = &dangerElements[i];

                        if (player.hurtTimer <= 0 && playerState != PLAYER_DEATH && CheckCollisionRecs(currentHitbox, danger->rect))
                        {
                            player.lives--;
                            
                            if (player.lives <= 0)
                            {
                                playerState = PLAYER_DEATH;
                                ResetAnimation(&death);
                                player.speed_x = 0; 
                                player.speed = 0; 
                                break;
                            }
                            
                            // Knockback
                            playerState = PLAYER_HURT;
                            player.hurtTimer = HURT_DURATION;
                            ResetAnimation(&hurt); 
                            
                            float knockbackDirX = (player.position.x < danger->rect.x + danger->rect.width / 2.0f) ? -1.0f : 1.0f;
                            
                            player.speed_x = knockbackDirX * KNOCKBACK_HOR_SPD;
                            player.speed = KNOCKBACK_VER_SPD; 
                            player.canJump = false;
                            
                            break; 
                        }
                    }
                    
                    if (gameState == STATE_PLAYING)
                    {
                        for (int i = 0; i < MAX_WIN_ELEMENTS; i++) {
                            Win *win_element = &winElements[i];
                            
                            if (CheckCollisionRecs(currentHitbox, win_element->rect))
                            {
                                win = true;
                                currentScreen = ENDING;
                                player.speed_x = 0; 
                                player.speed = 0; 
                                gameState = STATE_WAIT_RESTART;
                                break; 
                            }
                        }
                    }

                    //Wall Jump Logic 
                    if (player.hurtTimer <= 0 && gameState == STATE_PLAYING && IsKeyPressed(KEY_SPACE)) 
                    {
                        if (player.canJump)
                        {
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
                    if (hitWall && player.speed > 0 && player.hurtTimer <= 0 && gameState == STATE_PLAYING)
                    {
                        bool pressingIntoWall = (wallSide == 1 && IsKeyDown(KEY_RIGHT)) || (wallSide == -1 && IsKeyDown(KEY_LEFT));
                        
                        if (pressingIntoWall && player.speed > PLAYER_WALL_SLIDE_SPD)
                        {
                            player.speed = PLAYER_WALL_SLIDE_SPD;
                            newState = PLAYER_SLIDE;
                        }
                    }
                    
                    //Vertical Movement
                    const int SUB_STEPS = 4;
                    float subDeltaTime = deltaTime / SUB_STEPS;
                    int hitObstacle = 0; 

                    if (gameState != STATE_WAIT_RESTART)
                    {
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
                    }

                    //State Overrides 
                    if (player.hurtTimer <= 0 && gameState == STATE_PLAYING)
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
                    
                    if (playerState != PLAYER_DEATH && playerState != PLAYER_HURT)
                    {
                        playerState = newState;
                    }
                    
                    player.canJump = (hitObstacle == 1);
                    
                    // Update Animations
                    switch (playerState) {
                        case PLAYER_IDLE: UpdateAnimation(&idle, deltaTime); break;
                        case PLAYER_WALK: UpdateAnimation(&walk, deltaTime); break;
                        case PLAYER_JUMP: UpdateAnimation(&jump, deltaTime); break;
                        case PLAYER_SLIDE: UpdateAnimation(&wall_slide, deltaTime); break;
                        case PLAYER_HURT: UpdateAnimation(&hurt, deltaTime); break;
                        case PLAYER_DEATH: UpdateAnimation(&death, deltaTime); break;
                    }
                }

            } break;

            case ENDING: 
            {
                if (IsKeyPressed(KEY_T))
                {
                    ResetGame(&player, &camera, screenWidth, screenHeight);
                    currentScreen = TITLE;
                }
            } break;

            case CREDITS:
            {
                if (IsKeyPressed(KEY_T)) {
                    currentScreen = TITLE;
                }
            } break;
        }

        // ----------------------------------------------------------------------------------
        // Camera
        // ----------------------------------------------------------------------------------
        if (currentScreen == GAMEPLAY)
        {
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
        }
        
        // ----------------------------------------------------------------------------------

        // Draw
        // ----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(DARKBROWN);

            switch (currentScreen)
            {
                case TITLE:
                {
                    DrawText("ESCAPA DEL CASTILLO", screenWidth/2 - MeasureText("ESCAPA DEL CASTILLO", 40)/2, screenHeight/2 - 50, 40, LIME);
                    DrawText("Presiona ESPACIO para comenzar", screenWidth/2 - MeasureText("Presiona ESPACIO para comenzar", 20)/2, screenHeight/2 + 20, 20, WHITE);
                    DrawText("Presiona C para ver los creditos", screenWidth/2 - MeasureText("Presiona C para ver los creditos", 20)/2, screenHeight/2 + 50, 20, WHITE);
                    DrawText("Presiona ESC para salir", screenWidth/2 - MeasureText("Presiona ESC para salir", 20)/2, screenHeight/2 + 80, 20, WHITE);
                } break;

                case CREDITS:
                {
                    DrawText("CREDITOS", screenWidth/2 - MeasureText("CREDITOS", 40)/2, screenHeight/2 - 70, 40, YELLOW);
                    DrawText("DESARROLLADORA: Isabella Juarez Luna", screenWidth/2 - MeasureText("DESARROLLADORA: Isabella Juarez Luna", 20)/2, screenHeight/2 - 10, 20, WHITE);
                    DrawText("SPRITES DEL PERSONAJE: Free Game Assets (GUI, Sprite, Tilesets)", screenWidth/2 - MeasureText("SPRITES DEL PERSONAJE: Free Game Assets (GUI, Sprite, Tilesets)", 20)/2, screenHeight/2 + 20, 20, WHITE);
                    DrawText("TILEMAP: VEXED", screenWidth/2 - MeasureText("TILEMAP: VEXED", 20)/2, screenHeight/2 + 50, 20, WHITE);
                    DrawText("Presiona T para regresal al menu", screenWidth/2 - MeasureText("Presiona T para regresal al menu", 20)/2, screenHeight/2 + 80, 20, WHITE);
                } break;

                case GAMEPLAY:
                {
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
                                        Rectangle drawDestRec = { tilePosition.x, tilePosition.y, tileSize, tileSize };
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
                                    Rectangle drawDestRec = { tilePosition.x, tilePosition.y, tileSize, tileSize };
                                    DrawTexturePro(tilemapTexture, sourceRec, drawDestRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                                }
                            }
                        }
                        
                        for (int i = 0; i < MAX_WIN_ELEMENTS; i++)
                        {
                            Win *win_element = &winElements[i];
                            Rectangle destRec = win_element->rect;
                            Rectangle sourceRec = win_element->tileSourceRec;
                            float tileSize = sourceRec.width; 
                            if (destRec.width == 0.0f || destRec.height == 0.0f) continue;
                            int tilesX = (int)ceil(destRec.width / tileSize);
                            int tilesY = (int)ceil(destRec.height / tileSize);
                            for (int y = 0; y < tilesY; y++) {
                                for (int x = 0; x < tilesX; x++) {
                                    Vector2 tilePosition = { destRec.x + (x * tileSize), destRec.y + (y * tileSize) };
                                    Rectangle drawDestRec = { tilePosition.x, tilePosition.y, tileSize, tileSize };
                                    DrawTexturePro(tilemapTexture, sourceRec, drawDestRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
                                }
                            }
                        }

                        // Player
                        DrawPlayer(player.position); 

                    EndMode2D();
                }
                
                // Draw game control
                DrawText("Controles:", 20, 35, 20, BLACK);
                DrawText("Flechas izq der", 20, 60, 20, BLACK);
                DrawText("Salto - espacio", 20, 85, 20, BLACK);
                DrawText(TextFormat("Vidas: %d", player.lives), 20, 105, 20, player.lives > 0 ? DARKGREEN : RED);


                break;

                case ENDING:
                {
                    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));

                    if (gameOver) {
                        const char *gameOverText = "GAME OVER";
                        int fontSizeGO = 60;
                        int textWidthGO = MeasureText(gameOverText, fontSizeGO);
                        DrawText(gameOverText, (screenWidth - textWidthGO) / 2, screenHeight / 2 - 50, fontSizeGO, RED);

                    } else if (win) {
                        const char *winText = "¡VICTORIA! HAS ESCAPADO";
                        int fontSizeGO = 50;
                        int textWidthGO = MeasureText(winText, fontSizeGO);
                        DrawText(winText, (screenWidth - textWidthGO) / 2, screenHeight / 2 - 50, fontSizeGO, GREEN);
                    }
                    
                    const char *retryText = "PRESIONA 'T' PARA IR AL MENU";
                    int fontSizeRetry = 20;
                    int textWidthRetry = MeasureText(retryText, fontSizeRetry);
                    DrawText(retryText, (screenWidth - textWidthRetry) / 2, screenHeight / 2 + 30, fontSizeRetry, WHITE);
                } break;
            }


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
    player->hurtTimer = 0.0f;
}
void ResetGame(Player *player, Camera2D *camera, int screenWidth, int screenHeight)
{
    ResetPlayer(player);
    player->lives = 5;
    playerState = PLAYER_IDLE;
    gameState = STATE_PLAYING;
    ResetAnimation(&death);
    gameOver = false;
    win = false;
    
    camera->target = player->position;
    camera->offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera->rotation = 0.0f;
    camera->zoom = 2.0f; 
}
// --------------------------------------------------------------------------------------