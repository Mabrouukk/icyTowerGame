#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>

const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;
const float GRAVITY = -0.0008f;
const float JUMP_VELOCITY = 0.6f;
const float MOVE_SPEED = 0.15f;
const int INITIAL_LIVES = 3;
const int COLLECTABLES_COUNT = 7;
const float LAVA_INITIAL_SPEED = 0.01f;
const float LAVA_SPEED_INCREMENT = 0.0003f;

// Button positions
const float startButtonX = WINDOW_WIDTH / 2 - 75;
const float startButtonY = WINDOW_HEIGHT / 2 - 20;
const float startButtonWidth = 160;
const float startButtonHeight = 50;

const float restartButtonX = WINDOW_WIDTH / 2 - 75;
const float restartButtonY = (WINDOW_HEIGHT / 2) - 80;
const float restartButtonWidth = 150;
const float restartButtonHeight = 50;
bool isPaused = false;         
float pauseButtonX = 20;       
float pauseButtonY = WINDOW_HEIGHT - 100;
float pauseButtonWidth = 100;
float pauseButtonHeight = 35;

enum GameState { MENU, PLAYING, WIN, LOSE };
GameState gameState = MENU;

struct Player {
    float x, y;
    float width, height;
    float velocityY;
    bool isJumping;
    int lives;
    int score;
    bool hasKey;
    int activePowerUp;
    float powerUpTimer;
} player;

struct Platform {
    float x, y;
    float width, height;
    bool destroyed;
};

struct Collectable {
    float x, y;
    float size;
    bool collected;
    float rotation;
};

struct Rock {
    float x, y;
    float size;
    float speed;
    bool active;
};

struct PowerUp {
    float x, y;
    float size;
    int type;
    bool collected;
    float timer;
    float rotation;
};

struct Key {
    float x, y;
    float size;
    bool spawned;
    bool collected;
    float rotation;
};

struct Door {
    float x, y;
    float width, height;
    bool unlocked;
    float openAnimation;
};

std::vector<Platform> platforms;
std::vector<Collectable> collectables;
std::vector<Rock> rocks;
std::vector<PowerUp> powerUps;
Key key;
Door door;

float lavaHeight = 0.0f;
float lavaSpeed = LAVA_INITIAL_SPEED;

int lastRockSpawn = 0;
int gameTime = 0;
int powerUpSpawnTime = 0;

bool keys[256];

void init() {
   // glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    
    srand(time(0));
    
    player.x = WINDOW_WIDTH / 2;
    player.y = 50;
    player.width = 30;
    player.height = 40;
    player.velocityY = 0;
    player.isJumping = false;
    player.lives = INITIAL_LIVES;
    player.score = 0;
    player.hasKey = false;
    player.activePowerUp = 0;
    player.powerUpTimer = 0;
    
    float platformY = 100;
    float platformWidths[] = {80, 120, 100, 90, 110, 85, 95, 105, 80, 90};
    
    for (int i = 0; i < 10; i++) {
        Platform p;
        p.width = platformWidths[i];
        p.height = 20;
        p.y = platformY;
        p.x = rand() % (WINDOW_WIDTH - (int)p.width);
        p.destroyed = false;
        platforms.push_back(p);
        platformY += 55;
    }
    
    for (int i = 0; i < COLLECTABLES_COUNT; i++) {
        Collectable c;
        c.x = rand() % (WINDOW_WIDTH - 40) + 20;
        c.y = 150 + i * 70;
        c.size = 15;
        c.collected = false;
        c.rotation = 0;
        collectables.push_back(c);
    }
    
    key.spawned = false;
    key.collected = false;
    key.size = 20;
    key.rotation = 0;
    
    door.x = WINDOW_WIDTH / 2 - 40;
    door.y = platformY - 100;
    door.width = 80;
    door.height = 60;
    door.unlocked = false;
    door.openAnimation = 0;
    
    for (int i = 0; i < 256; i++) keys[i] = false;
}

void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        text++;
    }
}

void drawLargeText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *text);
        text++;
    }
}

void drawPlayer() {
    float x = player.x;
    float y = player.y;
    float w = player.width;
    float h = player.height;

    // Power-up aura (glow)
    if (player.activePowerUp == 1) {
        glColor4f(0.2f, 0.6f, 1.0f, 0.4f); // soft blue aura with transparency
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y + h / 2);
        for (int i = 0; i <= 40; i++) {
            float angle = i * 2.0f * 3.14159f / 40;
            glVertex2f(x + cos(angle) * (w / 2 + 10), y + h / 2 + sin(angle) * (h / 2 + 10));
        }
        glEnd();
    }

    // === BODY (Shirt) ===
    glColor3f(0.2f, 0.4f, 0.9f); // bright blue shirt
    glBegin(GL_QUADS);
    glVertex2f(x - w / 2, y + h * 0.25f);
    glVertex2f(x + w / 2, y + h * 0.25f);
    glVertex2f(x + w / 2, y + h * 0.7f);
    glVertex2f(x - w / 2, y + h * 0.7f);
    glEnd();

    // === LEGS ===
    glColor3f(0.1f, 0.1f, 0.2f); // dark navy pants
    float legWidth = w / 3.5f;
    float legHeight = h * 0.25f;

    // Left leg
    glBegin(GL_QUADS);
    glVertex2f(x - legWidth - 2, y);
    glVertex2f(x - 2, y);
    glVertex2f(x - 2, y + legHeight);
    glVertex2f(x - legWidth - 2, y + legHeight);
    glEnd();

    // Right leg
    glBegin(GL_QUADS);
    glVertex2f(x + 2, y);
    glVertex2f(x + legWidth + 2, y);
    glVertex2f(x + legWidth + 2, y + legHeight);
    glVertex2f(x + 2, y + legHeight);
    glEnd();

    // === SHOES ===
    glColor3f(0.2f, 0.05f, 0.05f); // brown shoes
    float shoeHeight = 4.0f;
    glBegin(GL_QUADS);
    // left shoe
    glVertex2f(x - legWidth - 2, y);
    glVertex2f(x - 2, y);
    glVertex2f(x - 2, y - shoeHeight);
    glVertex2f(x - legWidth - 2, y - shoeHeight);
    // right shoe
    glVertex2f(x + 2, y);
    glVertex2f(x + legWidth + 2, y);
    glVertex2f(x + legWidth + 2, y - shoeHeight);
    glVertex2f(x + 2, y - shoeHeight);
    glEnd();

    // === HEAD ===
    glColor3f(0.95f, 0.78f, 0.55f); // lighter, warmer skin tone
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y + h * 0.88f);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(x + cos(angle) * w / 3, y + h * 0.88f + sin(angle) * w / 3);
    }
    glEnd();

    // === CAP ===
    glColor3f(0.8f, 0.1f, 0.1f); // deep red
    glBegin(GL_POLYGON);
    glVertex2f(x - w / 2.5f, y + h * 0.97f);
    glVertex2f(x + w / 2.5f, y + h * 0.97f);
    glVertex2f(x + w / 2.2f, y + h * 1.05f);
    glVertex2f(x - w / 2.2f, y + h * 1.05f);
    glEnd();

    // Cap brim
    glColor3f(0.6f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(x - w / 2, y + h * 0.95f);
    glVertex2f(x + w / 2, y + h * 0.95f);
    glVertex2f(x + w / 2, y + h * 0.97f);
    glVertex2f(x - w / 2, y + h * 0.97f);
    glEnd();

    // Cap button
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y + h * 1.05f);
    for (int i = 0; i <= 12; i++) {
        float angle = i * 2.0f * 3.14159f / 12;
        glVertex2f(x + cos(angle) * 3, y + h * 1.05f + sin(angle) * 3);
    }
    glEnd();

    // === EYES ===
    glColor3f(0.1f, 0.1f, 0.1f);
    glPointSize(4);
    glBegin(GL_POINTS);
    glVertex2f(x - w / 6, y + h * 0.88f);
    glVertex2f(x + w / 6, y + h * 0.88f);
    glEnd();

    // === ARMS ===
    glLineWidth(3);
    glColor3f(0.95f, 0.78f, 0.55f);
    glBegin(GL_LINES);
    glVertex2f(x - w / 2, y + h * 0.55f);
    glVertex2f(x - w / 2 - 8, y + h * 0.35f);
    glVertex2f(x + w / 2, y + h * 0.55f);
    glVertex2f(x + w / 2 + 8, y + h * 0.35f);
    glEnd();
}

void drawPlatforms() {
    for (auto& p : platforms) {
        if (p.destroyed) continue;

        float x = p.x;
        float y = p.y;
        float w = p.width;
        float h = p.height;

        // Rock base color (gray)
        glColor3f(0.4f, 0.4f, 0.42f);
        
        // Main rock body (irregular polygon to look like a rock)
        glBegin(GL_POLYGON);
        glVertex2f(x + w * 0.1f, y);
        glVertex2f(x + w * 0.9f, y);
        glVertex2f(x + w, y + h * 0.3f);
        glVertex2f(x + w * 0.95f, y + h * 0.7f);
        glVertex2f(x + w * 0.7f, y + h);
        glVertex2f(x + w * 0.3f, y + h);
        glVertex2f(x + w * 0.05f, y + h * 0.7f);
        glVertex2f(x, y + h * 0.3f);
        glEnd();
        
        // Rock highlights (lighter gray triangles for texture)
        glColor3f(0.55f, 0.55f, 0.58f);
        glBegin(GL_TRIANGLES);
        // Top left highlight
        glVertex2f(x + w * 0.2f, y + h * 0.6f);
        glVertex2f(x + w * 0.35f, y + h * 0.8f);
        glVertex2f(x + w * 0.15f, y + h * 0.9f);
        
        // Top right highlight
        glVertex2f(x + w * 0.7f, y + h * 0.7f);
        glVertex2f(x + w * 0.85f, y + h * 0.6f);
        glVertex2f(x + w * 0.8f, y + h * 0.9f);
        glEnd();
        
        // Rock cracks/lines (dark lines for detail)
        glColor3f(0.25f, 0.25f, 0.27f);
        glLineWidth(2);
        glBegin(GL_LINES);
        // Crack 1
        glVertex2f(x + w * 0.3f, y + h * 0.2f);
        glVertex2f(x + w * 0.4f, y + h * 0.8f);
        // Crack 2
        glVertex2f(x + w * 0.6f, y + h * 0.1f);
        glVertex2f(x + w * 0.7f, y + h * 0.7f);
        glEnd();
        
        // Rock outline for definition
        glColor3f(0.2f, 0.2f, 0.22f);
        glLineWidth(2);
        glBegin(GL_LINE_LOOP);
        glVertex2f(x + w * 0.1f, y);
        glVertex2f(x + w * 0.9f, y);
        glVertex2f(x + w, y + h * 0.3f);
        glVertex2f(x + w * 0.95f, y + h * 0.7f);
        glVertex2f(x + w * 0.7f, y + h);
        glVertex2f(x + w * 0.3f, y + h);
        glVertex2f(x + w * 0.05f, y + h * 0.7f);
        glVertex2f(x, y + h * 0.3f);
        glEnd();
    }
}

void drawCollectables() {
    for (auto& c : collectables) {
        if (c.collected) continue;
        
        glPushMatrix();
        glTranslatef(c.x, c.y, 0);
        glRotatef(c.rotation, 0, 0, 1);
        
        glColor3f(1.0f, 0.85f, 0.2f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(cos(angle) * c.size, sin(angle) * c.size);
        }
        glEnd();
        
        glColor3f(0.9f, 0.7f, 0.1f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(cos(angle) * c.size * 0.6f, sin(angle) * c.size * 0.6f);
        }
        glEnd();
        
        glColor3f(1.0f, 0.95f, 0.5f);
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < 4; i++) {
            float angle = i * 3.14159f / 2;
            glVertex2f(0, 0);
            glVertex2f(cos(angle) * c.size * 0.4f, sin(angle) * c.size * 0.4f);
            glVertex2f(cos(angle + 3.14159f/2) * c.size * 0.4f, sin(angle + 3.14159f/2) * c.size * 0.4f);
        }
        glEnd();
        
        glPopMatrix();
    }
}

void drawRocks() {
    for (auto& r : rocks) {
        if (!r.active) continue;

        int layers = 6; // number of gradient layers
        float maxSize = r.size;

        // Draw layered glow (outer dark -> inner bright)
        for (int l = 0; l < layers; l++) {
            float t = (float)l / (layers - 1);
            float radius = maxSize * (1.0f - 0.12f * l);

            // Color gradient: from dark gray to fiery orange
            float rColor = 0.2f + 0.8f * (1 - t);
            float gColor = 0.1f + 0.4f * (1 - t);
            float bColor = 0.05f + 0.1f * (1 - t);

            glColor3f(rColor, gColor, bColor);

            glBegin(GL_POLYGON);
            for (int i = 0; i < 12; i++) {
                float angle = i * 2.0f * 3.14159f / 12.0f;
                float randOffset = (rand() % 10 - 5) * 0.01f * radius; // jagged edges
                float x = r.x + cos(angle) * (radius + randOffset);
                float y = r.y + sin(angle) * (radius + randOffset);
                glVertex2f(x, y);
            }
            glEnd();
        }

        // Add fiery cracks (random thin triangles)
        glColor3f(1.0f, 0.6f, 0.0f); // bright orange
        for (int i = 0; i < 3; i++) {
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            float innerR = r.size * 0.2f;
            float outerR = r.size * (0.5f + (rand() % 50) / 100.0f);

            glBegin(GL_TRIANGLES);
                glVertex2f(r.x, r.y);
                glVertex2f(r.x + cos(angle) * innerR, r.y + sin(angle) * innerR);
                glVertex2f(r.x + cos(angle) * outerR, r.y + sin(angle) * outerR);
            glEnd();
        }

        // Subtle glowing halo (transparency)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.4f, 0.1f, 0.15f); // soft orange glow
        glBegin(GL_POLYGON);
        for (int i = 0; i < 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20.0f;
            glVertex2f(r.x + cos(angle) * (r.size * 1.3f),
                       r.y + sin(angle) * (r.size * 1.3f));
        }
        glEnd();
        glDisable(GL_BLEND);
    }
}


void drawLava() {
    glColor3f(1.0f, 0.4f, 0.0f); 
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, lavaHeight);
    glVertex2f(0, lavaHeight);
    glEnd();
    
    glColor3f(1.0f, 0.4f, 0.0f); 
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        float wave = sin((i + gameTime * 0.1f) * 0.1f) * 10;
        glVertex2f(i, lavaHeight);
        glVertex2f(i + 20, lavaHeight + 15 + wave);
        glVertex2f(i + 40, lavaHeight);
    }
    glEnd();
}

void drawKey() {
    if (!key.spawned || key.collected) return;
    
    glPushMatrix();
    glTranslatef(key.x, key.y, 0);
    glRotatef(key.rotation, 0, 0, 1);
    
    glColor3f(1.0f, 0.85f, 0.2f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(cos(angle) * key.size * 0.6f, sin(angle) * key.size * 0.6f);
    }
    glEnd();
    
    glColor3f(0.4f, 0.25f, 0.05f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(cos(angle) * key.size * 0.2f, sin(angle) * key.size * 0.2f);
    }
    glEnd();
    
    glColor3f(1.0f, 0.85f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(key.size * 0.3f, -key.size * 0.2f);
    glVertex2f(key.size * 1.5f, -key.size * 0.2f);
    glVertex2f(key.size * 1.5f, key.size * 0.2f);
    glVertex2f(key.size * 0.3f, key.size * 0.2f);
    glEnd();
    
    glBegin(GL_TRIANGLES);
    glVertex2f(key.size * 1.2f, -key.size * 0.2f);
    glVertex2f(key.size * 1.3f, -key.size * 0.2f);
    glVertex2f(key.size * 1.25f, -key.size * 0.5f);
    
    glVertex2f(key.size * 1.4f, -key.size * 0.2f);
    glVertex2f(key.size * 1.5f, -key.size * 0.2f);
    glVertex2f(key.size * 1.45f, -key.size * 0.4f);
    glEnd();
    
    glPopMatrix();
}

void drawPowerUps() {
    for (auto& pu : powerUps) {
        if (pu.collected) continue;
        
        glPushMatrix();
        glTranslatef(pu.x, pu.y, 0);
        
        if (pu.type == 1) {
            glRotatef(pu.rotation, 0, 0, 1);
            glColor3f(0.2f, 0.55f, 0.95f);
            
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (int i = 0; i <= 20; i++) {
                float angle = i * 2.0f * 3.14159f / 20;
                glVertex2f(cos(angle) * pu.size, sin(angle) * pu.size);
            }
            glEnd();
            
            glColor3f(0.4f, 0.7f, 1.0f);
            glBegin(GL_POLYGON);
            glVertex2f(0, pu.size * 0.7f);
            glVertex2f(-pu.size * 0.5f, pu.size * 0.3f);
            glVertex2f(-pu.size * 0.5f, -pu.size * 0.5f);
            glVertex2f(0, -pu.size * 0.7f);
            glVertex2f(pu.size * 0.5f, -pu.size * 0.5f);
            glVertex2f(pu.size * 0.5f, pu.size * 0.3f);
            glEnd();
            
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            glVertex2f(-pu.size * 0.1f, -pu.size * 0.4f);
            glVertex2f(pu.size * 0.1f, -pu.size * 0.4f);
            glVertex2f(pu.size * 0.1f, pu.size * 0.4f);
            glVertex2f(-pu.size * 0.1f, pu.size * 0.4f);
            
            glVertex2f(-pu.size * 0.4f, -pu.size * 0.1f);
            glVertex2f(pu.size * 0.4f, -pu.size * 0.1f);
            glVertex2f(pu.size * 0.4f, pu.size * 0.1f);
            glVertex2f(-pu.size * 0.4f, pu.size * 0.1f);
            glEnd();
            
        } else {
            float scale = 1.0f + sin(pu.rotation * 0.05f) * 0.2f;
            glScalef(scale, scale, 1.0f);
            
            glColor3f(0.1f, 0.75f, 0.95f);
            
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (int i = 0; i <= 20; i++) {
                float angle = i * 2.0f * 3.14159f / 20;
                glVertex2f(cos(angle) * pu.size * 0.3f, sin(angle) * pu.size * 0.3f);
            }
            glEnd();
            
            glColor3f(0.5f, 0.85f, 1.0f);
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < 6; i++) {
                float angle = i * 3.14159f / 3;
                glVertex2f(0, 0);
                glVertex2f(cos(angle) * pu.size * 0.3f, sin(angle) * pu.size * 0.3f);
                glVertex2f(cos(angle) * pu.size, sin(angle) * pu.size);
            }
            glEnd();
            
            glLineWidth(2);
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_LINES);
            for (int i = 0; i < 6; i++) {
                float angle = i * 3.14159f / 3;
                glVertex2f(0, 0);
                glVertex2f(cos(angle) * pu.size * 0.8f, sin(angle) * pu.size * 0.8f);
            }
            glEnd();
        }
        
        glPopMatrix();
    }
}

void drawDoor() {
    float x = door.x;
    float y = door.y;
    float w = door.width;
    float h = door.height;

    // Draw the frame first (dark brown)
    glColor3f(0.20f, 0.12f, 0.04f);
    glBegin(GL_QUADS);
    glVertex2f(x - 5, y - 5);
    glVertex2f(x + w + 5, y - 5);
    glVertex2f(x + w + 5, y + h + 5);
    glVertex2f(x - 5, y + h + 5);
    glEnd();

    // Door open/close transformation
    if (door.unlocked) {
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(-door.openAnimation * 90, 0, 0, 1);
        glTranslatef(-x, -y, 0);
    }

    // --- Door body (wood gradient) ---
    glBegin(GL_QUADS);
    // darker side (simulate shading)
    glColor3f(0.35f, 0.22f, 0.08f);  // left side
    glVertex2f(x, y);
    glVertex2f(x + w * 0.4f, y);
    glVertex2f(x + w * 0.4f, y + h);
    glVertex2f(x, y + h);

    // lighter side (simulate light reflection)
    glColor3f(0.45f, 0.30f, 0.10f);  // right side
    glVertex2f(x + w * 0.4f, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x + w * 0.4f, y + h);
    glEnd();

    // --- Decorative horizontal panels (for realism) ---
    glColor3f(0.25f, 0.15f, 0.05f);
    for (int i = 1; i <= 3; i++) {
        float panelY = y + (h / 4.0f) * i;
        glBegin(GL_LINES);
        glVertex2f(x + 10, panelY);
        glVertex2f(x + w - 10, panelY);
        glEnd();
    }

    // --- Door knob (metallic with highlight) ---
    float knobX = x + w - 15;
    float knobY = y + h / 2;

    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.8f, 0.7f, 0.1f);  // gold base
    glVertex2f(knobX, knobY);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glColor3f(0.9f, 0.8f, 0.3f);  // light edge
        glVertex2f(knobX + cos(angle) * 5, knobY + sin(angle) * 5);
    }
    glEnd();

    // Small highlight on knob (simulated light)
    glColor3f(1.0f, 1.0f, 0.6f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(knobX + 1.5f, knobY + 1.5f);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(knobX + 1.5f + cos(angle) * 1.5f, knobY + 1.5f + sin(angle) * 1.5f);
    }
    glEnd();

    // --- Door open shadow effect ---
    if (door.unlocked && door.openAnimation > 0.1f) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f); // semi-transparent
        glBegin(GL_QUADS);
        glVertex2f(x + w + 2, y);
        glVertex2f(x + w + 10, y);
        glVertex2f(x + w + 10, y + h);
        glVertex2f(x + w + 2, y + h);
        glEnd();
    }

    if (door.unlocked)
        glPopMatrix();
}

void drawHUD() {
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 30);
    glVertex2f(160, WINDOW_HEIGHT - 30);
    glVertex2f(160, WINDOW_HEIGHT - 10);
    glVertex2f(10, WINDOW_HEIGHT - 10);
    glEnd();
    
    for (int i = 0; i < player.lives; i++) {
        glColor3f(0.95f, 0.15f, 0.15f);
        glBegin(GL_TRIANGLE_FAN);
        float cx = 25 + i * 50;
        float cy = WINDOW_HEIGHT - 20;
        glVertex2f(cx, cy);
        for (int j = 0; j <= 20; j++) {
            float angle = j * 2.0f * 3.14159f / 20;
            glVertex2f(cx + cos(angle) * 15, cy + sin(angle) * 12);
        }
        glEnd();
    }
    
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 60);
    glVertex2f(210, WINDOW_HEIGHT - 60);
    glVertex2f(210, WINDOW_HEIGHT - 40);
    glVertex2f(10, WINDOW_HEIGHT - 40);
    glEnd();
    
    float danger = lavaHeight / WINDOW_HEIGHT;
    float barWidth = 200 * danger;
    
    if (danger < 0.5f) {
        glColor3f(0.2f, 0.8f, 0.2f);
    } else if (danger < 0.75f) {
        glColor3f(0.95f, 0.75f, 0.1f);
    } else {
        glColor3f(0.95f, 0.2f, 0.1f);
    }
    
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 60);
    glVertex2f(10 + barWidth, WINDOW_HEIGHT - 60);
    glVertex2f(10 + barWidth, WINDOW_HEIGHT - 40);
    glVertex2f(10, WINDOW_HEIGHT - 40);
    glEnd();
    
    glColor3f(0.95f, 0.95f, 0.95f);
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", player.score);
    drawText(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 25, scoreText);
}

void drawMainMenu() {
    glColor3f(0.1f, 0.1f, 0.1f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    glVertex2f(0, WINDOW_HEIGHT);
    glEnd();
    
    // Title
    glColor3f(0.95f, 0.85f, 0.2f);
    drawLargeText(WINDOW_WIDTH/2 - 80, WINDOW_HEIGHT - 100, "ICY TOWER");
    
    glColor3f(0.7f, 0.7f, 0.75f);
    drawText(WINDOW_WIDTH/2 - 90, WINDOW_HEIGHT - 140, "ASCEND TO VICTORY");
    
    // Start button with gradient effect
    glColor3f(0.15f, 0.55f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(startButtonX, startButtonY);
    glVertex2f(startButtonX + startButtonWidth, startButtonY);
    glColor3f(0.2f, 0.65f, 0.35f);
    glVertex2f(startButtonX + startButtonWidth, startButtonY + startButtonHeight);
    glVertex2f(startButtonX, startButtonY + startButtonHeight);
    glEnd();
    
    // Button border
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(startButtonX, startButtonY);
    glVertex2f(startButtonX + startButtonWidth, startButtonY);
    glVertex2f(startButtonX + startButtonWidth, startButtonY + startButtonHeight);
    glVertex2f(startButtonX, startButtonY + startButtonHeight);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    drawLargeText(startButtonX , startButtonY + 18, "START GAME");
    
    // Instructions
    glColor3f(0.6f, 0.6f, 0.65f);
    drawText(WINDOW_WIDTH/2 - 130, 200, "WASD / Arrow Keys - Move & Jump");
    drawText(WINDOW_WIDTH/2 - 100, 170, "Collect all coins to unlock door");
    drawText(WINDOW_WIDTH/2 - 80, 140, "Avoid rocks and lava!");
    
    // Decorative elements
    glColor3f(0.95f, 0.25f, 0.05f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 10; i++) {
        float x = 50 + i * 75;
        glVertex2f(x, 80);
        glVertex2f(x + 20, 95);
        glVertex2f(x + 40, 80);
    }
    glEnd();
}

void drawGameOver() {
    if (gameState == WIN) {
        glColor3f(0.2f, 0.8f, 0.3f);
        drawLargeText(WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/1.5, "YOU WIN!");
    } else {
        glColor3f(0.95f, 0.2f, 0.2f);
        drawLargeText(WINDOW_WIDTH/2 - 70, WINDOW_HEIGHT/1.5, "GAME OVER!");
    }
    
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", player.score);
    glColor3f(0.9f, 0.9f, 0.95f);
    drawText(WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/2 , scoreText);
    
    // Restart button with gradient
    glColor3f(0.15f, 0.55f, 0.25f);
    glBegin(GL_QUADS);
    glVertex2f(restartButtonX, restartButtonY);
    glVertex2f(restartButtonX + restartButtonWidth, restartButtonY);
    glColor3f(0.2f, 0.65f, 0.35f);
    glVertex2f(restartButtonX + restartButtonWidth, restartButtonY + restartButtonHeight);
    glVertex2f(restartButtonX, restartButtonY + restartButtonHeight);
    glEnd();
    
    // Button border
    glColor3f(0.3f, 0.75f, 0.45f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(restartButtonX, restartButtonY);
    glVertex2f(restartButtonX + restartButtonWidth, restartButtonY);
    glVertex2f(restartButtonX + restartButtonWidth, restartButtonY + restartButtonHeight);
    glVertex2f(restartButtonX, restartButtonY + restartButtonHeight);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    drawLargeText(restartButtonX , restartButtonY + 10, "PLAY AGAIN");
}


bool checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool checkCircleCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrt(dx * dx + dy * dy);
    return distance < (r1 + r2);
}

void update(int value) {
    if (gameState == MENU) {
        gameTime++;
        glutPostRedisplay();
        glutTimerFunc(16, update, 0);
        return;
    }
    
    if (gameState != PLAYING) {
        glutTimerFunc(16, update, 0);
        return;
    } 
    if (isPaused) {
        glutPostRedisplay();           // still redraw the scene (so Pause UI shows)
        glutTimerFunc(16, update, 0);  // keep timer running
        return;                        // exit early to freeze game logic
    }
    
    gameTime++;
    
    if (keys['a'] || keys['A']) {
        player.x -= MOVE_SPEED * 16;
        if (player.x < player.width/2) player.x = player.width/2;
    }
    if (keys['d'] || keys['D']) {
        player.x += MOVE_SPEED * 16;
        if (player.x > WINDOW_WIDTH - player.width/2) player.x = WINDOW_WIDTH - player.width/2;
    }
    
    if ((keys['w'] || keys['W'] || keys[' ']) && !player.isJumping) {
        player.velocityY = JUMP_VELOCITY;
        player.isJumping = true;
    }
    
    player.velocityY += GRAVITY * 16;
    player.y += player.velocityY * 16;
    
    for (auto& p : platforms) {
        if (p.destroyed) continue;
        
        if (player.velocityY <= 0 && 
            checkCollision(player.x - player.width/2, player.y, player.width, 5,
                          p.x, p.y + p.height - 5, p.width, 10)) {
            player.y = p.y + p.height;
            player.velocityY = 0;
            player.isJumping = false;
        }
    }
    
    if (player.y <= 30) {
        player.y = 30;
        player.velocityY = 0;
        player.isJumping = false;
    }
    
    if (player.y > WINDOW_HEIGHT) player.y = WINDOW_HEIGHT;
    
    float currentLavaSpeed = lavaSpeed;
    if (player.activePowerUp == 2) {
        currentLavaSpeed *= 0.3f;
    }
    lavaHeight += currentLavaSpeed;
    lavaSpeed += LAVA_SPEED_INCREMENT;
    
    if (player.y < lavaHeight + 20) {
        gameState = LOSE;
    }
    
    for (auto& p : platforms) {
        if (!p.destroyed && p.y < lavaHeight) {
            p.destroyed = true;
        }
    }
    
    if (gameTime - lastRockSpawn > 120 + rand() % 180) {
        Rock r;
        r.x = rand() % WINDOW_WIDTH;
        r.y = WINDOW_HEIGHT;
        r.size = 15;
        r.speed = 2.0f + (rand() % 100) / 100.0f;
        r.active = true;
        rocks.push_back(r);
        lastRockSpawn = gameTime;
    }
    
    for (auto& r : rocks) {
        if (!r.active) continue;
        
        r.y -= r.speed;
        
        if (player.activePowerUp != 1) {
            if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                    r.x, r.y, r.size)) {
                player.lives--;
                r.active = false;
                
                if (player.lives <= 0) {
                    gameState = LOSE;
                }
            }
        } else {
            if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2 + 10,
                                    r.x, r.y, r.size)) {
                r.active = false;
            }
        }
        
        if (r.y < -r.size) {
            r.active = false;
        }
    }
    
    for (auto& c : collectables) {
        if (c.collected) continue;
        
        c.rotation += 2.0f;
        
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                c.x, c.y, c.size)) {
            c.collected = true;
            player.score += 10;
            
            bool allCollected = true;
            for (auto& col : collectables) {
                if (!col.collected) {
                    allCollected = false;
                    break;
                }
            }
            
            if (allCollected && !key.spawned) {
                key.spawned = true;
                key.x = WINDOW_WIDTH / 2;
                key.y = door.y - 50;
            }
        }
    }
    
    if (key.spawned && !key.collected) {
        key.rotation += 3.0f;
        
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                key.x, key.y, key.size)) {
            key.collected = true;
            player.hasKey = true;
            door.unlocked = true;
        }
    }
    
    if (gameTime - powerUpSpawnTime > 600 && powerUps.size() < 2) {
        PowerUp pu;
        pu.x = rand() % (WINDOW_WIDTH - 100) + 50;
        pu.y = lavaHeight + 150 + rand() % 200;
        pu.size = 20;
        pu.type = (powerUps.size() == 0) ? 1 : 2;
        pu.collected = false;
        pu.timer = 300;
        pu.rotation = 0;
        powerUps.push_back(pu);
        powerUpSpawnTime = gameTime;
    }
    
    for (auto& pu : powerUps) {
        if (pu.collected) continue;
        
        pu.rotation += 5.0f;
        pu.timer--;
        
        if (pu.timer <= 0) {
            pu.collected = true;
        }
        
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                pu.x, pu.y, pu.size)) {
            pu.collected = true;
            player.activePowerUp = pu.type;
            player.powerUpTimer = 180;
        }
    }
    
    if (player.activePowerUp > 0) {
        player.powerUpTimer--;
        if (player.powerUpTimer <= 0) {
            player.activePowerUp = 0;
        }
    }
    
    if (door.unlocked && door.openAnimation < 1.0f) {
        door.openAnimation += 0.02f;
    }
    
    if (door.unlocked && 
        checkCollision(player.x - player.width/2, player.y, player.width, player.height,
                      door.x, door.y, door.width, door.height)) {
        gameState = WIN;
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
    
}
void drawBackground() {
    glBegin(GL_QUADS);
    
    // Top color (dark charcoal)
    glColor3f(0.07f, 0.05f, 0.05f);
    glVertex2f(0, WINDOW_HEIGHT);
    glVertex2f(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Bottom color (lava orange glow)
    glColor3f(0.35f, 0.12f, 0.05f);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(0, 0);
    
    glEnd();
}
void drawPauseButton() {
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(pauseButtonX, pauseButtonY);
        glVertex2f(pauseButtonX + pauseButtonWidth, pauseButtonY);
        glVertex2f(pauseButtonX + pauseButtonWidth, pauseButtonY + pauseButtonHeight);
        glVertex2f(pauseButtonX, pauseButtonY + pauseButtonHeight);
    glEnd();

    // Button border
    glColor3f(1.0f, 0.5f, 0.0f);
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
        glVertex2f(pauseButtonX, pauseButtonY);
        glVertex2f(pauseButtonX + pauseButtonWidth, pauseButtonY);
        glVertex2f(pauseButtonX + pauseButtonWidth, pauseButtonY + pauseButtonHeight);
        glVertex2f(pauseButtonX, pauseButtonY + pauseButtonHeight);
    glEnd();

    // Button text
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(pauseButtonX + 20, pauseButtonY + 12);
    std::string text = isPaused ? "Resume" : "Pause";
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (gameState == MENU) {
        drawMainMenu();
    } else if (gameState == PLAYING) {
        drawBackground();
        drawLava();
        drawPlatforms();
        drawCollectables();
        drawKey();
        drawPowerUps();
        drawDoor();
        drawRocks();
        drawPlayer();
        drawHUD();
    } else {
        drawGameOver();
    }
    if (gameState == PLAYING) {
    drawPauseButton();
}
    
    glutSwapBuffers();
}

void keyDown(unsigned char key, int x, int y) {
    keys[key] = true;
    
    if ((gameState == WIN || gameState == LOSE) && key == 'r') {
        platforms.clear();
        collectables.clear();
        rocks.clear();
        powerUps.clear();
        lavaHeight = 0.0f;
        lavaSpeed = LAVA_INITIAL_SPEED;
        gameTime = 0;
        lastRockSpawn = 0;
        powerUpSpawnTime = 0;
        gameState = PLAYING;
        init();
    }
}

void keyUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void specialKeyDown(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) keys['a'] = true;
    if (key == GLUT_KEY_RIGHT) keys['d'] = true;
    if (key == GLUT_KEY_UP) keys['w'] = true;
}

void specialKeyUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) keys['a'] = false;
    if (key == GLUT_KEY_RIGHT) keys['d'] = false;
    if (key == GLUT_KEY_UP) keys['w'] = false;
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int glY = WINDOW_HEIGHT - y;

        // --- Start button on menu ---
        if (gameState == MENU) {
            if (x >= startButtonX && x <= startButtonX + startButtonWidth &&
                glY >= startButtonY && glY <= startButtonY + startButtonHeight) {
                gameState = PLAYING;
                init();
                glutPostRedisplay();
            }
        }

        // --- Pause button during gameplay ---
        if (gameState == PLAYING) {
            if (x >= pauseButtonX && x <= pauseButtonX + pauseButtonWidth &&
                glY >= pauseButtonY && glY <= pauseButtonY + pauseButtonHeight) {
                isPaused = !isPaused;
                glutPostRedisplay();
            }
        }

        // --- Restart button on game over or win ---
        if (gameState == WIN || gameState == LOSE) {
            if (x >= restartButtonX && x <= restartButtonX + restartButtonWidth &&
                glY >= restartButtonY && glY <= restartButtonY + restartButtonHeight) {
                platforms.clear();
                collectables.clear();
                rocks.clear();
                powerUps.clear();
                lavaHeight = 0.0f;
                lavaSpeed = LAVA_INITIAL_SPEED;
                gameTime = 0;
                lastRockSpawn = 0;
                powerUpSpawnTime = 0;
                gameState = PLAYING;
                init();
                glutPostRedisplay();
            }
        }
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Icy Tower Platformer - Ascend to Victory!");
    
    init();
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutTimerFunc(16, update, 0);
    glutMouseFunc(mouse);
    
    glutMainLoop();
    return 0;
}