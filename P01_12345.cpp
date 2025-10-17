#include <GLUT/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <string>

// Game constants
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float GRAVITY = -0.0008f;
const float JUMP_VELOCITY = 0.6f;
const float MOVE_SPEED = 0.15f;
const int INITIAL_LIVES = 3;
const int COLLECTABLES_COUNT = 7;
const float LAVA_INITIAL_SPEED = 0.02f;
const float LAVA_SPEED_INCREMENT = 0.0001f;
const float restartButtonX = WINDOW_WIDTH / 2 - 50;
const float restartButtonY = (WINDOW_HEIGHT / 2) - 80;
float restartButtonWidth = 100;
float restartButtonHeight = 30;


enum GameState { PLAYING, WIN, LOSE };
GameState gameState = PLAYING;


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
    int type; // 1=shield, 2=slow_lava
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

// Game objects
std::vector<Platform> platforms;
std::vector<Collectable> collectables;
std::vector<Rock> rocks;
std::vector<PowerUp> powerUps;
Key key;
Door door;

// Lava
float lavaHeight = 0.0f;
float lavaSpeed = LAVA_INITIAL_SPEED;

// Timing
int lastRockSpawn = 0;
int gameTime = 0;
int powerUpSpawnTime = 0;

// Input
bool keys[256];

// Initialize game
void init() {
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    
    srand(time(0));
    
    // Initialize player
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
    
    // Create platforms with varying sizes
    float platformY = 100;
    float platformWidths[] = {80, 120, 100, 90, 110, 85, 95, 105, 80, 90};
    
    for (int i = 0; i < 10; i++) {
        Platform p;
        p.width = platformWidths[i];
        p.height = 15;
        p.y = platformY;
        p.x = rand() % (WINDOW_WIDTH - (int)p.width);
        p.destroyed = false;
        platforms.push_back(p);
        platformY += 55;
    }
    
    // Create collectables
    for (int i = 0; i < COLLECTABLES_COUNT; i++) {
        Collectable c;
        c.x = rand() % (WINDOW_WIDTH - 40) + 20;
        c.y = 150 + i * 70;
        c.size = 15;
        c.collected = false;
        c.rotation = 0;
        collectables.push_back(c);
    }
    
    // Initialize key
    key.spawned = false;
    key.collected = false;
    key.size = 20;
    key.rotation = 0;
    
    // Initialize door
    door.x = WINDOW_WIDTH / 2 - 40;
    door.y = platformY + 20;
    door.width = 80;
    door.height = 60;
    door.unlocked = false;
    door.openAnimation = 0;
    
    // Initialize input
    for (int i = 0; i < 256; i++) keys[i] = false;
}

// Draw text
void drawText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text);
        text++;
    }
}

// Draw player
void drawPlayer() {
    float x = player.x;
    float y = player.y;
    float w = player.width;
    float h = player.height;
    
    // Shield effect
    if (player.activePowerUp == 1) {
        glColor3f(0.3f, 0.6f, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y + h/2);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(x + cos(angle) * (w/2 + 8), y + h/2 + sin(angle) * (h/2 + 8));
        }
        glEnd();
    }
    
    // Body (quad)
    glColor3f(0.2f, 0.7f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(x - w/2, y);
    glVertex2f(x + w/2, y);
    glVertex2f(x + w/2, y + h * 0.6f);
    glVertex2f(x - w/2, y + h * 0.6f);
    glEnd();
    
    // Head (triangle fan = circle)
    glColor3f(0.9f, 0.7f, 0.5f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y + h * 0.85f);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(x + cos(angle) * w/3, y + h * 0.85f + sin(angle) * w/3);
    }
    glEnd();
    
    // Eyes (points)
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(4);
    glBegin(GL_POINTS);
    glVertex2f(x - w/6, y + h * 0.88f);
    glVertex2f(x + w/6, y + h * 0.88f);
    glEnd();
    
    // Arms (lines)
    glLineWidth(3);
    glColor3f(0.9f, 0.7f, 0.5f);
    glBegin(GL_LINES);
    glVertex2f(x - w/2, y + h * 0.5f);
    glVertex2f(x - w/2 - 8, y + h * 0.3f);
    glVertex2f(x + w/2, y + h * 0.5f);
    glVertex2f(x + w/2 + 8, y + h * 0.3f);
    glEnd();
}

// Draw platforms
void drawPlatforms() {
    for (auto& p : platforms) {
        if (p.destroyed) continue;
        
        // Main platform (quad)
        glColor3f(0.5f, 0.3f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(p.x, p.y);
        glVertex2f(p.x + p.width, p.y);
        glVertex2f(p.x + p.width, p.y + p.height);
        glVertex2f(p.x, p.y + p.height);
        glEnd();
        
        // Top highlight (triangles)
        glColor3f(0.7f, 0.5f, 0.2f);
        glBegin(GL_TRIANGLES);
        glVertex2f(p.x, p.y + p.height);
        glVertex2f(p.x + p.width/2, p.y + p.height);
        glVertex2f(p.x + p.width/4, p.y + p.height + 3);
        
        glVertex2f(p.x + p.width/2, p.y + p.height);
        glVertex2f(p.x + p.width, p.y + p.height);
        glVertex2f(p.x + p.width * 0.75f, p.y + p.height + 3);
        glEnd();
        
        // Side stripes (polygon)
        glColor3f(0.4f, 0.2f, 0.05f);
        glBegin(GL_POLYGON);
        glVertex2f(p.x, p.y);
        glVertex2f(p.x + 5, p.y);
        glVertex2f(p.x + 5, p.y + p.height);
        glVertex2f(p.x, p.y + p.height);
        glEnd();
    }
}

// Draw collectables (coins)
void drawCollectables() {
    for (auto& c : collectables) {
        if (c.collected) continue;
        
        glPushMatrix();
        glTranslatef(c.x, c.y, 0);
        glRotatef(c.rotation, 0, 0, 1);
        
        // Outer circle (triangle fan)
        glColor3f(1.0f, 0.84f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(cos(angle) * c.size, sin(angle) * c.size);
        }
        glEnd();
        
        // Inner circle (triangle fan)
        glColor3f(0.8f, 0.64f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0, 0);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(cos(angle) * c.size * 0.6f, sin(angle) * c.size * 0.6f);
        }
        glEnd();
        
        // Star pattern (triangles)
        glColor3f(1.0f, 0.94f, 0.4f);
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

// Draw rocks
void drawRocks() {
    for (auto& r : rocks) {
        if (!r.active) continue;
        
        // Rock body (polygon - pentagon)
        glColor3f(0.4f, 0.4f, 0.4f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 5; i++) {
            float angle = i * 2.0f * 3.14159f / 5;
            glVertex2f(r.x + cos(angle) * r.size, r.y + sin(angle) * r.size);
        }
        glEnd();
        
        // Rock highlights (triangles)
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_TRIANGLES);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x - r.size/2, r.y + r.size/2);
        glVertex2f(r.x + r.size/2, r.y + r.size/2);
        glEnd();
    }
}

// Draw lava
void drawLava() {
    glColor3f(1.0f, 0.3f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(WINDOW_WIDTH, 0);
    glVertex2f(WINDOW_WIDTH, lavaHeight);
    glVertex2f(0, lavaHeight);
    glEnd();
    
    // Lava waves (triangles)
    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < WINDOW_WIDTH; i += 40) {
        float wave = sin((i + gameTime * 0.1f) * 0.1f) * 10;
        glVertex2f(i, lavaHeight);
        glVertex2f(i + 20, lavaHeight + 15 + wave);
        glVertex2f(i + 40, lavaHeight);
    }
    glEnd();
}

// Draw key
void drawKey() {
    if (!key.spawned || key.collected) return;
    
    glPushMatrix();
    glTranslatef(key.x, key.y, 0);
    glRotatef(key.rotation, 0, 0, 1);
    
    // Key head (circle - triangle fan)
    glColor3f(1.0f, 0.84f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(cos(angle) * key.size * 0.6f, sin(angle) * key.size * 0.6f);
    }
    glEnd();
    
    // Key hole (triangle fan - smaller)
    glColor3f(0.5f, 0.3f, 0.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0);
    for (int i = 0; i <= 20; i++) {
        float angle = i * 2.0f * 3.14159f / 20;
        glVertex2f(cos(angle) * key.size * 0.2f, sin(angle) * key.size * 0.2f);
    }
    glEnd();
    
    // Key shaft (quad)
    glColor3f(1.0f, 0.84f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(key.size * 0.3f, -key.size * 0.2f);
    glVertex2f(key.size * 1.5f, -key.size * 0.2f);
    glVertex2f(key.size * 1.5f, key.size * 0.2f);
    glVertex2f(key.size * 0.3f, key.size * 0.2f);
    glEnd();
    
    // Key teeth (triangles)
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

// Draw power-ups
void drawPowerUps() {
    for (auto& pu : powerUps) {
        if (pu.collected) continue;
        
        glPushMatrix();
        glTranslatef(pu.x, pu.y, 0);
        
        if (pu.type == 1) { // Shield
            glRotatef(pu.rotation, 0, 0, 1);
            glColor3f(0.3f, 0.6f, 1.0f);
            
            // Shield outline (triangle fan)
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (int i = 0; i <= 20; i++) {
                float angle = i * 2.0f * 3.14159f / 20;
                glVertex2f(cos(angle) * pu.size, sin(angle) * pu.size);
            }
            glEnd();
            
            // Shield inner (polygon)
            glColor3f(0.5f, 0.7f, 1.0f);
            glBegin(GL_POLYGON);
            glVertex2f(0, pu.size * 0.7f);
            glVertex2f(-pu.size * 0.5f, pu.size * 0.3f);
            glVertex2f(-pu.size * 0.5f, -pu.size * 0.5f);
            glVertex2f(0, -pu.size * 0.7f);
            glVertex2f(pu.size * 0.5f, -pu.size * 0.5f);
            glVertex2f(pu.size * 0.5f, pu.size * 0.3f);
            glEnd();
            
            // Plus sign (quads)
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
            
        } else { // Slow lava
            float scale = 1.0f + sin(pu.rotation * 0.05f) * 0.2f;
            glScalef(scale, scale, 1.0f);
            
            glColor3f(0.0f, 0.8f, 1.0f);
            
            // Snowflake center (triangle fan)
            glBegin(GL_TRIANGLE_FAN);
            glVertex2f(0, 0);
            for (int i = 0; i <= 20; i++) {
                float angle = i * 2.0f * 3.14159f / 20;
                glVertex2f(cos(angle) * pu.size * 0.3f, sin(angle) * pu.size * 0.3f);
            }
            glEnd();
            
            // Snowflake spikes (triangles)
            glColor3f(0.5f, 0.9f, 1.0f);
            glBegin(GL_TRIANGLES);
            for (int i = 0; i < 6; i++) {
                float angle = i * 3.14159f / 3;
                glVertex2f(0, 0);
                glVertex2f(cos(angle) * pu.size * 0.3f, sin(angle) * pu.size * 0.3f);
                glVertex2f(cos(angle) * pu.size, sin(angle) * pu.size);
            }
            glEnd();
            
            // Star pattern (lines)
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

// Draw door
void drawDoor() {
    float x = door.x;
    float y = door.y;
    float w = door.width;
    float h = door.height;
    
    if (door.unlocked) {
        // Open door animation
        glPushMatrix();
        glTranslatef(x, y, 0);
        glRotatef(-door.openAnimation * 90, 0, 0, 1);
        glTranslatef(-x, -y, 0);
        
        // Door panel (quad)
        glColor3f(0.4f, 0.25f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w/2, y);
        glVertex2f(x + w/2, y + h);
        glVertex2f(x, y + h);
        glEnd();
        
        glPopMatrix();
        
        // Door frame (polygon)
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_POLYGON);
        glVertex2f(x - 5, y);
        glVertex2f(x + w + 5, y);
        glVertex2f(x + w + 5, y + h + 10);
        glVertex2f(x + w/2, y + h + 20);
        glVertex2f(x - 5, y + h + 10);
        glEnd();
        
    } else {
        // Locked door - door panel (quad)
        glColor3f(0.4f, 0.25f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
        
        // Door frame (polygon)
        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_POLYGON);
        glVertex2f(x - 5, y);
        glVertex2f(x + w + 5, y);
        glVertex2f(x + w + 5, y + h + 10);
        glVertex2f(x + w/2, y + h + 20);
        glVertex2f(x - 5, y + h + 10);
        glEnd();
        
        // Lock (triangle fan)
        glColor3f(0.8f, 0.7f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x + w/2, y + h/2);
        for (int i = 0; i <= 20; i++) {
            float angle = i * 2.0f * 3.14159f / 20;
            glVertex2f(x + w/2 + cos(angle) * 8, y + h/2 + sin(angle) * 8);
        }
        glEnd();
        
        // Keyhole (triangles)
        glColor3f(0.2f, 0.1f, 0.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(x + w/2 - 3, y + h/2);
        glVertex2f(x + w/2 + 3, y + h/2);
        glVertex2f(x + w/2, y + h/2 - 8);
        glEnd();
    }
}

// Draw HUD
void drawHUD() {
    // Health bar background
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 30);
    glVertex2f(160, WINDOW_HEIGHT - 30);
    glVertex2f(160, WINDOW_HEIGHT - 10);
    glVertex2f(10, WINDOW_HEIGHT - 10);
    glEnd();
    
    // Health bar (hearts)
    for (int i = 0; i < player.lives; i++) {
        glColor3f(1.0f, 0.2f, 0.2f);
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
    
    // Lava danger bar background
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 60);
    glVertex2f(210, WINDOW_HEIGHT - 60);
    glVertex2f(210, WINDOW_HEIGHT - 40);
    glVertex2f(10, WINDOW_HEIGHT - 40);
    glEnd();
    
    // Lava danger bar
    float danger = lavaHeight / WINDOW_HEIGHT;
    float barWidth = 200 * danger;
    glColor3f(1.0f - danger, danger, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(10, WINDOW_HEIGHT - 60);
    glVertex2f(10 + barWidth, WINDOW_HEIGHT - 60);
    glVertex2f(10 + barWidth, WINDOW_HEIGHT - 40);
    glVertex2f(10, WINDOW_HEIGHT - 40);
    glEnd();
    
    // Score
    glColor3f(1.0f, 1.0f, 1.0f);
    char scoreText[50];
    sprintf(scoreText, "Score: %d", player.score);
    drawText(WINDOW_WIDTH - 150, WINDOW_HEIGHT - 25, scoreText);
}

// Draw game over screen
void drawGameOver() {
    glColor3f(0.8f, 0.1f, 0.1f);
    if (gameState == WIN) {
        drawText(WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/1.5, "YOU WIN!");
    } else {
        drawText(WINDOW_WIDTH/2 - 50, WINDOW_HEIGHT/1.5, "GAME OVER!");
    }
    
    char scoreText[50];
    sprintf(scoreText, "Final Score: %d", player.score);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(WINDOW_WIDTH/2 - 60, WINDOW_HEIGHT/2 - 40, scoreText);
   glColor3f(0.1f, 0.5f, 0.1f);  // Green color for the button
       glBegin(GL_QUADS);
       glVertex2f(restartButtonX, restartButtonY);
       glVertex2f(restartButtonX + restartButtonWidth, restartButtonY);
       glVertex2f(restartButtonX + restartButtonWidth, restartButtonY + restartButtonHeight);
       glVertex2f(restartButtonX, restartButtonY + restartButtonHeight);
       glEnd();
       
       glColor3f(1.0f, 1.0f, 1.0f);  // White text
       drawText(restartButtonX + 20, restartButtonY + 10, "Restart");

}

// Collision detection
bool checkCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

bool checkCircleCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float distance = sqrt(dx * dx + dy * dy);
    return distance < (r1 + r2);
}

// Update game state
void update(int value) {
    if (gameState != PLAYING) {
        glutTimerFunc(16, update, 0);
        return;
    }
    
    gameTime++;
    
    // Player movement
    if (keys['a'] || keys['A']) {
        player.x -= MOVE_SPEED * 16;
        if (player.x < player.width/2) player.x = player.width/2;
    }
    if (keys['d'] || keys['D']) {
        player.x += MOVE_SPEED * 16;
        if (player.x > WINDOW_WIDTH - player.width/2) player.x = WINDOW_WIDTH - player.width/2;
    }
    
    // Jump
    if ((keys['w'] || keys['W'] || keys[' ']) && !player.isJumping) {
        player.velocityY = JUMP_VELOCITY;
        player.isJumping = true;
    }
    
    // Apply gravity
    player.velocityY += GRAVITY * 16;
    player.y += player.velocityY * 16;
    
    // Platform collision
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
    
    // Ground collision
    if (player.y <= 30) {
        player.y = 30;
        player.velocityY = 0;
        player.isJumping = false;
    }
    
    // Keep player in bounds
    if (player.y > WINDOW_HEIGHT) player.y = WINDOW_HEIGHT;
    
    // Update lava
    float currentLavaSpeed = lavaSpeed;
    if (player.activePowerUp == 2) {
        currentLavaSpeed *= 0.3f; // Slow lava power-up
    }
    lavaHeight += currentLavaSpeed;
    lavaSpeed += LAVA_SPEED_INCREMENT;
    
    // Check lava collision with player
    if (player.y < lavaHeight + 20) {
        gameState = LOSE;
    }
    
    // Destroy platforms touched by lava
    for (auto& p : platforms) {
        if (!p.destroyed && p.y < lavaHeight) {
            p.destroyed = true;
        }
    }
    
    // Spawn rocks
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
    
    // Update rocks
    for (auto& r : rocks) {
        if (!r.active) continue;
        
        r.y -= r.speed;
        
        // Check collision with player
        if (player.activePowerUp != 1) { // No collision if shield active
            if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                    r.x, r.y, r.size)) {
                player.lives--;
                r.active = false;
                
                if (player.lives <= 0) {
                    gameState = LOSE;
                }
            }
        } else {
            // Rock bounces off shield
            if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2 + 10,
                                    r.x, r.y, r.size)) {
                r.active = false;
            }
        }
        
        // Remove rocks below screen
        if (r.y < -r.size) {
            r.active = false;
        }
    }
    
    // Update collectables
    for (auto& c : collectables) {
        if (c.collected) continue;
        
        c.rotation += 2.0f;
        
        // Check collision with player
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                c.x, c.y, c.size)) {
            c.collected = true;
            player.score += 10;
            
            // Check if all collectables collected
            bool allCollected = true;
            for (auto& col : collectables) {
                if (!col.collected) {
                    allCollected = false;
                    break;
                }
            }
            
            // Spawn key
            if (allCollected && !key.spawned) {
                key.spawned = true;
                key.x = WINDOW_WIDTH / 2;
                key.y = door.y - 80;
            }
        }
    }
    
    // Update key
    if (key.spawned && !key.collected) {
        key.rotation += 3.0f;
        
        // Check collision with player
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                key.x, key.y, key.size)) {
            key.collected = true;
            player.hasKey = true;
            door.unlocked = true;
        }
    }
    
    // Spawn power-ups
    if (gameTime - powerUpSpawnTime > 600 && powerUps.size() < 2) {
        PowerUp pu;
        pu.x = rand() % (WINDOW_WIDTH - 100) + 50;
        pu.y = lavaHeight + 150 + rand() % 200;
        pu.size = 20;
        pu.type = (powerUps.size() == 0) ? 1 : 2; // Alternate types
        pu.collected = false;
        pu.timer = 300; // 5 seconds to collect
        pu.rotation = 0;
        powerUps.push_back(pu);
        powerUpSpawnTime = gameTime;
    }
    
    // Update power-ups
    for (auto& pu : powerUps) {
        if (pu.collected) continue;
        
        pu.rotation += 5.0f;
        pu.timer--;
        
        // Disappear if not collected in time
        if (pu.timer <= 0) {
            pu.collected = true;
        }
        
        // Check collision with player
        if (checkCircleCollision(player.x, player.y + player.height/2, player.width/2,
                                pu.x, pu.y, pu.size)) {
            pu.collected = true;
            player.activePowerUp = pu.type;
            player.powerUpTimer = 180; // 3 seconds effect
        }
    }
    
    // Update active power-up timer
    if (player.activePowerUp > 0) {
        player.powerUpTimer--;
        if (player.powerUpTimer <= 0) {
            player.activePowerUp = 0;
        }
    }
    
    // Update door animation
    if (door.unlocked && door.openAnimation < 1.0f) {
        door.openAnimation += 0.02f;
    }
    
    // Check if player reaches door
    if (door.unlocked && 
        checkCollision(player.x - player.width/2, player.y, player.width, player.height,
                      door.x, door.y, door.width, door.height)) {
        gameState = WIN;
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

// Display function
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    if (gameState == PLAYING) {
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
    
    glutSwapBuffers();
}

// Keyboard down
void keyDown(unsigned char key, int x, int y) {
    keys[key] = true;
    
    // Restart game
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

// Keyboard up
void keyUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

// Special key down (arrow keys)
void specialKeyDown(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) keys['a'] = true;
    if (key == GLUT_KEY_RIGHT) keys['d'] = true;
    if (key == GLUT_KEY_UP) keys['w'] = true;
}

// Special key up
void specialKeyUp(int key, int x, int y) {
    if (key == GLUT_KEY_LEFT) keys['a'] = false;
    if (key == GLUT_KEY_RIGHT) keys['d'] = false;
    if (key == GLUT_KEY_UP) keys['w'] = false;
}
  void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && (gameState == WIN || gameState == LOSE)) {
        int glY = WINDOW_HEIGHT - y; 
        
        // Now check if the click is within the button bounds using OpenGL coordinates
        if (x >= restartButtonX && x <= restartButtonX + restartButtonWidth &&
            glY >= restartButtonY && glY <= restartButtonY + restartButtonHeight) {
            // Restart the game (same code as in keyDown for 'r')
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
            glutPostRedisplay();  // Redraw the screen
        }
    }
}

   

// Main function
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