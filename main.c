#include "raylib.h"
#include "raymath.h"

#define GRAVITY 850
#define PLAYER_JUMP_SPD 500.0f
#define PLAYER_HOR_SPD 200.0f

#define MAX_ENVIRONMENT_ELEMENTS    10

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player {
    Vector2 position;
    float speed;
    bool canJump;
} Player;

typedef struct EnvElement {
    Rectangle rect;
    int blocking;
    Color color;
} EnvElement;

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib - platformer");

    // Define player
    Player player = { 0 };
    player.position = (Vector2){ 400, 280 };
    player.speed = 0;
    player.canJump = false;

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
    camera.zoom = 1.0f;

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        float deltaTime = 0.015f;//GetFrameTime();

        // Dropped files logic
        //---------------------------------------------------------------------------------
        //----------------------------------------------------------------------------------

        // Update player
        //----------------------------------------------------------------------------------
        if (IsKeyDown(KEY_LEFT)) player.position.x -= PLAYER_HOR_SPD*deltaTime;
        if (IsKeyDown(KEY_RIGHT)) player.position.x += PLAYER_HOR_SPD*deltaTime;
        if (IsKeyDown(KEY_SPACE) && player.canJump)
        {
            player.speed = -PLAYER_JUMP_SPD;
            player.canJump = false;
        }

        int hitObstacle = 0;
        for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
        {
            EnvElement *element = &envElements[i];
            Vector2 *p = &(player.position);
            if (element->blocking &&
                element->rect.x <= p->x &&
                element->rect.x + element->rect.width >= p->x &&
                element->rect.y >= p->y &&
                element->rect.y <= p->y + player.speed*deltaTime)
            {
                hitObstacle = 1;
                player.speed = 0.0f;
                p->y = element->rect.y;
            }
        }

        if (!hitObstacle)
        {
            player.position.y += player.speed*deltaTime;
            player.speed += GRAVITY*deltaTime;
            player.canJump = false;
        }
        else player.canJump = true;

        if (IsKeyPressed(KEY_R))
        {
            // Reset game state
            player.position = (Vector2){ 400, 280 };
            player.speed = 0;
            player.canJump = false;

            camera.target = player.position;
            camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
            camera.rotation = 0.0f;
            camera.zoom = 1.0f;
        }

        // Update camera
        //----------------------------------------------------------------------------------
        camera.target = player.position;
        camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
        float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

        // WARNING: On event replay, mouse-wheel internal value is set
        camera.zoom += ((float)GetMouseWheelMove()*0.05f);
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.25f) camera.zoom = 0.25f;

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
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(LIGHTGRAY);

            BeginMode2D(camera);

                // Draw environment elements
                for (int i = 0; i < MAX_ENVIRONMENT_ELEMENTS; i++)
                {
                    DrawRectangleRec(envElements[i].rect, envElements[i].color);
                }

                // Draw player rectangle
                DrawRectangleRec((Rectangle){ player.position.x - 20, player.position.y - 40, 40, 40 }, RED);

            EndMode2D();

            // Draw game controls
            DrawRectangle(10, 10, 220, 90, Fade(SKYBLUE, 0.5f));
            DrawRectangleLines(10, 10, 220, 90, Fade(BLUE, 0.8f));

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- RIGHT | LEFT: Player movement", 30, 40, 10, DARKGRAY);
            DrawText("- SPACE: Player jump", 30, 60, 10, DARKGRAY);
            DrawText("- R: Reset game state", 30, 80, 10, DARKGRAY);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}