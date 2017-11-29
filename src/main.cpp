#include "Arduino.h"

#include <TVout.h>
#include <fontALL.h>

#define PIN_UP_PLAYER_1 2
#define PIN_DOWN_PLAYER_1 3

#define PIN_UP_PLAYER_2 4
#define PIN_DOWN_PLAYER_2 5

#define WIDTH 128
#define HEIGHT 96
#define OFFSET 2

#define PLAYER_WIDTH 2
#define PLAYER_HEIGHT 14
#define PLAYER_HALF_HEIGHT PLAYER_HEIGHT / 2
#define PLAYER_1_POS_X (OFFSET + 2)
#define PLAYER_2_POS_X (WIDTH - OFFSET - PLAYER_WIDTH - 2)
#define PLAYER_SPEED 2

#define BALL_SIZE 4
#define BALL_SPEED_MAX 5

#define SCORE_MAX 10
#define SCORE_POS_Y 20
#define SCORE_1_X_POS 30
#define SCORE_2_X_POS (WIDTH - 30 - 8)

#define MENU_TITLE_POS_X ((WIDTH / 2) - (4 * 8))
#define MENU_TITLE_POS_Y 25

#define SCORE_TITLE_POS_X ((WIDTH / 2) - (4 * 8))
#define SCORE_TITLE_POS_Y 25

#define PRESS_A_BUTTON_POS_X ((WIDTH / 2) - (8 * 6))
#define PRESS_A_BUTTON_POS_Y 55

#define MADE_BY_POS_X ((WIDTH / 2) - (7 * 6))
#define MADE_BY_POS_Y 75

#define MESSAGE_PONG "PONG BOY"
#define MESSAGE_MADE_BY "Made by Mefteg"
#define MESSAGE_PRESS_A_BUTTON "Press a button !"
#define MESSAGE_P1 "P1"
#define MESSAGE_P2 "P2"
#define MESSAGE_WON "WON !"

enum State
{
    MENU,
    RESET,
    PLAY,
    SCORE
};

struct Player
{
    unsigned short id;
    unsigned short x;
    unsigned short y;
    unsigned short score;
};

struct Ball
{
    unsigned short x;
    unsigned short y;

    float dx;
    float dy;
};

/**
 * STATIC
 */

TVout TV;

State GameState = State::MENU;

Player Player1;
Player Player2;

Ball Ball0;

template <typename type>
type sign(type value) {
    if (value > 0) return 1;
    if (value < 0) return -1;
    return 0;
}

short GetDirection(short playerId)
{
    short direction = 0;

    unsigned short pinUp = playerId == 1 ? PIN_UP_PLAYER_1 : PIN_UP_PLAYER_2;
    unsigned short pinDown = playerId == 1 ? PIN_DOWN_PLAYER_1 : PIN_DOWN_PLAYER_2;

    if (digitalRead(pinUp) == HIGH)
    {
        direction = 1;
    }
    else if (digitalRead(pinDown) == HIGH)
    {
        direction = -1;
    }

    return direction;
}

void InitPlayer(Player& p, unsigned short id)
{
    p.id = id;
    p.x = id == 1 ? PLAYER_1_POS_X : PLAYER_2_POS_X;
    p.y = (HEIGHT - PLAYER_HEIGHT) / 2;
    p.score = 0;
}

// kickoff: 0: random; 1: player 1; 2: player 2
void InitBall(Ball& b, unsigned short kickoff)
{
    Ball0.x = (WIDTH - BALL_SIZE) / 2;
    Ball0.y = 20;

    if (kickoff == 0 || kickoff > 2)
    {
        Ball0.dx = (short)random(1, 2 + 1);
    }
    else
    {
        Ball0.dx = kickoff == 1 ? -1 : 1;
    }
    Ball0.dy = 1;
}

// kickoff: 0: random; 1: player 1; 2: player 2
void InitGame(unsigned short kickoff)
{
    // init players
    InitPlayer(Player1, 1);
    InitPlayer(Player2, 2);

    // init ball
    InitBall(Ball0, kickoff);
}

void UpdateMenu()
{
    // get inputs
    short dy1 = GetDirection(1);
    short dy2 = GetDirection(2);

    if (dy1 != 0 || dy2 != 0)
    {
        GameState = State::RESET;
    }
}

void ResetPlayerPosition(Player &p)
{
    p.y = (HEIGHT - PLAYER_HEIGHT) / 2;
}

void UpdateReset()
{
    ResetPlayerPosition(Player1);
    ResetPlayerPosition(Player2);

    unsigned short kickoff = 0;
    if (Player1.score < Player2.score)
    {
        kickoff = 1;
    }
    else if (Player2.score < Player1.score)
    {
        kickoff = 2;
    }

    InitBall(Ball0, kickoff);

    GameState = State::PLAY;
}

// dy : 1: go up; 0; nothing; -1: go down
void UpdatePlayer(Player& p, short dy)
{
    if (p.y + dy <= OFFSET + 1)
    {
        p.y = OFFSET + 1 + 1;
    }
    else if (p.y + dy + PLAYER_HEIGHT >= HEIGHT - OFFSET - 1)
    {
        p.y = HEIGHT - OFFSET - PLAYER_HEIGHT - 1 - 1;
    }
    else
    {
        p.y += dy * PLAYER_SPEED;
    }
}

float Easing(float ratio)
{
    return 1.0f - cos(ratio * M_PI_2);
}

void SetBallVelocityRegardingPlayer(Ball& b, const Player& p)
{
    // speed depends on where on the player the ball bounces
    float ratio = (b.y + (BALL_SIZE / 2) - p.y - PLAYER_HALF_HEIGHT) / PLAYER_HALF_HEIGHT;
    b.dx *= (-1 * (1 + Easing(1 - ratio)));
    b.dy *= (1 + Easing(ratio));

    if (fabs(b.dx) > BALL_SPEED_MAX)
    {
        b.dx = sign(b.dx) * BALL_SPEED_MAX;
    }

    if (fabs(b.dy) > BALL_SPEED_MAX)
    {
        b.dy = sign(b.dy) * BALL_SPEED_MAX;
    }
}

// return true if the ball is outside the field (player just lost)
// false otherwise
bool ManageBallInsidePlayerField(Ball& b, const Player& p)
{
    short newX = b.x + b.dx;
    short newY = b.y + b.dy;

    // compute intermediate X value
    unsigned short intermediateX = p.id == 1 ? p.x + PLAYER_WIDTH : p.x - BALL_SIZE;

    // compute intermediate Y value
    short a = intermediateX - b.x;
    short A = newX - b.x;
    short B = newY - b.y;
    short offset = (short)(((float)a / (float)A) * (float)B);

    unsigned short intermediateY = b.y + offset;

    // if the ball bounce against the player
    if (intermediateY + BALL_SIZE >= p.y && intermediateY <= p.y + PLAYER_HEIGHT)
    {
        b.x = intermediateX;
        b.y = intermediateY;

        SetBallVelocityRegardingPlayer(b, p);

        return false;
    }
    else
    {
        b.x = newX;
        b.y = newY;

        return true;
    }

}

void UpdateBall(Ball& b, Player& p1, Player& p2)
{
    if (b.x < p1.x + PLAYER_WIDTH)
    {
        // already lost
        if (b.x <= OFFSET)
        {
            // Player 2 won
            ++p2.score;
            if (p2.score >= SCORE_MAX)
            {
                GameState = State::SCORE;
            }
            else
            {
                GameState = State::RESET;
            }

            return;
        }

        b.x += b.dx;
        b.y += b.dy;
    }
    else if (b.x + b.dx <= p1.x + PLAYER_WIDTH) // Player 1 side
    {
        ManageBallInsidePlayerField(b, p1);
    }
    else if (b.x + BALL_SIZE > p2.x)
    {
        if (b.x + BALL_SIZE >= WIDTH - OFFSET)
        {
            // Player 1 won
            ++p1.score;
            if (p1.score >= SCORE_MAX)
            {
                GameState = State::SCORE;
            }
            else
            {
                GameState = State::RESET;
            }

            return;
        }

        b.x += b.dx;
        b.y += b.dy;
    }
    else if (b.x + b.dx + BALL_SIZE >= p2.x) // Player 2 side
    {
        ManageBallInsidePlayerField(b, p2);
    }
    else // on field
    {
        b.x += b.dx;

        if (b.y + b.dy <= OFFSET + 1)
        {
            b.y = OFFSET + 1 + 1;
            b.dy *= -1;

        }
        else if (b.y + b.dy + BALL_SIZE >= HEIGHT - OFFSET - 1)
        {
            b.y = HEIGHT - OFFSET - 1 - BALL_SIZE - 1;
            b.dy *= -1;
        }
        else
        {
            b.y += b.dy;
        }
    }
}

void UpdatePlay()
{
    // get inputs
    short dy1 = GetDirection(1);
    short dy2 = GetDirection(2);

    // update players
    UpdatePlayer(Player1, dy1);
    UpdatePlayer(Player2, dy2);
    // update ball
    UpdateBall(Ball0, Player1, Player2);
}

void UpdateScore()
{
    // get inputs
    short dy1 = GetDirection(1);
    short dy2 = GetDirection(2);

    if (dy1 != 0 || dy2 != 0)
    {
        InitGame(0);

        GameState = State::RESET;
    }
}

void DrawFrame()
{
    TV.draw_line(OFFSET, OFFSET, WIDTH - OFFSET - 1, OFFSET, WHITE);
    TV.draw_line(WIDTH - OFFSET - 1, OFFSET, WIDTH - OFFSET - 1, HEIGHT - OFFSET - 1, WHITE);
    TV.draw_line(WIDTH - OFFSET - 1, HEIGHT - OFFSET - 1, OFFSET, HEIGHT - OFFSET - 1, WHITE);
    TV.draw_line(OFFSET, HEIGHT - OFFSET - 1, OFFSET, OFFSET, WHITE);
}

void DrawMiddleLane()
{
    unsigned short hw = WIDTH / 2;
    unsigned short offset = OFFSET + 1; // general offset + border
    TV.draw_line(hw,        offset + 1 * 6 + 0 * 15, hw,        offset + 1 * (6 + 15), WHITE);
    TV.draw_line(hw,        offset + 3 * 6 + 2 * 15, hw,        offset + 3 * (6 + 15), WHITE);

    TV.draw_line(hw + 1,    offset + 2 * 6 + 1 * 15, hw + 1,    offset + 2 * (6 + 15), WHITE);
    TV.draw_line(hw + 1,    offset + 4 * 6 + 3 * 15, hw + 1,    offset + 4 * (6 + 15), WHITE);
}

void DrawScore(const Player& p1, const Player& p2)
{
    String score1 = String(p1.score);
    String score2 = String(p2.score);

    TV.select_font(font8x8);
    TV.println(SCORE_1_X_POS, SCORE_POS_Y, score1.c_str());
    TV.println(SCORE_2_X_POS, SCORE_POS_Y, score2.c_str());
}

void DrawPlayer(const Player& p)
{
    for (unsigned short i=0; i<PLAYER_WIDTH; ++i)
    {
        TV.draw_line(p.x + i, p.y, p.x + i, p.y + PLAYER_HEIGHT, WHITE);
    }
}

void DrawBall(const Ball& b)
{
    for (unsigned short i=0; i<BALL_SIZE; ++i)
    {
        TV.draw_line(b.x + i, b.y, b.x + i, b.y + BALL_SIZE, WHITE);
    }
}

void DrawMenu()
{
    TV.select_font(font8x8);
    TV.println(MENU_TITLE_POS_X, MENU_TITLE_POS_Y, MESSAGE_PONG);
    TV.select_font(font6x8);
    TV.println(PRESS_A_BUTTON_POS_X, PRESS_A_BUTTON_POS_Y, MESSAGE_PRESS_A_BUTTON);
    TV.select_font(font4x6);
    TV.println(MADE_BY_POS_X, MADE_BY_POS_Y, MESSAGE_MADE_BY);
}

void DrawWinner(const Player& p1, const Player& p2)
{
    String winner = p1.score >= SCORE_MAX ? MESSAGE_P1 : MESSAGE_P2;
    winner += " " + String(MESSAGE_WON);

    TV.select_font(font8x8);
    TV.println(SCORE_TITLE_POS_X, SCORE_TITLE_POS_Y, winner.c_str());
    TV.select_font(font6x8);
    TV.println(PRESS_A_BUTTON_POS_X, PRESS_A_BUTTON_POS_Y, MESSAGE_PRESS_A_BUTTON);
}

void DrawGame()
{
    DrawFrame();

    switch (GameState) {
        case State::MENU:
        DrawMenu();
        break;

        case State::SCORE:
        DrawWinner(Player1, Player2);
        break;

        default:
        DrawMiddleLane();
        DrawScore(Player1, Player2);
        DrawPlayer(Player1);
        DrawPlayer(Player2);
        DrawBall(Ball0);
        break;
    }
}

void setup()
{
    // setup random seed
    randomSeed(analogRead(0));

    // setup game
    InitGame(0);
    GameState = State::MENU;

    // setup inputs
    pinMode(PIN_UP_PLAYER_1, INPUT);
    pinMode(PIN_DOWN_PLAYER_1, INPUT);
    pinMode(PIN_UP_PLAYER_2, INPUT);
    pinMode(PIN_DOWN_PLAYER_2, INPUT);

    // setup TVout
    TV.begin(PAL, 128, 96);
    TV.delay(2000);
}

void loop()
{
    switch (GameState) {
        case State::MENU:
        UpdateMenu();
        break;

        case State::RESET:
        UpdateReset();
        break;

        case State::PLAY:
        UpdatePlay();
        break;

        case State::SCORE:
        UpdateScore();
        break;
    }

    // render scene
    TV.clear_screen();

    DrawGame();

    TV.delay(60);
}
