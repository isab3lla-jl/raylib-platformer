#include "raylib.h"
#include "raymath.h"
#include "animations.c"

#define GRAVITY 850.0f
#define PLAYER_JUMP_SPD 500.0f
#define PLAYER_HOR_SPD 200.0f
#define MAX_ENVIRONMENT_ELEMENTS 10


#define PLAYER_WALL_SLIDE_SPD 75.0f
#define PLAYER_WALL_JUMP_X_SPD 350.0f
#define PLAYER_WALL_JUMP_DURATION 0.15f 

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
} Player;

typedef struct EnvElement {
    Rectangle rect;
    int blocking;
    Color color;
} EnvElement;

//Declaring Functions
Rectangle GetPlayerHitbox(Player player);
void LoadResources(void);
void UnloadResources(void);

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

    // Define environment elements (platforms)
    EnvElement envElements[MAX_ENVIRONMENT_ELEMENTS] = {
        {{ 0, 0, 1000, 1000 }, 0, LIGHTGRAY },
        {{ 0, -400, 200, 1000 }, 1, GRAY },
        {{ 800, -400, 200, 1000 }, 1, GRAY },
        {{ 0, 400, 1000, 200 }, 1, GRAY },
        {{ 300, 200, 400, 10 }, 1, GRAY },
        {{ 250, 300, 100, 10 }, 1, GRAY },
        {{ 10, 100, 150, 10 }, 1, GRAY },
        {{ 650, 300, 100, 10 }, 1, GRAY }
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
        
        //Wall Jump Variables
        bool hitWall = false;
        int wallSide = 0; // -1: Left Wall, 1: Right Wall

        player.speed += GRAVITY * deltaTime;
        if (player.wallJumpTimer > 0) player.wallJumpTimer -= deltaTime;

        float target_speed_x = 0;
        PlayerState newState = PLAYER_IDLE;
        
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
        // Movement
        float old_player_x = player.position.x; 
        player.position.x += player.speed_x * deltaTime;

        // Collisions
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

        //Wall Jump Logic
        if (IsKeyPressed(KEY_SPACE)) 
        {
            if (player.canJump)
            {
                //Normal Jump
                player.speed = -PLAYER_JUMP_SPD;
                player.canJump = false;
            }
            //Wall Jump Condition
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
                newState = PLAYER_SLIDE;
            }
        }
        
        //Vertical Movement
        const int SUB_STEPS = 4;
        float subDeltaTime = deltaTime / SUB_STEPS;
        int hitObstacle = 0; 

        for (int step = 0; step < SUB_STEPS; step++)
        {
            player.position.y += player.speed * subDeltaTime;
            Rectangle playerHitbox = GetPlayerHitbox(player); 

            for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
            {
                EnvElement *element = &envElements[i];

                if (element->blocking && CheckCollisionRecs(playerHitbox, element->rect))
                {
                    if (player.speed >= 0) 
                    {
                        hitObstacle = 1;
                        player.speed = 0.0f;
                        player.position.y = element->rect.y;
                        break; 
                    }
                }
            }
            
            if (hitObstacle) break; 
        }

        if (!player.canJump) {
            if (player.speed < 0) {
            newState = PLAYER_JUMP;
        }} 

        playerState = newState;
        
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

            ClearBackground(LIGHTGRAY);

            BeginMode2D(camera);

                // Draw environment elements
                for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
                {
                    DrawRectangleRec(envElements[i].rect, envElements[i].color);
                }

                // Player Hitbox
                DrawPlayer(player.position);
                //DrawRectangleRec(GetPlayerHitbox(player), RED);

            EndMode2D();

            // Draw game controls
            DrawRectangle(10, 10, 220, 110, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(10, 10, 220, 110, Fade(BLUE, 0.8f));

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- RIGHT | LEFT: Player movement", 30, 40, 10, DARKGRAY);
            DrawText("- SPACE: Player jump / Wall Jump", 30, 60, 10, DARKGRAY);
            DrawText("- R: Reset game state", 30, 80, 10, DARKGRAY);
            DrawText(TextFormat("WALL: %s (Side: %d)", hitWall ? "TRUE" : "FALSE", wallSide), 30, 100, 10, hitWall ? LIME : DARKGRAY);
            DrawText(TextFormat("SPEED_X: %.2f | JUMP_T: %.2f", player.speed_x, player.wallJumpTimer), 240, 10, 10, BLACK);


        EndDrawing();
        // ----------------------------------------------------------------------------------
    }

    // De-Initialization
    // --------------------------------------------------------------------------------------
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
    LoadPlayerAnimations();
}
void UnloadResources(void)
{
    UnloadPlayerAnimations();
}
// --------------------------------------------------------------------------------------