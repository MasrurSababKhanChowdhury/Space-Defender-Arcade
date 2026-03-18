#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>
#include <cstdio>
#include <fstream>
#include <algorithm> // Required for std::remove_if

// Window dimensions
const int WIDTH = 800;
const int HEIGHT = 600;

// Game states
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
GameState gameState = MENU;

// Player properties
struct Player {
    float x, y;
    float size;
    float speed;
    int lives;
    int score;
} player;

// Bullet structure
struct Bullet {
    float x, y;
    float speed;
    bool active;
};

// Enemy structure
struct Enemy {
    float x, y;
    float speed;
    bool active;
    int type; // 0: circle, 1: triangle, 2: square
};

// Power-up structure
struct PowerUp {
    float x, y;
    float speed;
    bool active;
};

// Particle effect structure (Arcade explosions)
struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    float r, g, b;
    bool active;
};

// Floating text feedback (Score popups)
struct FloatingText {
    float x, y;
    float vy;
    float life;
    float maxLife;
    float r, g, b;
    std::string text;
    bool active;
};

// Vector dynamic data type (Containers for game objects)
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;
std::vector<PowerUp> powerUps;
std::vector<Particle> particles;
std::vector<FloatingText> floatingTexts;

// Animation variables
float starOffset = 0;
float enemySpawnTimer = 0;
float powerUpTimer = 0;

// === GAME LEVEL & DIFFICULTY VARIABLES ===
float gameLevelTimer = 0;
int currentLevel = 1;
int enemySpawnRate = 60; // Initial rate: 60 frames = 1 enemy/second
float lastHitTimer = 0;  // Timer for no-hit penalty
float levelBannerTimer = 0;
float damageFlashTimer = 0;
float shakeTimer = 0;
float shakeIntensity = 0;
int comboHits = 0;
float comboTimer = 0;
int comboMultiplier = 1;
int highScore = 0;
float shootCooldownTimer = 0;
float weaponFlashTimer = 0;
const int MAX_LEVEL = 10;
float levelDifficultyMultiplier = 1.0f;
int powerUpSpawnInterval = 300;
const char* HIGH_SCORE_FILE = "highscore.dat";

// Controls
bool keys[256] = { false };

// --- Function Prototypes ---
void drawCircleMidpoint(float cx, float cy, float r);
void drawFilledCircle(float cx, float cy, float r);
void drawLineBresenham(int x1, int y1, int x2, int y2);
void drawText(float x, float y, const char* text);
void drawTextSmall(float x, float y, const char* text);
void drawTextSmallShadow(float x, float y, const char* text, float r, float g, float b);
void drawTextShadow(float x, float y, const char* text, float r, float g, float b);
int getTextWidth(void* font, const char* text);
void drawTextShadowCentered(float centerX, float y, const char* text, float r, float g, float b);
void drawTextSmallShadowCentered(float centerX, float y, const char* text, float r, float g, float b);
void drawStars();
void drawBackground();
void drawParticles();
void drawFloatingTexts();
void drawArcadeOverlay();
void drawNeonPanel(float x, float y, float w, float h, float r, float g, float b, float alpha);
void spawnExplosion(float x, float y, float r, float g, float b, int count);
void spawnFloatingText(float x, float y, const std::string& text, float r, float g, float b);
void update(int value);
void startNewGame();
void fireBullet();
const char* getRankByScore(int score);
void loadHighScore();
void saveHighScore();

float getSquaredDistance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

float getEnemyCollisionRadius(int enemyType) {
    switch (enemyType) {
    case 0: return 15.0f; // Circle
    case 1: return 18.0f; // Triangle
    case 2: return 17.0f; // Square
    default: return 20.0f; // Diamond
    }
}

float getRandomFloat(float minValue, float maxValue) {
    float normalized = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minValue + normalized * (maxValue - minValue);
}

const char* getRankByScore(int score) {
    if (score >= 600) return "LEGEND";
    if (score >= 350) return "ACE";
    if (score >= 180) return "VETERAN";
    if (score >= 80) return "ROOKIE+";
    return "ROOKIE";
}

void loadHighScore() {
    std::ifstream inFile(HIGH_SCORE_FILE);
    if (inFile.good()) {
        inFile >> highScore;
        if (highScore < 0) highScore = 0;
    }
}

void saveHighScore() {
    std::ofstream outFile(HIGH_SCORE_FILE, std::ios::trunc);
    if (outFile.good()) {
        outFile << highScore;
    }
}


// Initialize game
void init() {
    glClearColor(0.0, 0.0, 0.1, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Set 2D coordinates: (0, 0) is bottom-left
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);

    // Enable alpha blending for arcade effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize player
    player.x = WIDTH / 2;
    player.y = 50;
    player.size = 20;
    player.speed = 5.0f;
    player.lives = 3;
    player.score = 0;

    srand(time(NULL));
    loadHighScore();
}

// DDA Line Algorithm (Not used for primary drawing, but kept for completeness)
void drawLineDDA(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float steps = fabs(dx) > fabs(dy) ? fabs(dx) : fabs(dy);

    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = x1, y = y1;

    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        glVertex2f(round(x), round(y));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

// Bresenham's Line Algorithm (Used for Ship Wings)
void drawLineBresenham(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    glBegin(GL_POINTS);
    while (true) {
        glVertex2i(x1, y1);

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    glEnd();
}

// Midpoint Circle Algorithm (Used for Ship Cockpit and Enemy Outlines)
void drawCircleMidpoint(float cx, float cy, float r) {
    int x = 0;
    int y = r;
    int d = 1 - r;

    glBegin(GL_POINTS);

    auto plotCirclePoints = [&](int x, int y) {
        glVertex2f(cx + x, cy + y);
        glVertex2f(cx - x, cy + y);
        glVertex2f(cx + x, cy - y);
        glVertex2f(cx - x, cy - y);
        glVertex2f(cx + y, cy + x);
        glVertex2f(cx - y, cy + x);
        glVertex2f(cx + y, cy - x);
        glVertex2f(cx - y, cy - x);
        };

    while (x <= y) {
        plotCirclePoints(x, y);
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        }
        else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
    glEnd();
}

// Filled circle (Used for Life Icons, Power-ups, and Circular Enemy Body)
void drawFilledCircle(float cx, float cy, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.14159 / 180;
        glVertex2f(cx + r * cos(angle), cy + r * sin(angle));
    }
    glEnd();
}

// Draw text (Used for HUD and Menu Screens)
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void drawTextSmall(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
}

void drawTextSmallShadow(float x, float y, const char* text, float r, float g, float b) {
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    drawTextSmall(x + 2, y - 2, text);
    glColor3f(r, g, b);
    drawTextSmall(x, y, text);
}

void drawTextShadow(float x, float y, const char* text, float r, float g, float b) {
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    drawText(x + 2, y - 2, text);
    glColor3f(r, g, b);
    drawText(x, y, text);
}

int getTextWidth(void* font, const char* text) {
    int width = 0;
    for (const char* c = text; *c != '\0'; c++) {
        width += glutBitmapWidth(font, *c);
    }
    return width;
}

void drawTextShadowCentered(float centerX, float y, const char* text, float r, float g, float b) {
    int width = getTextWidth(GLUT_BITMAP_HELVETICA_18, text);
    drawTextShadow(centerX - width * 0.5f, y, text, r, g, b);
}

void drawTextSmallShadowCentered(float centerX, float y, const char* text, float r, float g, float b) {
    int width = getTextWidth(GLUT_BITMAP_HELVETICA_12, text);
    drawTextSmallShadow(centerX - width * 0.5f, y, text, r, g, b);
}

void drawBackground() {
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.0004f;
    float pulse = 0.08f + 0.04f * sin(t * 2.0f);

    glBegin(GL_QUADS);
    glColor3f(0.01f, 0.02f, 0.11f + pulse * 0.4f);
    glVertex2f(0, 0);
    glVertex2f(WIDTH, 0);
    glColor3f(0.01f, 0.03f, 0.16f + pulse);
    glVertex2f(WIDTH, HEIGHT);
    glVertex2f(0, HEIGHT);
    glEnd();

    glColor4f(0.2f, 0.35f, 0.7f, 0.08f);
    drawFilledCircle(150 + 60 * sin(t), 470 + 30 * cos(t * 1.2f), 140);
    glColor4f(0.45f, 0.2f, 0.6f, 0.08f);
    drawFilledCircle(600 + 50 * cos(t * 0.9f), 420 + 40 * sin(t * 1.1f), 170);
    glColor4f(0.1f, 0.4f, 0.45f, 0.07f);
    drawFilledCircle(430 + 35 * sin(t * 1.5f), 140 + 25 * cos(t * 1.3f), 120);

    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 120; i++) {
        float x = (i * 73) % WIDTH;
        float y = fmod((i * 97 + starOffset * 0.35f), HEIGHT);
        float twinkle = 0.5f + 0.5f * sin(t * 10.0f + i);
        glColor3f(0.5f + twinkle * 0.4f, 0.5f + twinkle * 0.4f, 0.7f + twinkle * 0.3f);
        glVertex2f(x, y);
    }
    glEnd();

    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 70; i++) {
        float x = (i * 131) % WIDTH;
        float y = fmod((i * 157 + starOffset * 0.75f), HEIGHT);
        float twinkle = 0.5f + 0.5f * sin(t * 13.0f + i * 1.7f);
        glColor3f(0.75f + twinkle * 0.25f, 0.75f + twinkle * 0.25f, 0.85f + twinkle * 0.15f);
        glVertex2f(x, y);
    }
    glEnd();
    glPointSize(1.0f);

    int elapsed = glutGet(GLUT_ELAPSED_TIME);
    const int cycleDuration = 7000;
    const int activeDuration = 1800;
    int cycleTime = elapsed % cycleDuration;

    if (cycleTime < activeDuration) {
        float progress = static_cast<float>(cycleTime) / static_cast<float>(activeDuration);

        float headX = -120.0f + progress * (WIDTH + 240.0f);
        float headY = HEIGHT - 70.0f - progress * 230.0f;

        float dirX = -0.88f;
        float dirY = 0.48f;

        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (int i = 1; i <= 10; i++) {
            float factor = i / 10.0f;
            float segLen = 12.0f + factor * 105.0f;
            float px = headX + dirX * segLen;
            float py = headY + dirY * segLen;
            float alpha = 0.38f * (1.0f - factor);

            glColor4f(0.82f, 0.91f, 1.0f, alpha * 0.35f);
            glVertex2f(px - dirX * 8.0f, py - dirY * 8.0f);
            glColor4f(0.95f, 0.98f, 1.0f, alpha);
            glVertex2f(px, py);
        }
        glEnd();
        glLineWidth(1.0f);

        glColor4f(0.90f, 0.96f, 1.0f, 0.28f);
        drawFilledCircle(headX, headY, 6.0f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.98f);
        drawFilledCircle(headX, headY, 2.2f);
    }
}

// Draw stars (Background scrolling effect)
void drawStars() {
    glColor3f(1.0, 1.0, 1.0);
    glPointSize(2.0);

    glBegin(GL_POINTS);
    for (int i = 0; i < 100; i++) {
        float x = (i * 73) % WIDTH;
        // Use starOffset for scrolling and fmod for looping
        float y = fmod((i * 117 + starOffset), HEIGHT);
        glVertex2f(x, y);
    }
    glEnd();
    glPointSize(1.0);
}

void spawnExplosion(float x, float y, float r, float g, float b, int count) {
    for (int i = 0; i < count; i++) {
        Particle particle;
        particle.x = x;
        particle.y = y;
        float angle = getRandomFloat(0.0f, 2.0f * 3.14159f);
        float speed = getRandomFloat(1.0f, 4.0f);
        particle.vx = cos(angle) * speed;
        particle.vy = sin(angle) * speed;
        particle.life = getRandomFloat(18.0f, 36.0f);
        particle.maxLife = particle.life;
        particle.r = r;
        particle.g = g;
        particle.b = b;
        particle.active = true;
        particles.push_back(particle);
    }
}

void drawParticles() {
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (auto& particle : particles) {
        if (!particle.active) continue;
        float alpha = particle.life / particle.maxLife;
        glColor4f(particle.r, particle.g, particle.b, alpha);
        glVertex2f(particle.x, particle.y);
    }
    glEnd();
    glPointSize(1.0f);
}

void spawnFloatingText(float x, float y, const std::string& text, float r, float g, float b) {
    FloatingText popup;
    popup.x = x;
    popup.y = y;
    popup.vy = 0.6f;
    popup.life = 60.0f;
    popup.maxLife = 60.0f;
    popup.r = r;
    popup.g = g;
    popup.b = b;
    popup.text = text;
    popup.active = true;
    floatingTexts.push_back(popup);
}

void drawFloatingTexts() {
    for (auto& popup : floatingTexts) {
        if (!popup.active) continue;
        float alpha = popup.life / popup.maxLife;
        glColor4f(popup.r, popup.g, popup.b, alpha);
        drawText(popup.x, popup.y, popup.text.c_str());
    }
}

void drawArcadeOverlay() {
    glColor4f(0.0f, 0.0f, 0.0f, 0.10f);
    glBegin(GL_LINES);
    for (int y = 0; y < HEIGHT; y += 4) {
        glVertex2f(0.0f, static_cast<float>(y));
        glVertex2f(static_cast<float>(WIDTH), static_cast<float>(y));
    }
    glEnd();

    glColor4f(0.0f, 0.4f, 0.5f, 0.25f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(2, 2);
    glVertex2f(WIDTH - 2, 2);
    glVertex2f(WIDTH - 2, HEIGHT - 2);
    glVertex2f(2, HEIGHT - 2);
    glEnd();
    glLineWidth(1.0f);
}

void drawNeonPanel(float x, float y, float w, float h, float r, float g, float b, float alpha) {
    glColor4f(r, g, b, alpha);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();

    glColor4f(r, g, b, alpha + 0.20f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    glLineWidth(1.0f);
}

// Draw player spaceship (Triangle Ship with wobble animation)
void drawPlayer() {
    glPushMatrix();

    // 1. Translation: Move drawing origin to player's center (player.x, player.y)
    glTranslatef(player.x, player.y, 0);

    // 2. Rotation effect (wobble)
    float wobble = sin(glutGet(GLUT_ELAPSED_TIME) * 0.005) * 2;
    glRotatef(wobble, 0, 0, 1);

    glLineWidth(3.0);

    // --- Determine Ship Color (Red Alert on 1 Life) ---
    if (player.lives == 1) {
        glColor3f(1.0, 0.0, 0.0); // CRITICAL: Red
    }
    else {
        glColor3f(0.0, 0.8, 1.0); // Default Blue color
    }

    // Neon outer glow hull
    glColor4f(0.0f, 0.9f, 1.0f, 0.25f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, player.size + 8);
    glVertex2f(-player.size / 2 - 5, -player.size / 2 - 5);
    glVertex2f(player.size / 2 + 5, -player.size / 2 - 5);
    glEnd();

    // --- Draw Triangle Ship ---
    glBegin(GL_TRIANGLES);
    glVertex2f(0, player.size); // Top point
    glVertex2f(-player.size / 2, -player.size / 2); // Bottom-left
    glVertex2f(player.size / 2, -player.size / 2); // Bottom-right
    glEnd();

    // Inner hull plate
    glColor3f(0.1f, 0.25f, 0.35f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, player.size - 6);
    glVertex2f(-player.size / 3, -player.size / 3);
    glVertex2f(player.size / 3, -player.size / 3);
    glEnd();

    // Engine thruster flame
    float flamePulse = 0.6f + 0.4f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.02f);
    glColor3f(1.0f, 0.4f + 0.3f * flamePulse, 0.0f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0, -player.size - (8.0f + 4.0f * flamePulse));
    glVertex2f(-5, -player.size / 2);
    glVertex2f(5, -player.size / 2);
    glEnd();

    // Muzzle flash on shooting
    if (weaponFlashTimer > 0) {
        float flashScale = weaponFlashTimer / 8.0f;
        glColor4f(1.0f, 0.95f, 0.5f, 0.6f * flashScale);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, player.size + 12 + (6 * flashScale));
        glVertex2f(-5, player.size - 2);
        glVertex2f(5, player.size - 2);
        glEnd();
    }

    // Cockpit
    glColor3f(0.3, 0.9, 1.0);
    drawCircleMidpoint(0, player.size / 3, player.size / 4);
    // Extra cockpits for style
    drawCircleMidpoint(10, player.size / 3, player.size / 4);
    drawCircleMidpoint(-10, player.size / 3, player.size / 4);
    drawCircleMidpoint(0, -player.size, player.size / 4);


    // Side fins + wings
    glColor3f(0.0f, 0.45f, 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(-player.size / 2, -player.size / 2);
    glVertex2f(-player.size - 4, -player.size / 2 - 8);
    glVertex2f(-player.size / 2 - 4, -player.size / 2 + 2);
    glVertex2f(player.size / 2, -player.size / 2);
    glVertex2f(player.size + 4, -player.size / 2 - 8);
    glVertex2f(player.size / 2 + 4, -player.size / 2 + 2);
    glEnd();

    // Wings (Drawn using Bresenham's algorithm)
    glColor3f(0.0, 0.6, 0.8);
    drawLineBresenham(-player.size / 2, -player.size / 2, -player.size, -player.size);
    drawLineBresenham(player.size / 2, -player.size / 2, player.size, -player.size);

    glLineWidth(1.0);

    glPopMatrix();
}

// Draw bullet (Conditional appearance based on level)
void drawBullet(float x, float y) {

    // --- Determine Bullet Appearance based on Level ---
    if (currentLevel < 3) {
        glColor4f(1.0, 1.0, 0.2, 0.35);
        glBegin(GL_QUADS);
        glVertex2f(x - 4, y - 8);
        glVertex2f(x + 4, y - 8);
        glVertex2f(x + 4, y + 8);
        glVertex2f(x - 4, y + 8);
        glEnd();

        glColor4f(1.0f, 1.0f, 0.8f, 0.6f);
        glBegin(GL_LINES);
        glVertex2f(x, y - 8);
        glVertex2f(x, y + 8);
        glEnd();

        // Level 1 or 2: Yellow (Original)
        glColor3f(1.0, 1.0, 0.0); // Yellow color
        glBegin(GL_QUADS);

        // Size: 4 units wide, 10 units tall
        glVertex2f(x - 2, y - 5);
        glVertex2f(x + 2, y - 5);
        glVertex2f(x + 2, y + 5);
        glVertex2f(x - 2, y + 5);

        glEnd();
    }
    else {
        glColor4f(1.0, 0.2, 0.2, 0.35);
        glBegin(GL_QUADS);
        glVertex2f(x - 5, y - 9);
        glVertex2f(x + 5, y - 9);
        glVertex2f(x + 5, y + 9);
        glVertex2f(x - 5, y + 9);
        glEnd();

        glColor4f(1.0f, 0.8f, 0.8f, 0.6f);
        glBegin(GL_LINES);
        glVertex2f(x, y - 9);
        glVertex2f(x, y + 9);
        glEnd();

        // Level 3 or higher: Red (Power-up look for final level)
        glColor3f(1.0, 0.0, 0.0); // Red color
        glBegin(GL_QUADS);

        // Size: 6 units wide, 14 units tall
        glVertex2f(x - 3, y - 7);
        glVertex2f(x + 3, y - 7);
        glVertex2f(x + 3, y + 7);
        glVertex2f(x - 3, y + 7);

        glEnd();
    }
}

// Draw enemy with different types (Circle, Triangle, Square, and DDA Line Enemy)
void drawEnemy(Enemy& enemy) {
    glPushMatrix();
    glTranslatef(enemy.x, enemy.y, 0);

    // Rotation animation
    float rotation = glutGet(GLUT_ELAPSED_TIME) * 0.1;
    glRotatef(rotation, 0, 0, 1);

    if (enemy.type == 0) { // Circle enemy
        glColor4f(1.0f, 0.2f, 0.2f, 0.25f);
        drawFilledCircle(0, 0, 20);
        glColor3f(1.0, 0.0, 0.0);
        drawFilledCircle(0, 0, 15);
        glColor3f(0.5, 0.0, 0.0);
        drawCircleMidpoint(0, 0, 15);
        glColor3f(1.0f, 0.8f, 0.8f);
        drawFilledCircle(-4, 4, 3);
    }
    else if (enemy.type == 1) { // Triangle enemy
        glColor4f(1.0f, 0.4f, 0.1f, 0.3f);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, -24);
        glVertex2f(-20, 18);
        glVertex2f(20, 18);
        glEnd();

        glColor3f(1.0, 0.3, 0.0);
        glBegin(GL_TRIANGLES);
        glVertex2f(0, -20);
        glVertex2f(-15, 15);
        glVertex2f(15, 15);
        glEnd();

        glColor3f(1.0f, 0.8f, 0.3f);
        glBegin(GL_LINES);
        glVertex2f(-8, 6);
        glVertex2f(8, 6);
        glEnd();
    }
    else if (enemy.type == 2) { // Square enemy
        glColor4f(0.9f, 0.2f, 1.0f, 0.25f);
        glBegin(GL_QUADS);
        glVertex2f(-19, -19);
        glVertex2f(19, -19);
        glVertex2f(19, 19);
        glVertex2f(-19, 19);
        glEnd();

        glColor3f(0.8, 0.0, 0.8);
        glBegin(GL_QUADS);
        glVertex2f(-15, -15);
        glVertex2f(15, -15);
        glVertex2f(15, 15);
        glVertex2f(-15, 15);
        glEnd();

        glColor3f(1.0f, 0.8f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-8, -8);
        glVertex2f(8, -8);
        glVertex2f(8, 8);
        glVertex2f(-8, 8);
        glEnd();
    }
    else { // Enemy type 3: DDA Line Diamond 🌟
        glColor3f(0.0, 1.0, 1.0); // Cyan color
        glLineWidth(2.0); // Make the lines thicker for visibility

        // Draw the four sides of a diamond using the DDA algorithm
        float size = 20.0f;

        // 1. Top to Right
        drawLineDDA(0, size, size, 0);
        // 2. Right to Bottom
        drawLineDDA(size, 0, 0, -size);
        // 3. Bottom to Left
        drawLineDDA(0, -size, -size, 0);
        // 4. Left to Top
        drawLineDDA(-size, 0, 0, size);

        glColor3f(0.7f, 1.0f, 1.0f);
        drawFilledCircle(0, 0, 3);

        glLineWidth(1.0);
    }

    glPopMatrix();
}

// Draw power-up (Green pulsing circle)
void drawPowerUp(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0);

    // Scaling animation (pulsing)
    float scale = 1.0 + 0.2 * sin(glutGet(GLUT_ELAPSED_TIME) * 0.01);
    glScalef(scale, scale, 1.0);

    glColor3f(0.0, 1.0, 0.0);
    drawFilledCircle(0, 0, 10);

    glColor4f(0.3f, 1.0f, 0.3f, 0.4f);
    drawFilledCircle(0, 0, 14);

    glColor3f(0.8f, 1.0f, 0.8f);
    drawCircleMidpoint(0, 0, 14);

    // Draw '+' sign
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2f(-5, 0);
    glVertex2f(5, 0);
    glVertex2f(0, -5);
    glVertex2f(0, 5);
    glEnd();

    glPopMatrix();
}

// Draw HUD (Score, Lives, Level, Life Icons)
void drawHUD() {
    glColor3f(1.0, 1.0, 1.0);

    drawNeonPanel(5, HEIGHT - 78, 160, 72, 0.0f, 0.4f, 0.6f, 0.08f);
    drawNeonPanel(WIDTH - 180, HEIGHT - 78, 175, 72, 0.0f, 0.4f, 0.6f, 0.08f);
    drawNeonPanel(WIDTH / 2 - 85, HEIGHT - 52, 170, 44, 0.0f, 0.35f, 0.55f, 0.08f);

    // Lives Text
    char livesText[20];
    sprintf(livesText, "Lives: %d", player.lives);
    drawTextShadow(12, HEIGHT - 30, livesText, 1.0f, 1.0f, 1.0f);

    // Score Text
    char scoreText[20];
    sprintf(scoreText, "Score: %d", player.score);
    drawTextShadow(WIDTH - 168, HEIGHT - 30, scoreText, 1.0f, 1.0f, 1.0f);

    char highScoreText[24];
    sprintf(highScoreText, "Best: %d", highScore);
    drawTextShadow(WIDTH - 168, HEIGHT - 55, highScoreText, 1.0f, 1.0f, 1.0f);

    // Current Level Text
    char levelText[20];
    sprintf(levelText, "Level: %d", currentLevel);
    drawTextShadow(WIDTH / 2 - 40, HEIGHT - 30, levelText, 1.0f, 1.0f, 1.0f);

    if (comboMultiplier > 1) {
        glColor3f(1.0f, 0.8f, 0.1f);
        char comboText[40];
        sprintf(comboText, "Combo x%d", comboMultiplier);
        drawText(WIDTH / 2 - 55, HEIGHT - 60, comboText);
    }

    if (player.lives == 1 && (glutGet(GLUT_ELAPSED_TIME) / 200) % 2 == 0) {
        glColor3f(1.0f, 0.1f, 0.1f);
        drawText(WIDTH / 2 - 45, HEIGHT - 90, "DANGER!");
    }

    // Draw life icons
    for (int i = 0; i < player.lives; i++) {
        glColor3f(1.0, 0.0, 0.0);
        drawFilledCircle(20 + i * 25, HEIGHT - 60, 8);
    }
}

// Draw menu
void drawMenu() {
    float titlePulse = 0.75f + 0.25f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.0045f);
    bool blink = ((glutGet(GLUT_ELAPSED_TIME) / 420) % 2 == 0);

    const float topPanelX = 80.0f;
    const float topPanelY = 335.0f;
    const float topPanelW = 640.0f;
    const float topPanelH = 200.0f;

    drawNeonPanel(topPanelX, topPanelY, topPanelW, topPanelH, 0.0f, 0.5f, 0.7f, 0.12f);
    drawNeonPanel(80, 80, 640, 240, 0.0f, 0.35f, 0.5f, 0.10f);

    float topPanelCenterX = topPanelX + topPanelW * 0.5f;
    float titleY = topPanelY + topPanelH - 55.0f;
    float subtitleY = titleY - 35.0f;
    float startY = subtitleY - 35.0f;

    drawTextShadowCentered(topPanelCenterX, titleY, "SPACE DEFENDER ARCADE", 0.2f, 0.9f * titlePulse, 1.0f);
    drawTextShadowCentered(topPanelCenterX, subtitleY, "Survive. Combo. Dominate.", 1.0f, 1.0f, 1.0f);

    if (blink) {
        drawTextShadowCentered(topPanelCenterX, startY, "PRESS SPACE TO START", 1.0f, 0.9f, 0.2f);
    }

    const float leftX = 110.0f;
    const float rightX = 460.0f;
    const float yTop = 285.0f;
    const float lineGap = 24.0f;

    drawTextShadow(leftX, yTop, "INSTRUCTIONS", 0.7f, 0.95f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 1, "Move : Arrow Keys or W A S D", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 2, "Shoot: SPACE (tap rhythm for combos)", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 3, "Mouse: Left Click to Start and Shoot", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 4, "Pause: P", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 5, "Power-Up: Green orb = +Life +Score", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 6, "Levels: 1-10, speed & spawn rise each level", 1.0f, 1.0f, 1.0f);
    drawTextSmallShadow(leftX, yTop - lineGap * 7, "ESC: Quit game", 1.0f, 1.0f, 1.0f);

    drawTextShadow(rightX, yTop, "ENEMY TYPES", 0.7f, 0.95f, 1.0f);
    drawTextShadow(rightX, yTop - lineGap * 2, "Red Orb", 1.0f, 0.7f, 0.7f);
    drawTextShadow(rightX, yTop - lineGap * 3, "Orange Triad", 1.0f, 0.8f, 0.3f);
    drawTextShadow(rightX, yTop - lineGap * 4, "Purple Cube", 1.0f, 0.7f, 1.0f);
    drawTextShadow(rightX, yTop - lineGap * 5, "Cyan Diamond", 0.7f, 1.0f, 1.0f);

    drawTextSmallShadowCentered(topPanelCenterX, 97, "Tip: Build combo x5 before Level 3 for high score runs.", 0.9f, 0.9f, 1.0f);
}

// Draw game over screen (Including requested text)
void drawGameOver() {
    drawNeonPanel(180, 170, 440, 270, 0.6f, 0.05f, 0.1f, 0.12f);

    drawTextShadow(WIDTH / 2 - 70, HEIGHT / 2 + 105, "MISSION FAILED", 1.0f, 0.15f, 0.15f);

    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", player.score);
    drawTextShadow(WIDTH / 2 - 80, HEIGHT / 2 + 65, scoreText, 1.0f, 1.0f, 1.0f);

    char highScoreText[50];
    sprintf(highScoreText, "Best Score: %d", highScore);
    drawTextShadow(WIDTH / 2 - 85, HEIGHT / 2 + 35, highScoreText, 1.0f, 1.0f, 1.0f);

    char rankText[40];
    sprintf(rankText, "Rank: %s", getRankByScore(player.score));
    drawTextShadow(WIDTH / 2 - 60, HEIGHT / 2 + 5, rankText, 1.0f, 1.0f, 1.0f);

    drawTextShadow(WIDTH / 2 - 125, HEIGHT / 2 - 45, "PRESS SPACE TO REDEPLOY", 1.0f, 0.9f, 0.2f);
    drawTextShadow(WIDTH / 2 - 130, HEIGHT / 2 - 75, "Press ENTER for Menu | ESC to Quit", 1.0f, 1.0f, 1.0f);
}

void startNewGame() {
    gameState = PLAYING;
    player.lives = 3;
    player.score = 0;
    player.x = WIDTH / 2;
    player.y = 50;
    bullets.clear();
    enemies.clear();
    powerUps.clear();
    particles.clear();
    floatingTexts.clear();

    currentLevel = 1;
    enemySpawnRate = 60;
    gameLevelTimer = 0;
    lastHitTimer = 0;
    levelBannerTimer = 0;
    damageFlashTimer = 0;
    shakeTimer = 0;
    shakeIntensity = 0;
    comboHits = 0;
    comboTimer = 0;
    comboMultiplier = 1;
    shootCooldownTimer = 0;
    weaponFlashTimer = 0;
    levelDifficultyMultiplier = 1.0f;
    powerUpSpawnInterval = 300;
}

void fireBullet() {
    Bullet bullet;
    bullet.x = player.x;
    bullet.y = player.y + player.size;
    bullet.speed = 10.0;
    bullet.active = true;
    bullets.push_back(bullet);

    shootCooldownTimer = 7;
    weaponFlashTimer = 8;
}

// Update game logic (Called 60 times per second)
void update(int value) {
    if (gameState == PLAYING) {
        // Update background animation
        starOffset += 0.5;
        if (starOffset > HEIGHT) starOffset = 0;

        // --- Timer Increments ---
        enemySpawnTimer++;
        powerUpTimer++;
        gameLevelTimer++;
        lastHitTimer++;
        if (shootCooldownTimer > 0) shootCooldownTimer--;
        if (weaponFlashTimer > 0) weaponFlashTimer--;

        if (comboTimer > 0) {
            comboTimer--;
            if (comboTimer <= 0) {
                comboHits = 0;
                comboMultiplier = 1;
            }
        }
        if (levelBannerTimer > 0) levelBannerTimer--;
        if (damageFlashTimer > 0) damageFlashTimer--;
        if (shakeTimer > 0) shakeTimer--;

        // --- 1. Level Management (Every 15 seconds) ---
        int levelDurationFrames = std::max(420, 900 - (currentLevel - 1) * 40);
        if (gameLevelTimer >= levelDurationFrames) {
            if (currentLevel < MAX_LEVEL) {
                currentLevel++;
                levelDifficultyMultiplier = 1.0f + (currentLevel - 1) * 0.16f;
                enemySpawnRate = std::max(10, static_cast<int>(60.0f / levelDifficultyMultiplier));
                powerUpSpawnInterval = std::min(560, 300 + (currentLevel - 1) * 25);
                levelBannerTimer = 150;
            }
            gameLevelTimer = 0;
        }

        // --- 2. Player Penalty (If no hit for 8 seconds) ---
        int noHitPenaltyFrames = std::max(240, 480 - (currentLevel - 1) * 20);
        if (lastHitTimer >= noHitPenaltyFrames) {
            if (player.score >= 25) {
                player.score -= 25;
                spawnFloatingText(player.x - 25, player.y + 35, "-25", 1.0f, 0.2f, 0.2f);
            }
            else if (player.lives > 0) {
                player.lives--;
                damageFlashTimer = 14;
                shakeTimer = 12;
                shakeIntensity = 7.0f;
            }
            lastHitTimer = 0;
            if (player.lives <= 0) {
                gameState = GAME_OVER;
            }
        }

        // Update player position based on key presses
        if (keys['a'] || keys['A']) {
            player.x -= player.speed;
            if (player.x < player.size) player.x = player.size;
        }
        if (keys['d'] || keys['D']) {
            player.x += player.speed;
            if (player.x > WIDTH - player.size) player.x = WIDTH - player.size;
        }
        if (keys['w'] || keys['W']) {
            player.y += player.speed;
            if (player.y > HEIGHT - player.size) player.y = HEIGHT - player.size;
        }
        if (keys['s'] || keys['S']) {
            player.y -= player.speed;
            if (player.y < player.size) player.y = player.size;
        }

        // Update bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                bullet.y += bullet.speed;
                if (bullet.y > HEIGHT) bullet.active = false;
            }
        }

        // Spawn enemies
        if (enemySpawnTimer > enemySpawnRate) {
            Enemy enemy;
            enemy.x = rand() % (WIDTH - 40) + 20;
            enemy.y = HEIGHT;

            enemy.speed = (1.6f + (rand() % 3)) * levelDifficultyMultiplier;

            enemy.active = true;
            enemy.type = rand() % 4;
            enemies.push_back(enemy);
            enemySpawnTimer = 0;
        }

        // Update enemies & Check collision with player
        for (auto& enemy : enemies) {
            if (enemy.active) {
                enemy.y -= enemy.speed;
                if (enemy.y < -30) enemy.active = false;

                // Check collision with player
                float enemyRadius = getEnemyCollisionRadius(enemy.type);
                float playerRadius = player.size * 0.75f;
                float collisionRadius = enemyRadius + playerRadius;
                float collisionRadiusSq = collisionRadius * collisionRadius;
                float distanceSq = getSquaredDistance(enemy.x, enemy.y, player.x, player.y);
                if (distanceSq < collisionRadiusSq) {
                    enemy.active = false;
                    player.lives--;
                    spawnExplosion(enemy.x, enemy.y, 1.0f, 0.2f, 0.2f, 24);
                    damageFlashTimer = 14;
                    shakeTimer = 12;
                    shakeIntensity = 7.0f;
                    if (player.lives <= 0) {
                        gameState = GAME_OVER;
                    }
                }
            }
        }

        // Check bullet-enemy collision (Resets lastHitTimer)
        for (auto& bullet : bullets) {
            if (bullet.active) {
                for (auto& enemy : enemies) {
                    if (enemy.active) {
                        float enemyRadius = getEnemyCollisionRadius(enemy.type);
                        float bulletRadius = (currentLevel < 3) ? 5.0f : 7.0f;
                        float collisionRadius = enemyRadius + bulletRadius;
                        float collisionRadiusSq = collisionRadius * collisionRadius;
                        float distanceSq = getSquaredDistance(bullet.x, bullet.y, enemy.x, enemy.y);
                        if (distanceSq < collisionRadiusSq) {
                            bullet.active = false;
                            enemy.active = false;
                            comboHits++;
                            comboMultiplier = std::min(5, 1 + (comboHits / 4));
                            comboTimer = 120;
                            int points = 10 * comboMultiplier;
                            player.score += points;
                            char pointsText[20];
                            sprintf(pointsText, "+%d", points);
                            spawnFloatingText(enemy.x - 12, enemy.y + 10, pointsText, 1.0f, 0.85f, 0.2f);

                            if (enemy.type == 0) spawnExplosion(enemy.x, enemy.y, 1.0f, 0.2f, 0.2f, 20);
                            else if (enemy.type == 1) spawnExplosion(enemy.x, enemy.y, 1.0f, 0.5f, 0.1f, 20);
                            else if (enemy.type == 2) spawnExplosion(enemy.x, enemy.y, 0.9f, 0.1f, 1.0f, 20);
                            else spawnExplosion(enemy.x, enemy.y, 0.2f, 1.0f, 1.0f, 20);

                            shakeTimer = std::max(shakeTimer, 4.0f);
                            shakeIntensity = 3.0f;
                            // Reset the timer on successful hit
                            lastHitTimer = 0;
                        }
                    }
                }
            }
        }

        // Spawn power-ups (Every 5 seconds/300 frames)
        if (powerUpTimer > powerUpSpawnInterval) {
            PowerUp powerUp;
            powerUp.x = rand() % (WIDTH - 40) + 20;
            powerUp.y = HEIGHT;
            powerUp.speed = 1.5 + (currentLevel * 0.05f);
            powerUp.active = true;
            powerUps.push_back(powerUp);
            powerUpTimer = 0;
        }

        // Update power-ups
        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                powerUp.y -= powerUp.speed;
                if (powerUp.y < -20) powerUp.active = false;

                // Check collision with player
                float collisionRadius = player.size + 10.0f;
                float collisionRadiusSq = collisionRadius * collisionRadius;
                float distanceSq = getSquaredDistance(powerUp.x, powerUp.y, player.x, player.y);
                if (distanceSq < collisionRadiusSq) {
                    powerUp.active = false;
                    if (player.lives < 5) player.lives++; // Max 5 lives
                    player.score += 20;
                    lastHitTimer = 0;
                    spawnExplosion(powerUp.x, powerUp.y, 0.1f, 1.0f, 0.2f, 16);
                    spawnFloatingText(powerUp.x - 12, powerUp.y + 10, "+20", 0.1f, 1.0f, 0.2f);
                }
            }
        }

        if (player.score > highScore) {
            highScore = player.score;
            saveHighScore();
        }

        for (auto& particle : particles) {
            if (particle.active) {
                particle.x += particle.vx;
                particle.y += particle.vy;
                particle.vy -= 0.03f;
                particle.life -= 1.0f;
                if (particle.life <= 0.0f) {
                    particle.active = false;
                }
            }
        }

        for (auto& popup : floatingTexts) {
            if (popup.active) {
                popup.y += popup.vy;
                popup.life -= 1.0f;
                if (popup.life <= 0.0f) {
                    popup.active = false;
                }
            }
        }

        // Remove inactive objects (Cleanup)
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return !b.active; }), bullets.end());
        enemies.erase(std::remove_if(enemies.begin(), enemies.end(),
            [](const Enemy& e) { return !e.active; }), enemies.end());
        powerUps.erase(std::remove_if(powerUps.begin(), powerUps.end(),
            [](const PowerUp& p) { return !p.active; }), powerUps.end());
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return !p.active; }), particles.end());
        floatingTexts.erase(std::remove_if(floatingTexts.begin(), floatingTexts.end(),
            [](const FloatingText& t) { return !t.active; }), floatingTexts.end());
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0); // Next frame in 16ms (~60 FPS)
}

// Display function (Renders everything)
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    drawBackground(); // Draw animated background first

    if (gameState == MENU) {
        drawMenu();
    }
    else if (gameState == PLAYING) {
        float shakeX = 0.0f;
        float shakeY = 0.0f;
        if (shakeTimer > 0) {
            shakeX = getRandomFloat(-shakeIntensity, shakeIntensity);
            shakeY = getRandomFloat(-shakeIntensity, shakeIntensity);
        }

        glPushMatrix();
        glTranslatef(shakeX, shakeY, 0.0f);
        drawPlayer();

        // Draw active bullets
        for (auto& bullet : bullets) {
            if (bullet.active) {
                drawBullet(bullet.x, bullet.y);
            }
        }

        // Draw active enemies
        for (auto& enemy : enemies) {
            if (enemy.active) {
                drawEnemy(enemy);
            }
        }

        // Draw active power-ups
        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                drawPowerUp(powerUp.x, powerUp.y);
            }
        }

        drawParticles();
        glPopMatrix();

        drawFloatingTexts();

        drawHUD(); // Draw HUD last so it's on top

        if (levelBannerTimer > 0) {
            float pulse = 0.6f + 0.4f * sin(glutGet(GLUT_ELAPSED_TIME) * 0.02f);
            glColor3f(1.0f, pulse, 0.1f);
            drawText(WIDTH / 2 - 60, HEIGHT / 2 + 120, "LEVEL UP!");
        }

        if (damageFlashTimer > 0) {
            float alpha = (damageFlashTimer / 14.0f) * 0.25f;
            glColor4f(1.0f, 0.0f, 0.0f, alpha);
            glBegin(GL_QUADS);
            glVertex2f(0, 0);
            glVertex2f(WIDTH, 0);
            glVertex2f(WIDTH, HEIGHT);
            glVertex2f(0, HEIGHT);
            glEnd();
        }
    }
    else if (gameState == PAUSED) {
        drawPlayer();

        for (auto& bullet : bullets) {
            if (bullet.active) {
                drawBullet(bullet.x, bullet.y);
            }
        }

        for (auto& enemy : enemies) {
            if (enemy.active) {
                drawEnemy(enemy);
            }
        }

        for (auto& powerUp : powerUps) {
            if (powerUp.active) {
                drawPowerUp(powerUp.x, powerUp.y);
            }
        }

        drawParticles();
        drawFloatingTexts();
        drawHUD();

        drawNeonPanel(220, 230, 360, 130, 0.2f, 0.5f, 0.8f, 0.12f);
        drawTextShadow(WIDTH / 2 - 42, HEIGHT / 2 + 32, "PAUSED", 1.0f, 0.95f, 0.2f);
        drawTextShadow(WIDTH / 2 - 145, HEIGHT / 2 - 2, "Press P to Resume | ESC to Quit", 1.0f, 1.0f, 1.0f);
    }
    else if (gameState == GAME_OVER) {
        drawGameOver();
    }

    drawArcadeOverlay();

    glutSwapBuffers();
}

// Keyboard input handlers
void keyboardDown(unsigned char key, int x, int y) {
    keys[key] = true;

    if (key == 'p' || key == 'P') {
        if (gameState == PLAYING) {
            gameState = PAUSED;
            return;
        }
        if (gameState == PAUSED) {
            gameState = PLAYING;
            return;
        }
    }

    if (key == 13 && gameState == GAME_OVER) {
        gameState = MENU;
        return;
    }

    if (key == 27) { // ESC
        exit(0);
    }

    if (key == ' ') {
        if (gameState == MENU || gameState == GAME_OVER) {
            startNewGame();
        }
        else if (gameState == PLAYING) {
            if (shootCooldownTimer <= 0) {
                fireBullet();
            }
        }
    }
}

void mouseClick(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;
    if (button != GLUT_LEFT_BUTTON) return;

    if (gameState == MENU || gameState == GAME_OVER) {
        startNewGame();
        return;
    }

    if (gameState == PLAYING && shootCooldownTimer <= 0) {
        fireBullet();
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Special keyboard functions (For Arrow Keys)
void specialDown(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:
        keys['a'] = true;
        break;
    case GLUT_KEY_RIGHT:
        keys['d'] = true;
        break;
    case GLUT_KEY_UP:
        keys['w'] = true;
        break;
    case GLUT_KEY_DOWN:
        keys['s'] = true;
        break;
    }
}

void specialUp(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_LEFT:
        keys['a'] = false;
        break;
    case GLUT_KEY_RIGHT:
        keys['d'] = false;
        break;
    case GLUT_KEY_UP:
        keys['w'] = false;
        break;
    case GLUT_KEY_DOWN:
        keys['s'] = false;
        break;
    }
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Space Defender - 2D OpenGL Game");

    init();

    glutDisplayFunc(display);

    // Register keyboard functions
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutMouseFunc(mouseClick);

    // Register SPECIAL keyboard functions (for ARROW keys)
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);

    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
