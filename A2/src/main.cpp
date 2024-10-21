#include <iostream>
#include <cmath>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

using namespace std;

// Constants
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const int PLAYER_RADIUS = 20;
const int BALL_RADIUS = 10;
const int FIELD_WIDTH = 1100;
const int FIELD_HEIGHT = 600;
const int FIELD_X = (WINDOW_WIDTH - FIELD_WIDTH) / 2;
const int FIELD_Y = (WINDOW_HEIGHT - FIELD_HEIGHT - 30);
const int GOAL_WIDTH = 30;
const int GOAL_HEIGHT = 150;
const int GOAL_Y = (FIELD_HEIGHT - GOAL_HEIGHT) / 2;
const int SKILL_DURATION = 5000; // 5 sec
const int SKILL_COOLDOWN = 20000; // 20 sec
const float BOOST_MULTIPLIER = 1.5f; // Speed multiplier during skill
const int FPS = 60;
const int frameDelay = 1000 / FPS;
const int BALL_HOLDING_DISTANCE = PLAYER_RADIUS * 2 - 10; // Distance for holding ball

// Goal rectangles
SDL_Rect goal1 = {FIELD_X - 20, FIELD_Y + GOAL_Y, GOAL_WIDTH, GOAL_HEIGHT};
SDL_Rect goal2 = {FIELD_X + FIELD_WIDTH - GOAL_WIDTH + 20, FIELD_Y + GOAL_Y, GOAL_WIDTH, GOAL_HEIGHT};

// Ball structure
struct Ball {
    int x, y;
    int lastx, lasty;
    int dx, dy;
    float speed;
    SDL_Color color;
};

// Player structure
struct Player {
    int x, y;          
    int dx, dy;         
    float speed;      
    SDL_Color color;   
    bool holding = false; // true if player is holding the ball
};

// Team structure
struct Team {
    Player player1;     
    Player player2; 
    int control = 1; // Value: 1 or 2, defines which player is under control

    // Skill-related variables
    Uint32 skillTimer = 0; // Timer to track skill duration
    Uint32 skillCooldownTimer = 0; // Timer to track cooldown
    bool isSkillActive = false;
    bool isSkillOnCooldown = false;
};

// Initialize ball and teams
Ball ball = {WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30, 0, 0, 0, {255, 255, 255, 255}};
Team team1 = {
    {WINDOW_WIDTH / 2 - 100, WINDOW_HEIGHT / 2 + 30, 0, 0, 4, {232, 109, 82, 255}},
    {WINDOW_WIDTH / 2 - 500, WINDOW_HEIGHT / 2 + 30, 0, 0, 4, {232, 109, 82, 255}}
};
Team team2 = {
    {WINDOW_WIDTH / 2 + 100, WINDOW_HEIGHT / 2 + 30, 0, 0, 4, {85, 137, 227, 255}},
    {WINDOW_WIDTH / 2 + 500, WINDOW_HEIGHT / 2 + 30, 0, 0, 4, {85, 137, 227, 255}}
};

// Switch player control for a team
void switchPlayer(Team &team) {
    team.control = (team.control == 1) ? 2 : 1;
}

// Handle switch for team 1
void handlePlayerSwitchTeam1(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_Z]) {
        switchPlayer(team1);
    }
} 

// Handle switch for team 2
void handlePlayerSwitchTeam2(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_0]) {
        switchPlayer(team2);
    }
}

// Player movement
void movePlayer(Player &player, const Uint8 *keys, int upKey, int downKey, int leftKey, int rightKey) {
    int newX = player.x;
    int newY = player.y;

    // Move the player based on input
    bool move = false;
    if (keys[upKey] && player.y - player.speed >= FIELD_Y + PLAYER_RADIUS - 30) {
        newY -= player.speed;
        move = true;
    }
    if (keys[downKey] && player.y + player.speed <= FIELD_Y + FIELD_HEIGHT - PLAYER_RADIUS + 30) {
        newY += player.speed;
        move = true;
    }
    if (keys[leftKey] && player.x - player.speed >= FIELD_X + PLAYER_RADIUS - 30) {
        newX -= player.speed;
        move = true;
    }
    if (keys[rightKey] && player.x + player.speed <= FIELD_X + FIELD_WIDTH - PLAYER_RADIUS + 30) {
        newX += player.speed;
        move = true;
    }

    // Calculate the direction vector
    float directionX = newX - player.x;
    float directionY = newY - player.y;

    // Normalize the direction vector
    float length = sqrt(directionX * directionX + directionY * directionY);
    if (length > 0) {
        directionX /= length;
        directionY /= length;
    }

    // Update player position
    player.x = newX;
    player.y = newY;

    // If the player is holding the ball, move the ball to be in front of the player
    if (player.holding) {
        ball.x = player.x + directionX * BALL_HOLDING_DISTANCE;
        ball.y = player.y + directionY * BALL_HOLDING_DISTANCE;
        if (move) {
            ball.lastx = ball.x;
            ball.lasty = ball.y;
        }
        else {
            ball.x = ball.lastx;
            ball.y = ball.lasty;
        }
    }
}

// Move team 1's players
void moveTeam1(const Uint8 *keys) {
    Player *player = (team1.control == 1) ? &team1.player1 : &team1.player2; // Use pointer
    movePlayer(*player, keys, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D);
}

// Move team 2's players
void moveTeam2(const Uint8 *keys) {
    Player *player = (team2.control == 1) ? &team2.player1 : &team2.player2; // Use pointer
    movePlayer(*player, keys, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT);
}

// Steal ball function
void stealBall(Player &player, Team &opponent) {
    int dx = player.x - ball.x;
    int dy = player.y - ball.y;
    int distance = sqrt(dx * dx + dy * dy);

    if (distance < BALL_HOLDING_DISTANCE) {
        if (opponent.player1.holding) {
            player.holding = true; // Steal the ball
            opponent.player1.holding = false;
        } else if (opponent.player2.holding) {
            player.holding = true; // Steal the ball
            opponent.player2.holding = false;
        } else {
            player.holding = true; // No one is holding the ball
        }
    }
}

// Handle stealing for team 1
void stealTeam1(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_J]) {
        Player *player = (team1.control == 1) ? &team1.player1 : &team1.player2;
        stealBall(*player, team2);
    }
}

// Handle stealing for team 2
void stealTeam2(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_1]) {
        Player *player = (team2.control == 1) ? &team2.player1 : &team2.player2;
        stealBall(*player, team1);
    }
}

// Kick the ball function
void kickBall(Player &player) {
    if (player.holding) {
        player.holding = false;
        ball.dx = (ball.x - player.x);
        ball.dy = (ball.y - player.y);
        ball.speed = 2;
    }
}

// Handle kicking for team 1
void kickTeam1(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_K]) {
        Player *player = (team1.control == 1) ? &team1.player1 : &team1.player2; // Use pointer
        kickBall(*player);
    }
}

// Handle kicking for team 2
void kickTeam2(const Uint8 *keys) {
    if (keys[SDL_SCANCODE_2]) {
        Player *player = (team2.control == 1) ? &team2.player1 : &team2.player2; // Use pointer
        kickBall(*player);
    }
}

// Skill activation function
void activateSkill(Team &team, const Uint8 *keys) {
    Uint32 currentTime = SDL_GetTicks();

    // Check if the skill is on cooldown
    if (team.isSkillOnCooldown) {
        if (currentTime - team.skillCooldownTimer >= SKILL_COOLDOWN) {
            team.isSkillOnCooldown = false; // Reset cooldown
        } else {
            return; // Still on cooldown
        }
    }

    // Activate the skill
    if (keys[SDL_SCANCODE_L + (team.control - 1)] && !team.isSkillActive) {
        team.isSkillActive = true;
        team.skillTimer = currentTime;
        team.skillCooldownTimer = currentTime;
        team.isSkillOnCooldown = true;

        // Increase speed for both players
        team.player1.speed *= BOOST_MULTIPLIER;
        team.player2.speed *= BOOST_MULTIPLIER;
    }

    // Handle skill duration
    if (team.isSkillActive) {
        Uint32 elapsedTime = currentTime - team.skillTimer;

        // Gradually decrease speed
        if (elapsedTime >= SKILL_DURATION) {
            team.isSkillActive = false; // Skill ends
            team.player1.speed /= BOOST_MULTIPLIER;
            team.player2.speed /= BOOST_MULTIPLIER;
        }
    }
}

// Handle skill activation for team 1
void skillTeam1(const Uint8 *keys) {
    activateSkill(team1, keys);
}

// Handle skill activation for team 2
void skillTeam2(const Uint8 *keys) {
    activateSkill(team2, keys);
}

bool checkGoal(SDL_Rect &goal, Ball &ball) 
{
    if (ball.x - PLAYER_RADIUS < goal.x + goal.w &&
        ball.x + PLAYER_RADIUS > goal.x &&
        ball.y - PLAYER_RADIUS < goal.y + goal.h &&
        ball.y + PLAYER_RADIUS > goal.y) {
        return true;
    }
    return false;
}

// Handle ball movement
void moveBall() {
    int newX = ball.x + ball.dx * ball.speed;
    int newY = ball.y + ball.dy * ball.speed;

    // Check if the ball is in goal1 (left goal)
    if (checkGoal(goal1, ball)) {
        // Ball in goal1, skip border checks
        return;
    }
    
    // Check if the ball is in goal2 (right goal)
    if (checkGoal(goal2, ball)) {
        // Ball in goal2, skip border checks
        return;
    }

    // Border collision detection
    if (newX - BALL_RADIUS < FIELD_X || newX + BALL_RADIUS > FIELD_X + FIELD_WIDTH) {
        ball.dx *= -1; // Reverse horizontal direction
    } else {
        ball.x = newX;
    }

    if (newY - BALL_RADIUS < FIELD_Y || newY + BALL_RADIUS > FIELD_Y + FIELD_HEIGHT) {
        ball.dy *= -1; // Reverse vertical direction
    } else {
        ball.y = newY;
    }

    // Decrease ball speed
    ball.speed *= 0.85;

    // Stop the ball when speed is low
    if (ball.speed < 0.08) {
        ball.speed = 0;
    }
}

void drawCircle(SDL_Renderer *renderer, int centerX, int centerY, int radius)  {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

void drawPlayer(SDL_Renderer *renderer, Player &player) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    drawCircle(renderer, player.x, player.y, PLAYER_RADIUS + 3);

    SDL_SetRenderDrawColor(renderer, player.color.r, player.color.g, player.color.b, player.color.a);
    drawCircle(renderer, player.x, player.y, PLAYER_RADIUS);
}

void drawBall(SDL_Renderer *renderer, Ball &ball) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    drawCircle(renderer, ball.x, ball.y, BALL_RADIUS + 3);

    SDL_SetRenderDrawColor(renderer, ball.color.r, ball.color.g, ball.color.b, ball.color.a);
    drawCircle(renderer, ball.x, ball.y, BALL_RADIUS);
}

int main(int argc, char** argv) {
    // Init window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Tiny Football", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Init SDL_ttf
    if (TTF_Init() == -1) {
        cout << "TTF_Init: " << TTF_GetError() << endl;
        exit(2);
    }

    // Load font
    TTF_Font *font = TTF_OpenFont("./font/AldotheApache.ttf", 24);
    if (font == NULL) {
        cout << "TTF_OpenFont: " << TTF_GetError() << endl;
    }

    bool isRunning = true;
    int team1Score = 0, team2Score = 0; // Corrected score variable names
    Uint32 gameDuration = 300;
    Uint32 start = SDL_GetTicks();

    Uint32 frameStart, frameTime;  // Declared frameStart and frameTime

    while(isRunning) {
        frameStart = SDL_GetTicks();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                isRunning = false;
            }
        }

        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        handlePlayerSwitchTeam1(keys);
        handlePlayerSwitchTeam2(keys);
        moveTeam1(keys);
        moveTeam2(keys);
        stealTeam1(keys);
        stealTeam2(keys);
        kickTeam1(keys);
        kickTeam2(keys);
        skillTeam1(keys);
        skillTeam2(keys);
        moveBall();

        if (checkGoal(goal1, ball)) {
            team2Score++; 
            // Reset ball position and movement
            ball.x = WINDOW_WIDTH / 2;
            ball.y = WINDOW_HEIGHT / 2 + 30;
            ball.lastx = ball.x; 
            ball.lasty = ball.y;
            ball.dx = 0;
            ball.dy = 0;
            ball.speed = 0;

            // Reset players' positions
            team1.player1.x = WINDOW_WIDTH / 2 - 100;
            team1.player1.y = WINDOW_HEIGHT / 2 + 30;
            team1.player2.x = WINDOW_WIDTH / 2 - 500;
            team1.player2.y = WINDOW_HEIGHT / 2 + 30;
            team2.player1.x = WINDOW_WIDTH / 2 + 100;
            team2.player1.y = WINDOW_HEIGHT / 2 + 30;
            team2.player2.x = WINDOW_WIDTH / 2 + 500;
            team2.player2.y = WINDOW_HEIGHT / 2 + 30;
            team1.player1.holding = false;
            team1.player2.holding = false;
            team2.player1.holding = false;
            team2.player2.holding = false;
        }

        if (checkGoal(goal2, ball)) {
            team1Score++; 
            // Reset ball position and movement
            ball.x = WINDOW_WIDTH / 2;
            ball.y = WINDOW_HEIGHT / 2 + 30;
            ball.lastx = ball.x; 
            ball.lasty = ball.y;
            ball.dx = 0;
            ball.dy = 0;
            ball.speed = 0;

            // Reset players' positions
            team1.player1.x = WINDOW_WIDTH / 2 - 100;
            team1.player1.y = WINDOW_HEIGHT / 2 + 30;
            team1.player2.x = WINDOW_WIDTH / 2 - 500;
            team1.player2.y = WINDOW_HEIGHT / 2 + 30;
            team2.player1.x = WINDOW_WIDTH / 2 + 100;
            team2.player1.y = WINDOW_HEIGHT / 2 + 30;
            team2.player2.x = WINDOW_WIDTH / 2 + 500;
            team2.player2.y = WINDOW_HEIGHT / 2 + 30;
            team1.player1.holding = false;
            team1.player2.holding = false;
            team2.player1.holding = false;
            team2.player2.holding = false;
        }

        SDL_RenderClear(renderer);

        // Draw Field and goals
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect field_border = {FIELD_X - 3, FIELD_Y - 3, FIELD_WIDTH + 6, FIELD_HEIGHT + 6};
        SDL_RenderFillRect(renderer, &field_border);
        SDL_SetRenderDrawColor(renderer, 6, 129, 55, 255);
        SDL_Rect field = {FIELD_X, FIELD_Y, FIELD_WIDTH, FIELD_HEIGHT};
        SDL_RenderFillRect(renderer, &field);

        // Draw the center line with thickness
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Set color to white
        int centerX = FIELD_X + FIELD_WIDTH / 2; // Calculate the center X position
        int lineThickness = 3; // Desired thickness of the line

        // Define a rectangle for the line
        SDL_Rect centerLineRect = { centerX - lineThickness / 2, FIELD_Y, lineThickness, FIELD_HEIGHT };
        SDL_RenderFillRect(renderer, &centerLineRect); // Fill the rectangle to create a thick line

        // Draw players & ball
        drawPlayer(renderer, team1.player1);
        drawPlayer(renderer, team1.player2);
        drawPlayer(renderer, team2.player1);
        drawPlayer(renderer, team2.player2);
        drawBall(renderer, ball);

        // Draw goals
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &goal1);
        SDL_RenderFillRect(renderer, &goal2);

        // Background color
        SDL_SetRenderDrawColor(renderer, 79, 82, 115, 255);

        // Score display
        char score_text[50];
        sprintf(score_text, "Team 1: %d - Team 2: %d", team1Score, team2Score);  // Corrected variable names

        // Render the score text
        SDL_Color textColor = {0, 0, 0, 255};
        SDL_Surface* scoreSurface = TTF_RenderText_Solid(font, score_text, textColor);
        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
        SDL_Rect scoreRect = {WINDOW_WIDTH / 2 - scoreSurface->w / 2, 30, scoreSurface->w, scoreSurface->h};
        SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
        SDL_FreeSurface(scoreSurface);
        SDL_DestroyTexture(scoreTexture);

        // Timer display
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = (now - start) / 1000;
        Uint32 remaining = gameDuration - elapsed;
        string timerText = "Time remaining: " + to_string(remaining) + " seconds";
        SDL_Surface* timerSurface = TTF_RenderText_Solid(font, timerText.c_str(), textColor);
        SDL_Texture* timerTexture = SDL_CreateTextureFromSurface(renderer, timerSurface);
        SDL_Rect timerRect = {5, 5, timerSurface->w, timerSurface->h};
        SDL_RenderCopy(renderer, timerTexture, NULL, &timerRect);
        SDL_FreeSurface(timerSurface);
        SDL_DestroyTexture(timerTexture);

        // End game conditions
        if (remaining <= 0) {
            isRunning = false;
            string message;
            if (team1Score > team2Score) {
                message = "Team 1 wins!";
            } else if (team2Score > team1Score) {
                message = "Team 2 wins!";
            } else {
                message = "Draw!";
            }
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Game Over", message.c_str(), NULL);
        }

        SDL_RenderPresent(renderer);

        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Cleanup
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}