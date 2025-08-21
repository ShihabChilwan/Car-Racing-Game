#include <graphics.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <bios.h>
#include <string.h>
#include <stdlib.h>  // For rand() function

#define LEFT_LANE 230
#define MIDDLE_LANE 330
#define RIGHT_LANE 430
#define LEFT_KEY 75   // Scan code for left arrow
#define RIGHT_KEY 77  // Scan code for right arrow

// ===== GLOBAL VARIABLES =====
// 1. Game State & Settings
int sound_active = 1;
int game_over = 0;
int lives = 3;
int score = 0;
int invulnerable = 0;
int push_back_distance = 0;
int road_push_offset = 0;
int current_lane = 1;
int score_paused = 0;     // Tracks if score incrementing is paused
int score_pause_timer = 0; // Timer for how long the score remains paused
#define SCORE_PAUSE_DURATION 30

// 2. Player Car
int player_car_x = MIDDLE_LANE;
int player_car_y = 350;
int prev_player_car_x = player_car_x;
int prev_player_car_y = player_car_y;

// 3. Enemy Cars
int car2_x = 330, car2_y = 450;
int car3_x = 430, car3_y = -40;
int car4_x = 230, car4_y = 20;
int car5_x = 330, car5_y = 180;
int car6_x = 430, car6_y = 300;
int prev_car2_y = car2_y;
int prev_car3_y = car3_y;
int prev_car4_y = car4_y;
int prev_car5_y = car5_y;
int prev_car6_y = car6_y;

// 4. Speed & Movement
int base_speed = 7;
float user_speed_multiplier = 1.0;
float max_speed_multiplier = 10.0;
float speed_increase_rate = 0.005;
int effective_speed;
int divider_offset = 0;

// 5. Sound
int startFreq = 50;
int endFreq = 80;
int steps = 100;
int totalDuration = 5000;
int sound_delay = totalDuration / steps;
float stepIncrement = (float)(endFreq - startFreq) / steps;
int current_step = 0;
int sound_timer = 0;

// 6. Animation
int game_timer = 0;
int animation_frame = 0;

// 7. Coins
#define MAX_COINS 5
int coin_x[MAX_COINS];
int coin_y[MAX_COINS];
int prev_coin_y[MAX_COINS];
int coin_active[MAX_COINS];  // 1 if coin is visible, 0 if collected
int coins_collected = 0;     // Counter for collected coins
int coin_respawn_timer[MAX_COINS]; // Timer for respawning coins
#define COIN_RESPAWN_TIME 150  // How long before a collected coin respawns

// ===== Shield Power-Up =====
int shield_active = 0;
int shield_timer = 0;
#define SHIELD_DURATION 500   // 1000 ticks of 10ms = 10 seconds
#define BLINK_INTERVAL 3       // blink toggle every 5 ticks (50 ms)

// ===== FUNCTION DECLARATIONS =====
void playCarSound();
void drawWave(int x, int y, int size, int color);
void drawWaves(int yPos, int frame);
void drawSeascape(int frame);
void drawBackground();
void updateLaneDividers(int offset);
int sX(int offset, int baseX, float scale);
int sY(int offset, int baseY, float scale);
void displayStats(int score, int speed, int lives);
void playCrashSound();
int checkCollision(int player_x, int player_y, int other_x, int other_y, float car_width, float car_height);
void countdown(int seconds);
void initializeCoins();
void drawCoin(int x, int y);
int checkCoinCollision(int player_x, int player_y, int coin_x, int coin_y);
void updateCoins(int speed);

// Car-drawing functions declarations (implementations unchanged):
void drawCarBody(int baseX, int baseY, float scale, int bodycolor);
void drawBonnet(int baseX, int baseY, float scale, int bonnetColor);
void drawHeadlights(int baseX, int baseY, float scale);
void drawWindow(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
	  int baseX, int baseY, float scale, int fillColor, int fillX, int fillY);
void drawWindows(int baseX, int baseY, float scale, int windshieldColor);
void drawRoof(int baseX, int baseY, float scale);
void drawTyres(int baseX, int baseY, float scale);
void drawRearWindshield(int baseX, int baseY, float scale, int windshieldColor);
void drawBoot(int baseX, int baseY, float scale);
void drawCar(int baseX, int baseY, float scale, int bonnetColor, int windshieldColor, int bodycolor);

// ===== MAIN GAME CODE =====
int main() {
    int gd = DETECT, gm;
    initgraph(&gd, &gm, "C:\\TurboC3\\BGI");

    countdown(5);
    setbkcolor(BLACK);
    settextstyle(DEFAULT_FONT, HORIZ_DIR, 1);
    settextjustify(LEFT_TEXT, TOP_TEXT);

    drawBackground();
    initializeCoins();

    while (!game_over) {
	animation_frame++;

	drawSeascape(animation_frame);

	// Sound management
	sound_timer++;
	if (sound_active) {
	    if (sound_timer >= 5) {
		playCarSound();
		sound_timer = 0;
		int speedRange = (int)(user_speed_multiplier * 10);
		if (speedRange > 0) {
		    endFreq = 80 + speedRange;
		    stepIncrement = (float)(endFreq - startFreq) / steps;
		}
	    }
	} else {
	    nosound();
	}

	// Keyboard input
	if (bioskey(1)) {
	    int key = bioskey(0);
	    int scan_code = (key >> 8) & 0xFF;

	    if (scan_code == LEFT_KEY && current_lane > 0) {
		current_lane--;
		player_car_x = (current_lane == 0) ? LEFT_LANE : MIDDLE_LANE;
	    } else if (scan_code == RIGHT_KEY && current_lane < 2) {
		current_lane++;
		player_car_x = (current_lane == 2) ? RIGHT_LANE : MIDDLE_LANE;
	    }

	    if ((key & 0xFF) == 's' || (key & 0xFF) == 'S') {
		sound_active = !sound_active;
		if (!sound_active) nosound();
	    }

	    if (scan_code == 1) {
		sound_active = 0;
		nosound();
		game_over = 1;
	    }

	    // Activate shield on Spacebar
	    if ((key & 0xFF) == ' ' && !shield_active) {
		if (coins_collected >= 10) {
		    coins_collected -= 10;
		    shield_active = 1;
		    shield_timer = SHIELD_DURATION;
		} else {
		    setcolor(WHITE);
		    outtextxy(20, 100, "Shield Unavailable");
		}
	    }
	}

	// Erase previous coins
	for (int i = 0; i < MAX_COINS; i++) {
	    if (coin_active[i]) {
		setfillstyle(SOLID_FILL, DARKGRAY);
		bar(coin_x[i] - 10, prev_coin_y[i] - 40, coin_x[i] + 10, prev_coin_y[i] + 40);
		prev_coin_y[i] = coin_y[i];
	    }
	}

	// Erase previous frames of cars...
	setfillstyle(SOLID_FILL, DARKGRAY);
	bar(295, 0, 305, 500);
	bar(395, 0, 405, 500);
	bar(prev_player_car_x-3, prev_player_car_y-4, prev_player_car_x+53, prev_player_car_y+115);
	bar(car2_x-3, prev_car2_y-4, car2_x+53, prev_car2_y+115);
	bar(car3_x-3, prev_car3_y-4, car3_x+53, prev_car3_y+115);
	bar(car4_x-3, prev_car4_y-4, car4_x+53, prev_car4_y+115);
	bar(car5_x-3, prev_car5_y-4, car5_x+53, prev_car5_y+115);
	bar(car6_x-3, prev_car6_y-4, car6_x+53, prev_car6_y+115);

	// Store current positions
	prev_player_car_x = player_car_x;
	prev_player_car_y = player_car_y;
	prev_car2_y = car2_y;
	prev_car3_y = car3_y;
	prev_car4_y = car4_y;
	prev_car5_y = car5_y;
	prev_car6_y = car6_y;

	// Speed management
	if (user_speed_multiplier < max_speed_multiplier) {
	    user_speed_multiplier += speed_increase_rate;
	}
	effective_speed = (int)(base_speed * user_speed_multiplier);

	// Update car positions
	car2_y += effective_speed;
	car3_y += effective_speed;
	car4_y += effective_speed;
	car5_y += effective_speed;
	car6_y += effective_speed;

	if (car2_y > 550) car2_y = -120;
	if (car3_y > 550) car3_y = -120;
	if (car4_y > 550) car4_y = -120;
	if (car5_y > 550) car5_y = -120;
	if (car6_y > 550) car6_y = -120;

	// Update coin positions
	updateCoins(effective_speed);

	// Update shield timer
	if (shield_active) {
	    if (--shield_timer <= 0) shield_active = 0;
	}

	divider_offset = (divider_offset + effective_speed + 20) % 20;

	// Collision detection (skip if shield_active)
	if (!shield_active && invulnerable <= 0) {
	    if (checkCollision(player_car_x, player_car_y, car2_x, car2_y, 0.5, 0.5) ||
		checkCollision(player_car_x, player_car_y, car3_x, car3_y, 0.5, 0.5) ||
		checkCollision(player_car_x, player_car_y, car4_x, car4_y, 0.5, 0.5) ||
		checkCollision(player_car_x, player_car_y, car5_x, car5_y, 0.5, 0.5) ||
		checkCollision(player_car_x, player_car_y, car6_x, car6_y, 0.5, 0.5)) {
		lives--;
		if (sound_active) playCrashSound();
		user_speed_multiplier *= 0.6;
		if (user_speed_multiplier < 1.0) user_speed_multiplier = 1.0;
		invulnerable = 50;

		// Pause score incrementing when crash happens
		score_paused = 1;
		score_pause_timer = SCORE_PAUSE_DURATION;

		if (lives <= 0) {
		    sound_active = 0;
		    nosound();
		    game_over = 1;
		}
	    }
	} else if (invulnerable > 0) {
	    invulnerable--;
	}

	// Coin collection detection
	for (i = 0; i < MAX_COINS; i++) {
	    if (coin_active[i] && checkCoinCollision(player_car_x, player_car_y, coin_x[i], coin_y[i])) {
		coin_active[i] = 0;
		coins_collected++;
		coin_respawn_timer[i] = COIN_RESPAWN_TIME;
		if (sound_active) { sound(1500); delay(50); nosound(); }
	    }
	}

	// Handle score pause timer
	if (score_paused) {
	    if (score_pause_timer > 0) score_pause_timer--;
	    else score_paused = 0;
	}

	// Only increment score if not paused
	if (!score_paused) score += (int)(user_speed_multiplier);
	game_timer++;

	// Draw updated elements
	updateLaneDividers(divider_offset);

	// Draw player car with shield blink/invulnerable flash
	if (shield_active) {
	    if (((shield_timer / BLINK_INTERVAL) % 2) == 0) {
		drawCar(player_car_x, player_car_y, 0.5, DARKGRAY, RED, BLUE);
		//setcolor(RED);
		//circle(player_car_x,player_car_y,20);
	    }
	} else if (invulnerable <= 0 || (invulnerable % 5 >= 3)) {
	    drawCar(player_car_x, player_car_y, 0.5, DARKGRAY, RED, BLUE);
	}

	// Draw enemy cars
	drawCar(car2_x, car2_y, 0.5, GREEN, MAGENTA, BROWN);
	drawCar(car3_x, car3_y, 0.5, CYAN, YELLOW, LIGHTBLUE);
	drawCar(car4_x, car4_y, 0.5, DARKGRAY, GREEN, CYAN);
	drawCar(car5_x, car5_y, 0.5, RED, MAGENTA, BLACK);
	drawCar(car6_x, car6_y, 0.5, BLUE, BROWN, LIGHTBLUE);

	// Draw coins
	for (i = 0; i < MAX_COINS; i++) {
	    if (coin_active[i]) drawCoin(coin_x[i], coin_y[i]);
	}

	displayStats(score, (int)(user_speed_multiplier * 50), lives);

	// Display shield timer
	if (shield_active) {
	    char buf[32];
	    int secs_left = (shield_timer + 99) / 100;
	    sprintf(buf, "SHIELD: %2ds", secs_left);
	    outtextxy(20, 100, buf);
	}

	if (!sound_active) outtextxy(20, 80, "SOUND: OFF");
	else              outtextxy(20, 80, "SOUND: ON");

	delay(10);
    }

    // Game over screen
    setfillstyle(SOLID_FILL, BLACK);
    bar(0, 0, getmaxx(), getmaxy());

    setcolor(RED);
    settextstyle(TRIPLEX_FONT, HORIZ_DIR, 5);
    outtextxy(210, 150, "GAME OVER");

    settextstyle(TRIPLEX_FONT, HORIZ_DIR, 3);
    char str[80];
    sprintf(str, "Final Score: %d", score);
    outtextxy(230, 250, str);
    sprintf(str, "Coins Collected: %d", coins_collected);
    outtextxy(230, 300, str);

    setcolor(WHITE);
    outtextxy(220, 450, "Press any key to exit");
    nosound();
    getch();
    closegraph();
    return 0;
}


void playCarSound() {
    if (!sound_active) {
	nosound();
	return;
    }
    int freq = startFreq + (int)(stepIncrement * current_step);
    sound(freq);
    current_step = (current_step + 3) % steps;
    if (current_step == 0) {
	nosound();
    }
}

void playCrashSound() {
    if (!sound_active) return;

    for (int i = 0; i < 3; ++i) {
	sound(1000);
	delay(40);
	nosound();
	delay(40);
    }
}

void drawWave(int x, int y, int size, int color) {
    int points[8] = {x, y, x + size/2, y - size/4, x + size, y, x + size + size/2, y + size/4};
    setcolor(color);
    drawpoly(4, points);
}

void drawWaves(int yPos, int frame) {
    int waveColor1 = LIGHTCYAN, waveColor2 = CYAN;
    int offset = frame % 20;
    for (int y = yPos; y < 500; y += 90) {
        for (int x = 20; x < 165; x += 40) drawWave(x - offset, y, 20, waveColor1);
	for (x = 0; x < 160; x += 40) drawWave(x + 20 - ((frame + 10) % 20), y + 20, 20, waveColor2);
    }
    // Right side waves (same logic as left side)
    for (y = yPos; y < 500; y += 90) {
        for (int x = 525; x < 650; x += 40) drawWave(x - offset, y, 20, waveColor1);
	for (x = 505; x < 650; x += 40) drawWave(x + 20 - ((frame + 10) % 20), y + 20, 20, waveColor2);
    }
}

// Helper function to draw a filled rectangle
void drawRectangle(int x1, int y1, int x2, int y2, int fillColor) {
    setfillstyle(SOLID_FILL, fillColor);
    bar(x1, y1, x2, y2);
}

void drawSeascape(int frame) {
    drawRectangle(0, 150, 200, 500, LIGHTBLUE);
    drawRectangle(505, 90, 650, 500, LIGHTBLUE);
    drawWaves(0, frame);
}

void drawBackground() {
    drawRectangle(200, 0, 500, 500, DARKGRAY);
    setcolor(WHITE);
    setlinestyle(SOLID_LINE, 0, 1);
    drawRectangle(200, 0, 205, 500, WHITE);
    drawRectangle(500, 0, 505, 500, WHITE);
    drawRectangle(0, 0, 200, 500, LIGHTBLUE);
    drawRectangle(505, 0, 650, 500, LIGHTBLUE);
}

void updateLaneDividers(int offset) {
    setcolor(YELLOW);
    setlinestyle(DASHED_LINE, 3, 3);
    for (int y = (20 + offset - road_push_offset) % 20; y < 500; y += 20) {
        line(400, y, 400, y + 15);
        line(300, y, 300, y + 15);
    }
    setlinestyle(SOLID_LINE, 0, 1);
}

int sX(int offset, int baseX, float scale) {
    return baseX + (int)(offset * scale);
}

int sY(int offset, int baseY, float scale) {
    return baseY + (int)(offset * scale);
}

void drawCarBody(int baseX, int baseY, float scale, int bodycolor) {
    setcolor(WHITE);
    rectangle(sX(0, baseX, scale), sY(0, baseY, scale), sX(50, baseX, scale), sY(115, baseY, scale));
    setfillstyle(SOLID_FILL, bodycolor);
    floodfill(sX(30, baseX, scale), sY(60, baseY, scale), WHITE);
}

void drawBonnet(int baseX, int baseY, float scale, int bonnetColor) {
    line(sX(13, baseX, scale), sY(4, baseY, scale), sX(10, baseX, scale), sY(20, baseY, scale));
    line(sX(36, baseX, scale), sY(4, baseY, scale), sX(41, baseX, scale), sY(20, baseY, scale));
    line(sX(13, baseX, scale), sY(4, baseY, scale), sX(37, baseX, scale), sY(4, baseY, scale));
    line(sX(10, baseX, scale), sY(20, baseY, scale), sX(41, baseX, scale), sY(20, baseY, scale));
    setfillstyle(SOLID_FILL, bonnetColor);
    floodfill(sX(20, baseX, scale), sY(10, baseY, scale), WHITE);
}

void drawHeadlights(int baseX, int baseY, float scale) {
    rectangle(sX(4, baseX, scale), sY(-4, baseY, scale), sX(15, baseX, scale), sY(0, baseY, scale));
    setfillstyle(SOLID_FILL, YELLOW);
    floodfill(sX(10, baseX, scale), sY(-2, baseY, scale), WHITE);
    rectangle(sX(34, baseX, scale), sY(-4, baseY, scale), sX(45, baseX, scale), sY(0, baseY, scale));
    setfillstyle(SOLID_FILL, YELLOW);
    floodfill(sX(40, baseX, scale), sY(-2, baseY, scale), WHITE);
}

void drawWindow(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                int baseX, int baseY, float scale, int fillColor, int fillX, int fillY) {
    line(sX(x1, baseX, scale), sY(y1, baseY, scale), sX(x2, baseX, scale), sY(y2, baseY, scale));
    line(sX(x2, baseX, scale), sY(y2, baseY, scale), sX(x3, baseX, scale), sY(y3, baseY, scale));
    line(sX(x3, baseX, scale), sY(y3, baseY, scale), sX(x4, baseX, scale), sY(y4, baseY, scale));
    line(sX(x4, baseX, scale), sY(y4, baseY, scale), sX(x1, baseX, scale), sY(y1, baseY, scale));

    setfillstyle(SOLID_FILL, fillColor);
    floodfill(sX(fillX, baseX, scale), sY(fillY, baseY, scale), WHITE);
}

void drawWindows(int baseX, int baseY, float scale, int windshieldColor) {
    // Left and right side windows
    drawWindow(6, 20, 10, 40, 10, 80, 6, 100, baseX, baseY, scale, BLUE, 8, 50);
    drawWindow(45, 20, 40, 40, 40, 80, 45, 100, baseX, baseY, scale, BLUE, 42, 50);

    // Window split lines
    line(sX(6, baseX, scale), sY(60, baseY, scale), sX(10, baseX, scale), sY(60, baseY, scale));
    line(sX(41, baseX, scale), sY(60, baseY, scale), sX(45, baseX, scale), sY(60, baseY, scale));

    // Front windshield
    drawWindow(7, 20, 10, 40, 40, 40, 44, 20, baseX, baseY, scale, windshieldColor, 15, 30);
}

void drawRoof(int baseX, int baseY, float scale) {
    rectangle(sX(10, baseX, scale), sY(40, baseY, scale), sX(40, baseX, scale), sY(80, baseY, scale));
    setfillstyle(SOLID_FILL, LIGHTGRAY);
    floodfill(sX(30, baseX, scale), sY(60, baseY, scale), WHITE);
}

void drawTyre(int x1, int y1, int x2, int y2, int baseX, int baseY, float scale) {
    rectangle(sX(x1, baseX, scale), sY(y1, baseY, scale),
              sX(x2, baseX, scale), sY(y2, baseY, scale));
    floodfill(sX((x1 + x2) / 2, baseX, scale),
              sY((y1 + y2) / 2, baseY, scale), WHITE);
}

void drawTyres(int baseX, int baseY, float scale) {
    setfillstyle(SOLID_FILL, BLACK);

    // Draw 4 tyres
    drawTyre(-3, 15, 2, 35, baseX, baseY, scale);   // Front left
    drawTyre(-3, 75, 2, 95, baseX, baseY, scale);   // Rear left
    drawTyre(49, 15, 53, 35, baseX, baseY, scale);  // Front right
    drawTyre(49, 75, 53, 95, baseX, baseY, scale);  // Rear right
}

void drawRearWindshield(int baseX, int baseY, float scale, int windshieldColor) {
    int rearWindshield[] = {
        sX(7, baseX, scale),  sY(100, baseY, scale),
        sX(10, baseX, scale), sY(80, baseY, scale),
        sX(40, baseX, scale), sY(80, baseY, scale),
        sX(44, baseX, scale), sY(100, baseY, scale),
        sX(7, baseX, scale),  sY(100, baseY, scale)  // Closing point
    };

    drawpoly(5, rearWindshield);

    setfillstyle(SOLID_FILL, windshieldColor);
    floodfill(sX(15, baseX, scale), sY(90, baseY, scale), WHITE);
}

void drawBoot(int baseX, int baseY, float scale) {
    rectangle(sX(11, baseX, scale), sY(107, baseY, scale),
              sX(40, baseX, scale), sY(115, baseY, scale));
    setfillstyle(SOLID_FILL, DARKGRAY);
    floodfill(sX(14, baseX, scale), sY(110, baseY, scale), WHITE);
}

void drawCar(int baseX, int baseY, float scale, int bonnetColor, int windshieldColor, int bodyColor) {
    setfillstyle(SOLID_FILL, bodyColor);
    drawCarBody(baseX, baseY, scale, bodyColor);

    setfillstyle(SOLID_FILL, bonnetColor);
    drawBonnet(baseX, baseY, scale, bonnetColor);

    setfillstyle(SOLID_FILL, windshieldColor);
    drawWindows(baseX, baseY, scale, windshieldColor);
    drawRearWindshield(baseX, baseY, scale, windshieldColor);

    drawRoof(baseX, baseY, scale);
    drawHeadlights(baseX, baseY, scale);
    drawTyres(baseX, baseY, scale);
    drawBoot(baseX, baseY, scale);
}

void displayStats(int score, int speed, int lives) {
    char str[80];
    setfillstyle(SOLID_FILL, LIGHTBLUE);
    bar(0, 0, 199, 150);
    setcolor(LIGHTRED);
    sprintf(str, "SCORE: %d", score);
    outtextxy(20, 20, str);
    sprintf(str, "SPEED: %d", speed);
    outtextxy(20, 40, str);
    sprintf(str, "COINS: %d", coins_collected);  // Display coins collected
    outtextxy(20, 60, str);
    setfillstyle(SOLID_FILL, LIGHTBLUE);
    bar(505, 0, 650, 90);
    setcolor(LIGHTRED);
    settextstyle(TRIPLEX_FONT, HORIZ_DIR, 1);
    sprintf(str, "LIVES: %d", lives);
    outtextxy(540, 20, str);
    setcolor(WHITE);
}

int checkCollision(int player_x, int player_y, int other_x, int other_y, float car_width, float car_height) {
    int collision_margin = 5;
    return (player_x + (int)(50 * car_width) - collision_margin > other_x &&
            player_x < other_x + (int)(50 * car_width) - collision_margin &&
            player_y + (int)(115 * car_height) - collision_margin > other_y &&
            player_y < other_y + (int)(115 * car_height) - collision_margin);
}

void countdown(int seconds) {
    char buf[8];
    settextjustify(CENTER_TEXT, CENTER_TEXT);
    settextstyle(10, 0, 9);

    for (int i = seconds; i >= 0; --i) {
        if (i > 3) setbkcolor(RED);
        else if (i > 1) { setbkcolor(YELLOW); setcolor(BLUE); }
        else setbkcolor(LIGHTGREEN);

        cleardevice();

        if (i > 0) {
            sprintf(buf, "%d", i);
            outtextxy(getmaxx()/2, getmaxy()/2, buf);

            sound(500);
            delay(100);
            nosound();
            delay(900);
        } else {
            strcpy(buf, "GO!");
	    outtextxy(getmaxx()/2, getmaxy()/2, buf);

            for (int j = 0; j < 3; ++j) {
                sound(1000);
                delay(100);
                nosound();
                delay(100);
	    }
	    delay(500);
	}
    }

    cleardevice();
}

// Initialize coin positions
void initializeCoins() {
    // Set up initial positions for coins
    int lanes[3] = {LEFT_LANE-12, MIDDLE_LANE-12, RIGHT_LANE-12};

    for (int i = 0; i < MAX_COINS; i++) {
	// Randomly assign a lane for each coin
	coin_x[i] = lanes[i % 3] + 25;  // Center in lane

	// Space them out vertically
	coin_y[i] = -200 - (i * 150);
	prev_coin_y[i] = coin_y[i];

	// All coins are initially active
	coin_active[i] = 1;

	// Initialize respawn timers
	coin_respawn_timer[i] = 0;
    }
}

// Draw a coin at the specified position
void drawCoin(int x, int y) {
    // Draw a yellow circle for the coin
    setcolor(YELLOW);
    setfillstyle(SOLID_FILL, YELLOW);

    // Draw outer circle
    circle(x, y, 8);
    //setcolor(YELLOW);
    floodfill(x+1, y+1, YELLOW);

    // Draw inner details to make it look like a coin
    setcolor(BROWN);
    circle(x, y, 5);
}

// Check if player car collides with a coin
int checkCoinCollision(int player_x, int player_y, int coin_x, int coin_y) {
    // The coin is smaller than cars, so we use a different collision box
    int player_width = 50;
    int player_height = 115;
    int coin_radius = 8;

    // Check if the coin overlaps with the car
    return (player_x < (coin_x + coin_radius) &&
	    (player_x + player_width) > (coin_x - coin_radius) &&
	    player_y < (coin_y + coin_radius) &&
	    (player_y + player_height) > (coin_y - coin_radius));
}

// Update coin positions and manage respawning
void updateCoins(int speed) {
    for (int i = 0; i < MAX_COINS; i++) {
	// If coin is active, move it down
	if (coin_active[i]) {
	    coin_y[i] += speed;

	    // If coin goes off screen, reset it to top
	    if (coin_y[i] > 550) {
		coin_y[i] = -50 - (rand() % 100);

		// Randomly assign a lane
		int lane = rand() % 3;
		switch (lane) {
		    case 0: coin_x[i] = LEFT_LANE + 15; break;
		    case 1: coin_x[i] = MIDDLE_LANE + 15; break;
		    case 2: coin_x[i] = RIGHT_LANE + 15; break;
		}
	    }
	}
	// If coin is inactive (collected), handle respawn timer
	else {
	    if (coin_respawn_timer[i] > 0) {
		coin_respawn_timer[i]--;
	    } else {
		// Respawn the coin at the top of the screen
		coin_active[i] = 1;
		coin_y[i] = -50 - (rand() % 100);

		// Randomly assign a lane
		int lane = rand() % 3;
                switch (lane) {
                    case 0: coin_x[i] = LEFT_LANE + 15; break;
                    case 1: coin_x[i] = MIDDLE_LANE + 15; break;
                    case 2: coin_x[i] = RIGHT_LANE + 15; break;
                }
            }
        }
    }
}
