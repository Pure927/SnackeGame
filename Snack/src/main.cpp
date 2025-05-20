#include "raylib.h"
#include <vector>   // 用于存储蛇的身体段
#include <deque>    // 也可以用双端队列来优化蛇身体的移动
#include <iostream> // 用于调试输出 (可选)

// ------------------------------------------------------------------------------------
// Game Defines
// ------------------------------------------------------------------------------------
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SQUARE_SIZE = 20; // 每个格子的大小
const int GAME_AREA_WIDTH = SCREEN_WIDTH / SQUARE_SIZE;
const int GAME_AREA_HEIGHT = SCREEN_HEIGHT / SQUARE_SIZE;

// 蛇的移动方向
typedef enum
{
    DIR_RIGHT = 0,
    DIR_LEFT,
    DIR_UP,
    DIR_DOWN
} SnakeDirection;

// 游戏状态
typedef enum
{
    STATE_PLAYING,
    STATE_GAME_OVER
} GameState;

// ------------------------------------------------------------------------------------
// Game Structures
// ------------------------------------------------------------------------------------
struct SnakeSegment
{
    Vector2 position; // 格子坐标 (不是像素坐标)
};

struct Food
{
    Vector2 position; // 格子坐标
    bool active;
};

// ------------------------------------------------------------------------------------
// Global Variables
// ------------------------------------------------------------------------------------
static GameState gameState;
static std::vector<SnakeSegment> snake;
static SnakeDirection snakeDir;
static SnakeDirection nextSnakeDir; // 用于缓存下一个方向，防止快速按键导致180度转向
static Food food;
static bool allowMove; // 控制蛇的移动频率
static float moveTimer;
static const float MOVE_INTERVAL = 0.15f; // 蛇移动的时间间隔 (秒)
static int score;
static bool paused;

// ------------------------------------------------------------------------------------
// Module Functions Declaration
// ------------------------------------------------------------------------------------
void InitGame(void);   // Initialize game
void UpdateGame(void); // Update game (one frame)
void DrawGame(void);   // Draw game (one frame)
void SpawnFood(void);  // Spawn food at a random valid position

// ------------------------------------------------------------------------------------
// Program main entry point
// ------------------------------------------------------------------------------------
int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Simple Raylib Snake");
    SetTargetFPS(60);

    InitGame();

    while (!WindowShouldClose())
    {
        UpdateGame();
        DrawGame();
    }

    CloseWindow();
    return 0;
}

// ------------------------------------------------------------------------------------
// Module Functions Implementation
// ------------------------------------------------------------------------------------
void InitGame(void)
{
    gameState = STATE_PLAYING;
    paused = false;
    score = 0;

    snake.clear();
    // 初始化蛇头
    snake.push_back({{(float)GAME_AREA_WIDTH / 2, (float)GAME_AREA_HEIGHT / 2}});
    // 可以初始时多几节身体
    snake.push_back({{(float)GAME_AREA_WIDTH / 2 - 1, (float)GAME_AREA_HEIGHT / 2}});
    snake.push_back({{(float)GAME_AREA_WIDTH / 2 - 2, (float)GAME_AREA_HEIGHT / 2}});

    snakeDir = DIR_RIGHT;
    nextSnakeDir = DIR_RIGHT;

    SpawnFood();

    allowMove = false;
    moveTimer = 0.0f;
}

void SpawnFood(void)
{
    food.active = false;
    while (!food.active)
    {
        food.position = {
            (float)GetRandomValue(0, GAME_AREA_WIDTH - 1),
            (float)GetRandomValue(0, GAME_AREA_HEIGHT - 1)};
        food.active = true;

        // 确保食物不生成在蛇身上
        for (const auto &segment : snake)
        {
            if (CheckCollisionRecs(
                    {food.position.x * SQUARE_SIZE, food.position.y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE},
                    {segment.position.x * SQUARE_SIZE, segment.position.y * SQUARE_SIZE, SQUARE_SIZE, SQUARE_SIZE}))
            {
                food.active = false;
                break;
            }
        }
    }
}

void UpdateGame(void)
{
    if (gameState == STATE_GAME_OVER)
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame(); // 按回车重新开始
        }
        return;
    }

    if (IsKeyPressed(KEY_P))
        paused = !paused;

    if (paused)
        return;

    // --- 处理输入 ---
    if (IsKeyPressed(KEY_RIGHT) && snakeDir != DIR_LEFT)
        nextSnakeDir = DIR_RIGHT;
    if (IsKeyPressed(KEY_LEFT) && snakeDir != DIR_RIGHT)
        nextSnakeDir = DIR_LEFT;
    if (IsKeyPressed(KEY_UP) && snakeDir != DIR_DOWN)
        nextSnakeDir = DIR_UP;
    if (IsKeyPressed(KEY_DOWN) && snakeDir != DIR_UP)
        nextSnakeDir = DIR_DOWN;

    // --- 更新蛇的移动计时器 ---
    moveTimer += GetFrameTime();
    if (moveTimer >= MOVE_INTERVAL)
    {
        moveTimer = 0.0f;
        allowMove = true;
    }

    if (allowMove)
    {
        snakeDir = nextSnakeDir; // 应用缓存的方向
        Vector2 oldHeadPos = snake[0].position;
        Vector2 newHeadPos = oldHeadPos;

        // --- 移动蛇头 ---
        switch (snakeDir)
        {
        case DIR_RIGHT:
            newHeadPos.x++;
            break;
        case DIR_LEFT:
            newHeadPos.x--;
            break;
        case DIR_UP:
            newHeadPos.y--;
            break;
        case DIR_DOWN:
            newHeadPos.y++;
            break;
        }

        // --- 碰撞检测 ---
        // 1. 撞墙
        if (newHeadPos.x < 0 || newHeadPos.x >= GAME_AREA_WIDTH ||
            newHeadPos.y < 0 || newHeadPos.y >= GAME_AREA_HEIGHT)
        {
            gameState = STATE_GAME_OVER;
            PlaySound(LoadSound("resources/gameover.wav")); // 可选：添加音效
            return;
        }

        // 2. 撞自己身体 (从第二节开始检查，不包括尾巴，因为尾巴马上要移动)
        for (size_t i = 1; i < snake.size() - 1; ++i)
        { // 注意这里的循环条件
            if (newHeadPos.x == snake[i].position.x && newHeadPos.y == snake[i].position.y)
            {
                gameState = STATE_GAME_OVER;
                PlaySound(LoadSound("resources/gameover.wav")); // 可选
                return;
            }
        }

        // --- 移动蛇身体 ---
        // 将新头插入到最前面
        snake.insert(snake.begin(), {newHeadPos});

        // --- 检查是否吃到食物 ---
        bool ateFood = false;
        if (food.active && newHeadPos.x == food.position.x && newHeadPos.y == food.position.y)
        {
            ateFood = true;
            score += 10;
            SpawnFood();
            PlaySound(LoadSound("resources/eat.wav")); // 可选
        }

        // 如果没有吃到食物，移除蛇尾 (蛇身体向前移动的效果)
        if (!ateFood)
        {
            snake.pop_back();
        }

        allowMove = false; // 重置移动许可，等待下一个计时周期
    }
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(RAYWHITE);

    if (gameState == STATE_PLAYING)
    {
        // 绘制网格线 (可选)
        for (int i = 0; i < GAME_AREA_WIDTH; i++)
        {
            DrawLine(i * SQUARE_SIZE, 0, i * SQUARE_SIZE, SCREEN_HEIGHT, LIGHTGRAY);
        }
        for (int i = 0; i < GAME_AREA_HEIGHT; i++)
        {
            DrawLine(0, i * SQUARE_SIZE, SCREEN_WIDTH, i * SQUARE_SIZE, LIGHTGRAY);
        }

        // 绘制蛇
        for (size_t i = 0; i < snake.size(); ++i)
        {
            Color snakeColor = (i == 0) ? DARKGREEN : GREEN; // 蛇头用深绿色
            DrawRectangle(
                (int)snake[i].position.x * SQUARE_SIZE,
                (int)snake[i].position.y * SQUARE_SIZE,
                SQUARE_SIZE, SQUARE_SIZE, snakeColor);
        }

        // 绘制食物
        if (food.active)
        {
            DrawRectangle(
                (int)food.position.x * SQUARE_SIZE,
                (int)food.position.y * SQUARE_SIZE,
                SQUARE_SIZE, SQUARE_SIZE, RED);
        }

        // 绘制分数
        DrawText(TextFormat("Score: %i", score), 10, 10, 20, BLACK);

        if (paused)
        {
            DrawText("PAUSED", SCREEN_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, GRAY);
        }
    }
    else if (gameState == STATE_GAME_OVER)
    {
        DrawText("GAME OVER", SCREEN_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, SCREEN_HEIGHT / 2 - 40, 40, RED);
        DrawText(TextFormat("Your Score: %i", score), SCREEN_WIDTH / 2 - MeasureText(TextFormat("Your Score: %i", score), 20) / 2, SCREEN_HEIGHT / 2 + 10, 20, DARKGRAY);
        DrawText("Press [ENTER] to play again", SCREEN_WIDTH / 2 - MeasureText("Press [ENTER] to play again", 20) / 2, SCREEN_HEIGHT / 2 + 40, 20, GRAY);
    }

    EndDrawing();
}