#include <TFT_eSPI.h>
#include <Arduino.h>

// -------------------------
// Display & Layout Constants
// -------------------------
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 135
#define MARGIN 1  // Border thickness to preserve

TFT_eSPI tft = TFT_eSPI(); // Configured for TTGO T-Display, rotation(1)

// -------------------------
// Game State – Base Model Game (with Max's upgrades)
// -------------------------
int charX, charY, charOldX, charOldY;
int charSize = 8;

struct Obstacle {
  int x, y;         // current position
  int w, h;         // width and height
  int oldX, oldY;   // previous position
  bool active;      // is active on screen?
  bool isMystery;   // if true, this obstacle is a mystery box that grants an extra life
};

#define MAX_OBSTACLES 5
Obstacle obstacles[MAX_OBSTACLES];
float obstacleSpeed = 4.0; // pixels per frame - now a float for gradual speed increases

int score = 0;
int highScore = 0; // Added high score tracking
bool gameOver = false;
unsigned long nextSpawnTime = 0;   // when next obstacle spawns
int nextMysteryScore = 20;         // spawn mystery when score reaches this

// Speed increase variables
unsigned long lastSpeedIncreaseTime = 0;
const unsigned long speedIncreaseInterval = 4000; // 4 seconds

// Effects: Lives, Invincibility, Special Move
int lives = 1;         // start with one life
const int maxLives = 3; // maximum lives allowed

bool invincible = false;
unsigned long invincibleEndTime = 0;  // ms timestamp

bool specialActive = false;
unsigned long specialEndTime = 0;
const unsigned long specialDuration = 1000;  // special move lasts 1 sec
int specialMoveType = 0; // chosen randomly when special move triggers

// Special animation variables
bool specialAnimationActive = false;
unsigned long specialAnimationEndTime = 0;

// -------------------------
// Button Settings – Physical buttons
// -------------------------
// Up: GPIO35, Down: GPIO0. (Both pressed trigger special move.)
const int buttonUpPin   = 35;
const int buttonDownPin = 0;

// -------------------------
// Timing & Frame Rate
// -------------------------
unsigned long lastFrameTime = 0;
const unsigned long frameInterval = 50; // ~20 FPS

// -------------------------
// Function Prototypes
// -------------------------
void safeFillRect(int x, int y, int w, int h, uint16_t color);
void eraseCharacter(int x, int y);
void drawCharacter(int x, int y);
void eraseObstacle(Obstacle &obs);
void drawObstacle(Obstacle &obs);
void updateScoreAndLivesDisplay();
void drawGameOverMessage();
void resetGame();
void handleButtons();
bool checkCollision(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh);
void spawnObstacle();
void performSpecialAnimation();
void cleanupSpecialAnimation();

// -------------------------
// Helper: Draw Rectangle Clipped to Safe Area (preserving border)
// -------------------------
void safeFillRect(int x, int y, int w, int h, uint16_t color) {
  int x1 = max(x, MARGIN);
  int y1 = max(y, MARGIN);
  int x2 = min(x + w, SCREEN_WIDTH - MARGIN);
  int y2 = min(y + h, SCREEN_HEIGHT - MARGIN);
  int wClipped = x2 - x1;
  int hClipped = y2 - y1;
  if (wClipped > 0 && hClipped > 0) {
    tft.fillRect(x1, y1, wClipped, hClipped, color);
  }
}

// -------------------------
// Setup()
// -------------------------
void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_WHITE);

  // Initialize player position
  charX = 20;
  charY = 50;
  charOldX = charX;
  charOldY = charY;
  drawCharacter(charX, charY);

  // Initialize obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
    obstacles[i].isMystery = false;
  }

  score = 0;
  lives = 1;
  updateScoreAndLivesDisplay();
  nextSpawnTime = millis() + random(500, 2000);
  nextMysteryScore = 20;
  lastSpeedIncreaseTime = millis(); // Initialize speed increase timer

  // Configure physical buttons
  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);

  Serial.println("Base Model Game started.");
}

// -------------------------
// Main Loop()
// -------------------------
void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastFrameTime < frameInterval) return;
  lastFrameTime = currentTime;

  // Update invincibility & special state
  if (invincible && currentTime > invincibleEndTime) invincible = false;
  if (specialActive && currentTime > specialEndTime) {
    specialActive = false;
    // After special move, remove any mystery boxes
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacles[i].active && obstacles[i].isMystery) {
        obstacles[i].active = false;
      }
    }
  }
  
  // Clean up special animations when they're done
  if (specialAnimationActive && currentTime > specialAnimationEndTime) {
    specialAnimationActive = false;
    cleanupSpecialAnimation();
  }

  // Increase speed every 4 seconds
  if (!gameOver && currentTime - lastSpeedIncreaseTime >= speedIncreaseInterval) {
    obstacleSpeed *= 1.04; // Increase speed by ~4%
    lastSpeedIncreaseTime = currentTime;
    Serial.print("Speed increased to: ");
    Serial.println(obstacleSpeed);
  }

  if (!gameOver) {
    // Handle inputs (physical buttons only)
    handleButtons();

    // Erase old character
    eraseCharacter(charOldX, charOldY);
    charOldX = charX;
    charOldY = charY;

    // Update obstacles: erase, move, redraw
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacles[i].active) {
        eraseObstacle(obstacles[i]);
        obstacles[i].oldX = obstacles[i].x;
        obstacles[i].oldY = obstacles[i].y;
        obstacles[i].x -= obstacleSpeed;
        if (obstacles[i].x + obstacles[i].w < MARGIN) {
          if (!obstacles[i].isMystery) {
            score++;
            if (score > highScore) {
              highScore = score; // Update high score
            }
            updateScoreAndLivesDisplay();
          }
          obstacles[i].active = false;
        } else {
          drawObstacle(obstacles[i]);
        }
      }
    }

    // Spawn obstacles or mystery boxes
    if (currentTime > nextSpawnTime) {
      // If score reached threshold and no mystery is active, spawn mystery box
      bool mysteryActive = false;
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (obstacles[i].active && obstacles[i].isMystery) { mysteryActive = true; break; }
      }
      if (score >= nextMysteryScore && !mysteryActive) {
        for (int i = 0; i < MAX_OBSTACLES; i++) {
          if (!obstacles[i].active) {
            obstacles[i].active = true;
            obstacles[i].isMystery = true;
            obstacles[i].x = SCREEN_WIDTH - MARGIN - 1;
            obstacles[i].oldX = obstacles[i].x;
            int obsHeight = 20;
            obstacles[i].h = obsHeight;
            int minY = MARGIN + 1;
            int maxY = SCREEN_HEIGHT - obsHeight - MARGIN - 1;
            obstacles[i].y = random(minY, maxY + 1);
            obstacles[i].oldY = obstacles[i].y;
            obstacles[i].w = random(10, 31);
            break;
          }
        }
        nextMysteryScore += 20;
      } else {
        spawnObstacle();
      }
      nextSpawnTime = currentTime + random(500, 2000);
    }

    // Draw character (with special animation if active)
    drawCharacter(charX, charY);

    // Collision detection
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (obstacles[i].active) {
        if (checkCollision(charX, charY, charSize, charSize,
                           obstacles[i].x, obstacles[i].y, obstacles[i].w, obstacles[i].h)) {
          if (obstacles[i].isMystery) {
            if (lives < maxLives) { lives++; }
            obstacles[i].active = false;
            updateScoreAndLivesDisplay();
          } else {
            if (!invincible && !specialActive) {
              if (lives > 1) {
                lives--;
                invincible = true;
                invincibleEndTime = currentTime + 3000; // 3 sec invincibility
                updateScoreAndLivesDisplay();
              } else {
                gameOver = true;
                if (score > highScore) {
                  highScore = score;
                }
                drawGameOverMessage();
                break;
              }
            }
          }
        }
      }
    }
  } else {
    // If game over, wait for both physical buttons to restart
    if (digitalRead(buttonUpPin) == LOW && digitalRead(buttonDownPin) == LOW) {
      resetGame();
    }
  }
}

// -------------------------
// Handle Buttons: Physical Buttons Only
// -------------------------
void handleButtons() {
  bool upPhysical = (digitalRead(buttonUpPin) == LOW);
  bool downPhysical = (digitalRead(buttonDownPin) == LOW);
  
  // If both buttons are pressed, trigger special move
  if (upPhysical && downPhysical && !specialActive) {
    specialActive = true;
    specialEndTime = millis() + specialDuration;
    specialMoveType = random(0, 2); // choose 0 or 1
    
    // Set up special animation timing
    specialAnimationActive = true;
    specialAnimationEndTime = specialEndTime;
    
    performSpecialAnimation();
    return; // skip normal movement
  }
  
  if (specialActive) return; // ignore movement during special move
  
  if (upPhysical) {
    charY -= 3;
    if (charY < MARGIN) charY = MARGIN;
  }
  if (downPhysical) {
    charY += 3;
    if (charY > SCREEN_HEIGHT - charSize - MARGIN) {
      charY = SCREEN_HEIGHT - charSize - MARGIN;
    }
  }
}

// -------------------------
// Collision Detection (Axis-Aligned Bounding Box)
// -------------------------
bool checkCollision(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return !(ax + aw <= bx || ax >= bx + bw || ay + ah <= by || ay >= by + bh);
}

// -------------------------
// Spawn a Regular Obstacle
// -------------------------
void spawnObstacle() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].active = true;
      obstacles[i].isMystery = false;
      obstacles[i].x = SCREEN_WIDTH - MARGIN - 1;
      obstacles[i].oldX = obstacles[i].x;
      int obsHeight = 20;
      obstacles[i].h = obsHeight;
      int minY = MARGIN + 1;
      int maxY = SCREEN_HEIGHT - obsHeight - MARGIN - 1;
      obstacles[i].y = random(minY, maxY + 1);
      obstacles[i].oldY = obstacles[i].y;
      obstacles[i].w = random(10, 41);
      break;
    }
  }
}

// -------------------------
// Drawing Functions
// -------------------------
void eraseCharacter(int x, int y) {
  safeFillRect(x, y, charSize, charSize, TFT_BLACK);
}

void drawCharacter(int x, int y) {
  if (specialActive) {
    if (specialMoveType == 0) {
      unsigned long elapsed = millis() - (specialEndTime - specialDuration);
      int radius = map(elapsed, 0, specialDuration, charSize, charSize + 10);
      tft.drawCircle(x + charSize/2, y + charSize/2, radius, TFT_MAGENTA);
      tft.fillRect(x, y, charSize, charSize, TFT_GREEN);
    }
    else if (specialMoveType == 1) {
      int numFlashes = 3;
      for (int i = 0; i < numFlashes; i++) {
        int offset = i * 2;
        tft.drawRect(x - offset, y - offset, charSize + offset*2, charSize + offset*2, TFT_CYAN);
      }
      tft.fillRect(x, y, charSize, charSize, TFT_GREEN);
    }
  }
  else if (invincible) {
    tft.fillRect(x, y, charSize, charSize, TFT_BLUE);
  } else {
    tft.fillRect(x, y, charSize, charSize, TFT_GREEN);
  }
}

// Clean up special animation effects
void cleanupSpecialAnimation() {
  // Erase any special animation effects that might be lingering
  if (specialMoveType == 0) {
    // Erase the circle animation
    int maxRadius = charSize + 10;
    // Draw a black circle slightly larger than the max radius to ensure complete erasure
    tft.drawCircle(charOldX + charSize/2, charOldY + charSize/2, maxRadius + 1, TFT_BLACK);
    tft.drawCircle(charOldX + charSize/2, charOldY + charSize/2, maxRadius, TFT_BLACK);
  }
  else if (specialMoveType == 1) {
    // Erase the rectangle animations
    int numFlashes = 3;
    int maxOffset = (numFlashes - 1) * 2;
    // Draw a black rectangle slightly larger than the max animation size
    tft.drawRect(charOldX - maxOffset - 1, charOldY - maxOffset - 1, 
                charSize + (maxOffset + 1)*2, charSize + (maxOffset + 1)*2, TFT_BLACK);
    tft.drawRect(charOldX - maxOffset, charOldY - maxOffset, 
                charSize + maxOffset*2, charSize + maxOffset*2, TFT_BLACK);
  }
  
  // Redraw the character in its current state
  drawCharacter(charX, charY);
}

void eraseObstacle(Obstacle &obs) {
  safeFillRect(obs.oldX, obs.oldY, obs.w, obs.h, TFT_BLACK);
}

void drawObstacle(Obstacle &obs) {
  if (obs.isMystery) {
    safeFillRect(obs.x, obs.y, obs.w, obs.h, TFT_MAGENTA);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
    int tx = obs.x + (obs.w - 6) / 2;
    int ty = obs.y + (obs.h - 8) / 2;
    tft.drawString("♥", tx, ty);
  } else {
    safeFillRect(obs.x, obs.y, obs.w, obs.h, TFT_RED);
  }
}

// -------------------------
// Special Move Animation Helper
// -------------------------
void performSpecialAnimation() {
  drawCharacter(charX, charY);
}

// -------------------------
// Score & Lives Display
// -------------------------
void updateScoreAndLivesDisplay() {
  // Clear score area
  tft.fillRect(2, 2, 100, 12, TFT_BLACK);
  
  // Display score and lives on left
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(5, 2);
  tft.print("Score: ");
  tft.print(score);
  tft.print(" L:");
  tft.print(lives);
  
  // Display high score on right
  tft.fillRect(SCREEN_WIDTH - 80, 2, 75, 12, TFT_BLACK);
  tft.setCursor(SCREEN_WIDTH - 75, 2);
  tft.print("HI: ");
  tft.print(highScore);
}

// -------------------------
// Game Over Display & Reset
// -------------------------
void drawGameOverMessage() {
  tft.setTextSize(2);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(60, SCREEN_HEIGHT / 2 - 10);
  tft.print("GAME OVER");
  
  // Show final score and high score
  tft.setTextSize(1);
  tft.setCursor(70, SCREEN_HEIGHT / 2 + 10);
  tft.print("Score: ");
  tft.print(score);
  tft.print(" High: ");
  tft.print(highScore);
}

void resetGame() {
  tft.fillRect(MARGIN, MARGIN, SCREEN_WIDTH - 2 * MARGIN, SCREEN_HEIGHT - 2 * MARGIN, TFT_BLACK);
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_WHITE);
  
  charX = 20;
  charY = 50;
  charOldX = charX;
  charOldY = charY;
  drawCharacter(charX, charY);
  
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
    obstacles[i].isMystery = false;
  }
  
  score = 0;
  lives = 1;
  obstacleSpeed = 40.0; // Reset speed
  lastSpeedIncreaseTime = millis(); // Reset speed increase timer
  updateScoreAndLivesDisplay();
  gameOver = false;
  invincible = false;
  specialActive = false;
  specialAnimationActive = false;
  nextMysteryScore = 20;
  nextSpawnTime = millis() + random(500, 2000);
}
