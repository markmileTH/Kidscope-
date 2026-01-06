#include <TFT_eSPI.h>
#include <SPI.h>
#define LONG_PRESS_TIME   800
#define RESET_PRESS_TIME  3000

// ================== TFT ==================
TFT_eSPI tft = TFT_eSPI();

// ================== BUTTON ==================
#define BTN_PIN 0   // ปุ่มเดียว
bool btnLast = HIGH;
unsigned long pressTime = 0;
// ================== ZOOM ==================
float zoomLevel = 2.0;      // 2x - 6x
const float ZOOM_MIN = 2.0;
const float ZOOM_MAX = 6.0;

// ================== SCALE ==================
//#define CM_TO_PIXEL 2.0   // 1 cm = 2 pixel
float cmToPixel() {
  return 2.0 * (zoomLevel / 2.0);
}


// ================== BALLISTIC PARAM ==================
float bulletSpeed   = 395.0;  // m/s
float bulletWeight  = 0.88;   // gram (reserved)
float targetDistance = 20.0;  // m
float windSpeed     = 0.0;    // m/s
//bool  steelBB       = false;

// Zero
float zeroDrop = 0.0;

// ================== CONST ==================
const float g = 9.81;
// ================== AMMO MODE ==================
enum AmmoMode {
  AMMO_LIGHT,
  AMMO_STANDARD,
  AMMO_HEAVY
};

AmmoMode ammoMode = AMMO_STANDARD;

struct AmmoPreset {
  const char* name;
  float speed;
  float weight;
};

AmmoPreset ammoTable[] = {
  { "LIGHT",    420.0, 0.70 },
  { "STD",      395.0, 0.88 },
  { "HEAVY",    330.0, 1.05 }
};

void loadAmmoMode() {
  bulletSpeed  = ammoTable[ammoMode].speed;
  bulletWeight = ammoTable[ammoMode].weight;
}

float getDragFactor() {
  return 1.0 + (1.0 / bulletWeight) * 0.15;
}

// ================== MENU ==================
enum Menu {
  MENU_DIST,
  MENU_WIND,
  MENU_SPEED,
  MENU_MODE,
  MENU_ZOOM,
  MENU_ZERO
};

Menu currentMenu = MENU_DIST;

// ================== DRAW ==================
void drawCrosshair() {
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;

  tft.drawLine(cx - 40, cy, cx + 40, cy, TFT_GREEN);
  tft.drawLine(cx, cy - 40, cx, cy + 40, TFT_GREEN);
}

void drawTargetPoint(float drop_m, float wind_m) {
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;

  float drop_cm = drop_m * 100.0;
  float wind_cm = wind_m * 100.0;

  int yOffset = drop_cm * cmToPixel();
  int xOffset = wind_cm * cmToPixel();

  int tx = cx + xOffset;
  int ty = cy + yOffset;

  tft.fillCircle(tx, ty, 4, TFT_RED);
  tft.drawCircle(tx, ty, 6, TFT_RED);
}

// ================== UI ==================
void drawMenuIndicator() {
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 170);

  switch (currentMenu) {
    case MENU_DIST:  tft.print("> DIST");  break;
    case MENU_WIND:  tft.print("> WIND");  break;
    case MENU_SPEED: tft.print("> SPEED"); break;
    case MENU_MODE:  tft.print("> MODE");  break;
    case MENU_ZOOM:  tft.print("> ZOOM");  break;
    case MENU_ZERO:  tft.print("> ZERO");  break;
  }
}

// ================== CALC ==================
void calculateAndDisplay() {
  tft.fillScreen(TFT_BLACK);
  drawCrosshair();

  float time = (targetDistance / bulletSpeed) * getDragFactor();
  float drop = 0.5 * g * time * time;
  float windDrift = windSpeed * time;

  float adjDrop = drop - zeroDrop;

  drawTargetPoint(adjDrop, windDrift);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);

  tft.printf("AMMO : %s\n", ammoTable[ammoMode].name);
  tft.printf("WT   : %.2f g\n", bulletWeight);
  tft.printf("DIST : %.1f m\n", targetDistance);
  tft.printf("SPD  : %.0f m/s\n", bulletSpeed);
  tft.printf("WIND : %.1f m/s\n", windSpeed);
  tft.printf("ZOOM : %.0fx\n", zoomLevel);
  //tft.printf("ZERO : %.1f m\n", targetDistance);

  drawMenuIndicator();
}

// ================== BUTTON LOGIC ==================
void shortPress() {
  switch (currentMenu) {
    case MENU_DIST:  targetDistance += 1; break;
    case MENU_WIND:  windSpeed += 0.2;    break;
    case MENU_SPEED: bulletSpeed += 5;    break;
    case MENU_MODE:  ammoMode = (AmmoMode)((ammoMode + 1) % 3); loadAmmoMode(); break;
    case MENU_ZOOM:  zoomLevel += 1.0; if (zoomLevel > ZOOM_MAX) zoomLevel = ZOOM_MIN; break;
    case MENU_ZERO:  break;
  }
  calculateAndDisplay();
}

void longPress() {
  if (currentMenu == MENU_ZERO) {
    float t = targetDistance / bulletSpeed;
    zeroDrop = 0.5 * g * t * t;
  }

  currentMenu = (Menu)((currentMenu + 1) % 5);
  calculateAndDisplay();
}
void handleButton() {
  bool btn = digitalRead(BTN_PIN);

  if (btn == LOW && btnLast == HIGH) {
    pressTime = millis();
  }

  if (btn == HIGH && btnLast == LOW) {
    unsigned long dt = millis() - pressTime;

    if (dt >= RESET_PRESS_TIME) {
      resetAll();                 // 🔥 กดค้าง 3 วิ
    }
    else if (dt >= LONG_PRESS_TIME) {
      longPress();                // กดค้างปกติ
    }
    else {
      shortPress();               // กดสั้น
    }
  }

  btnLast = btn;
}

void resetAll() {
  targetDistance = 0;
  windSpeed = 0;
  zeroDrop = 0;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.print("RESET DONE");

  delay(600);
  calculateAndDisplay();
}

// ================== SETUP ==================
void setup() {
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);   // Backlight
  loadAmmoMode();
  pinMode(BTN_PIN, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  calculateAndDisplay();
}

// ================== LOOP ==================
void loop() {
  handleButton();
}
