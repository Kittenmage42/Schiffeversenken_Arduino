#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#if defined(ARDUINO_FEATHER_ESP32) // Feather Huzzah32
  #define TFT_CS         14
  #define TFT_RST        15
  #define TFT_DC         32

#elif defined(ESP8266)
  #define TFT_CS         4
  #define TFT_RST        16                                            
  #define TFT_DC         5

#else
  #define TFT_CS        10
  #define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         8
#endif


/*

Display-Pin	Arduino Uno R3 Pin	Beschreibung
GND	        GND	                Masse
VCC	        5V	                Stromversorgung
LED	        3.3V oder 5V	      Backlight (oft über 100–330 Ω Widerstand optional)
SCK	        13	                SPI Clock
SDA / MOSI	11	                SPI Data Out
AO / DC	    8	                  Data/Command
RESET	      9	                  Display Reset
CS	        10	                SPI Chip Select

*/

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


#define VRx A0
#define VRy A1
#define bPin 2
#define bfw 8 //BattleFieldWidth
#define bfh 8 //BattleFieldHeight
#define buzzerPin 6
#define JSEmpf 200 // Joystickempfindlichkeit



long long unsigned int tick = 0;
int bf[bfw][bfh];
int cx = 0; // Cursor Position
int cy = 0; 
int xz; // Joysticknullwerte //x zero
int yz;
int jx; // Joystickwerte für weitere Verwendung
int jy;
bool jb; // Knopf
bool debug;
int ammo = 18;
int shipcount;
const int highscore = EEPROM.get(0, highscore);

float p = 3.1415926;




void setup(void) {


  randomSeed(analogRead(A5));

  Serial.begin(2000000);
  Serial.print("Aktueller Highscore: "); Serial.println(highscore);


  tft.initR(INITR_BLACKTAB);
  tft.setRotation(3);

  calJs(VRx, VRy, bPin);

  //Debug Connector
  pinMode(12, INPUT_PULLUP);
  debug = !digitalRead(12);


  // TUTORIAL
  if (!debug) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextWrap(true);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(0, 20);
    tft.setTextSize(2);
    tft.print("  Schiffe-\n\n  versenken\n\n");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    delay(500);
    tft.print(" (Knopf des Joysticks\n druecken um fortzufahren)");
    while(digitalRead(bPin)) {}
    delay(50);

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(0, 0);
    tft.print("Anleitung: \n\n");
    tft.setTextSize(1);
    tft.print("\n Waehle mithilfe des\n Joysticks ein Feld aus."); 
    delay(50);
    while(digitalRead(bPin)) {}
    delay(50);

    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK); 
    tft.print("\n Nutze dann den im\n Joystick eingebauten\n Knopf, um auf dieses Feld\n zu schiessen."); 
    delay(50);
    while(digitalRead(bPin)) {}
    delay(50);

    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK); 
    tft.print("\n Wenn du ein Schiff\n getroffen hast, faerbt\n sich das Feld "); 
    tft.setTextColor(ST77XX_GREEN); 
    tft.print("gruen"); 
    tft.setTextColor(ST77XX_WHITE); 
    tft.print(" und\n du bekommst einen\n Extraversuch. "); 
    delay(50);
    while(digitalRead(bPin)) {}
    delay(50);

    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK); 
    tft.print("\n Wenn nicht, wird es "); 
    tft.setTextColor(ST77XX_RED); 
    tft.print("rot"); 
    tft.setTextColor(ST77XX_WHITE);
    tft.print("\n und dir wird ein Versuch\n abgezogen. "); 
    delay(50);
    while(digitalRead(bPin)) {}
    delay(50);

    tft.setCursor(0, 0);
    tft.fillScreen(ST77XX_BLACK); 
    tft.print("\n Dein Ziel ist es, alle 16 Schiffe mit begrenzt\n vielen Versuchen zu\n finden und zu zerstoeren.\n\n       Viel Glueck! ");
    delay(50);
    while(digitalRead(bPin)) {}
    delay(50);
  }

  tft.fillScreen(ST77XX_BLACK);



  for (int i = 0; i < bfw; i++) {
    for (int c = 0; c < bfh; c++) {
      Serial.print("BF: ");
      Serial.print(i);
      Serial.print(',');
      Serial.print(c);
      Serial.println(" set to 0");
      bf[i][c] = 0;
    }
  }
  Serial.println("Done setting BF default values!");

  setShips(16, 2, 5);
  renderBF();
  rVnS();

}

void loop() {
  tick++;

  refJs();
  getCPos();
  
  /*
  Serial.print(jx);
  Serial.print(" / ");
  Serial.print(jy);
  Serial.print(" / ");
  Serial.print(cx);
  Serial.print(" / ");
  Serial.print(cy);
  Serial.print('\n');
  */


  renderBF();

  if (!digitalRead(bPin) && (bf[cx][cy] < 2) && ammo > 0) {
    if (bf[cx][cy] == 0) { ammo--; tone(buzzerPin, 800, 400); }
    else if (bf[cx][cy] == 1) { shipcount--; ammo++; tone(buzzerPin, 1800, 100); delay(100); tone(buzzerPin, 2000, 100); }
    bf[cx][cy] += 2;
    rVnS();
  }

  delay(100);

  if(shipcount == 0) WINFUNC();
  if(ammo == 0) LOSEFUNC();
}

// Funktionen

void calJs(int x, int y, int b) { // Calibrate Joystick
  xz = analogRead(x);
  yz = analogRead(y);
  pinMode(b, INPUT_PULLUP);
}

void refJs() { // Refresh Joystick Values
  jx = -(analogRead(VRx) - xz);
  jy = -(analogRead(VRy) - yz); 
  jb = digitalRead(bPin);
}

void getCPos() {
  drawX(cx * 14 + 4, cy * 14 + 4, ST77XX_BLACK);
  if (jx > JSEmpf) {
    cx += 1;
  } else if (jx < -JSEmpf) {
    cx -= 1;
  }
  //cx = cx % bfw;
  if (jy > JSEmpf) {
    cy += 1;
  } else if (jy < -JSEmpf) {
    cy -= 1;
  }

  cx = (cx + bfw) % bfw;
  cy = (cy + bfh) % bfh;
}

void renderBF() {

  /*
    Grid State Cheatsheet: 
      0 = Nichts da, noch nicht beschossen
      1 = etwas da, noch nicht beschossen
      2 = nichts da, beschossen
      3 = etwas da, schiff zerstört
  */

  for (int i = 0; i < bfw; i++) {

    for (int c = 0; c < bfh; c++) {
      if (bf[i][c] == 0) tft.drawRect(i * 14, c * 14, 9, 9, ST77XX_WHITE); 
      else if (bf[i][c] == 1) tft.drawRect(i * 14, c * 14, 9, 9, debug ? ST77XX_MAGENTA : ST77XX_WHITE);
      else if (bf[i][c] == 2) tft.drawRect(i * 14, c * 14, 9, 9, ST77XX_RED);
      else if (bf[i][c] == 3) tft.drawRect(i * 14, c * 14, 9, 9, ST77XX_GREEN);
      else tft.drawRect(i * 14, c * 14, 9, 9, ST77XX_BLUE);
    }
  }


  drawX(cx * 14 + 4, cy * 14 + 4, ST77XX_WHITE);

}

void rVnS() { // GUI / HUD
  tft.setTextWrap(false);
  tft.fillRect(0, 115, 160, 128, ST77XX_BLACK);
  tft.setCursor(0, 115);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.print(ammo);
  tft.println(" Vers.");
  tft.setCursor(80, 115);
  tft.print(shipcount);
  tft.println(" Schiffe");
}

bool setShip(uint16_t size) {
    int x, y;
    bool horizontal, positive;
    int retries = 0;
    const int MAX_RETRIES = 200;   // etwas höher, da zusätzliche Logik für size=1
    bool placed = false;

    // --- NEU: Wenn Schiffgröße = 1: nur neben andere Schiffe setzen ---
    if (size == 1) {
        while (!placed && retries < MAX_RETRIES) {
            retries++;

            // zufälliges leeres Feld finden
            x = random(0, bfw);
            y = random(0, bfh);
            if (bf[x][y] != 0) continue;

            // Prüfen, ob ein Nachbar ein Schiff ist
            bool neighborShip = false;

            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {

                    if (dx == 0 && dy == 0) continue; // sich selbst überspringen

                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0 || ny < 0 || nx >= bfw || ny >= bfh) continue;

                    if (bf[nx][ny] == 1) {
                        neighborShip = true;
                        break;
                    }
                }
                if (neighborShip) break;
            }

            if (!neighborShip) continue;  // weiter suchen

            // Position akzeptieren
            bf[x][y] = 1;
            placed = true;
        }
        return placed;
    }

    // --- ALTES VERHALTEN für Schiffe mit Größe > 1 ---
    while (!placed && retries < MAX_RETRIES) {
        retries++;
        horizontal = random(0, 2);
        positive   = random(0, 2);

        if (horizontal) {
            x = positive ? random(0, 8 - size + 1) : random(size - 1, 8);
            y = random(0, 8);
        } else {
            x = random(0, 8);
            y = positive ? random(0, 8 - size + 1) : random(size - 1, 8);
        }

        bool canPlace = true;

        for (int c = 0; c < size; c++) {
            int nx = horizontal ? (positive ? x + c : x - c) : x;
            int ny = horizontal ? y : (positive ? y + c : y - c);

            if (bf[nx][ny] != 0) { 
                canPlace = false; 
                break; 
            }
        }

        if (canPlace) {
            for (int c = 0; c < size; c++) {
                int nx = horizontal ? (positive ? x + c : x - c) : x;
                int ny = horizontal ? y : (positive ? y + c : y - c);
                bf[nx][ny] = 1;
            }
            placed = true;
        }
    }

    return placed;
}




void setShips(uint16_t amount, uint8_t minSize, uint8_t maxSize) {
    uint16_t placed = 0; // größerer Datentyp
    while (placed < amount) {
        uint8_t size = random(minSize, maxSize + 1); // maxSize inklusive
        if (placed + size > amount) size = amount - placed;

        bool success = setShip(size);
        if (!success) {
            Serial.println("Unable to place more ships! Grid may be full.");
            break; // vermeidet unendliche Schleife
        }

        placed += size;
    }
    shipcount += placed;
}



void drawX(int x, int y, uint16_t color) {
  tft.drawPixel(x + 0, y + 0, color);
  tft.drawPixel(x - 1, y - 1, color);
  tft.drawPixel(x + 1, y + 1, color);
  tft.drawPixel(x - 1, y + 1, color);
  tft.drawPixel(x + 1, y - 1, color);
}

void WINFUNC() {
  debug = true;
  renderBF();
  delay(8000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(7, 50);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(3);
  tft.print("GEWONNEN");
  playVictoryTone();
  tft.setTextSize(1);
  tft.setCursor(0, 80);
  tft.print("Score: "); tft.println(ammo);
  tft.print(ammo > highscore ? "Neuer Rekord!" : "Highscore: "); if (ammo > highscore) EEPROM.put(0, ammo); else tft.print(highscore); 

  delay(5000);
  tft.setTextWrap(true);
  tft.setCursor(0, 100);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("(Knopf von Joystick\ndruecken um das Spiel\nneuzustarten)");
  while(digitalRead(bPin)) {}
  delay(200);

  advertisement();
}

void LOSEFUNC() {
  debug = true;
  renderBF();
  delay(8000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(7, 50);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(3);
  tft.print("VERLOREN");
  playGameOverTone();

  delay(5000);
  tft.setTextWrap(true);
  tft.setCursor(0, 100);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("(Knopf von Joystick\ndruecken um das Spiel\nneuzustarten)");
  while(digitalRead(bPin)) {}
  delay(200);

  advertisement();
}

void advertisement() {
  const String gamenames[] = {"Zeitstopp", "Chronos", "Blitzbuzzer"}; // Nicht Fertig, muss erweitert werden! 
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 5);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setTextWrap(false);
  tft.print(" Gut gespielt, probiert\n doch auch einmal das Spiel\n "); tft.print(gamenames[random(0, (sizeof(gamenames) / sizeof(gamenames[0]) ))]);
  delay(10000);
  initiateReset();
}

void initiateReset() {
  wdt_enable(WDTO_15MS);  // Watchdog aktivieren mit 15ms Timeout
  while (true) {}         // Warten bis Reset passiert
}


void playVictoryTone() {
  int melody[] = { 1047, 1319, 1568, 2093 }; // C6–E6–G6–C7
  int duration[] = { 120, 120, 120, 400 };

  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, melody[i], duration[i]);
    delay(duration[i] * 1.4);
  }
  noTone(buzzerPin);
}


void playGameOverTone() {
  int melody[] = { 523, 494, 440, 330 }; // C5 - B4 - A4 - E4, absteigend und traurig
  int duration[] = { 200, 200, 400, 600 };

  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, melody[i], duration[i]);
    delay(duration[i] * 1.3);
  }
  noTone(buzzerPin);
}



































// Graphics Test
/*
void testlines(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, 0, x, tft.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, 0, tft.width()-1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, 0, x, tft.height()-1, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, 0, 0, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(0, tft.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(0, tft.height()-1, tft.width()-1, y, color);
    delay(0);
  }

  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, x, 0, color);
    delay(0);
  }
  for (int16_t y=0; y < tft.height(); y+=6) {
    tft.drawLine(tft.width()-1, tft.height()-1, 0, y, color);
    delay(0);
  }
}

void testdrawtext(char *text, uint16_t color) {
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y=0; y < tft.height(); y+=5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x=0; x < tft.width(); x+=5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

void testdrawrects(uint16_t color) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=0; x < tft.width(); x+=6) {
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color);
  }
}

void testfillrects(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t x=tft.width()-1; x > 6; x-=6) {
    tft.fillRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color1);
    tft.drawRect(tft.width()/2 -x/2, tft.height()/2 -x/2 , x, x, color2);
  }
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=radius; x < tft.width(); x+=radius*2) {
    for (int16_t y=radius; y < tft.height(); y+=radius*2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}

void testdrawcircles(uint8_t radius, uint16_t color) {
  for (int16_t x=0; x < tft.width()+radius; x+=radius*2) {
    for (int16_t y=0; y < tft.height()+radius; y+=radius*2) {
      tft.drawCircle(x, y, radius, color);
    }
  }
}

void testtriangles() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 0xF800;
  int t;
  int w = tft.width()/2;
  int x = tft.height()-1;
  int y = 0;
  int z = tft.width();
  for(t = 0 ; t <= 15; t++) {
    tft.drawTriangle(w, y, y, x, z, x, color);
    x-=4;
    y+=4;
    z-=4;
    color+=100;
  }
}

void testroundrects() {
  tft.fillScreen(ST77XX_BLACK);
  uint16_t color = 100;
  int i;
  int t;
  for(t = 0 ; t <= 4; t+=1) {
    int x = 0;
    int y = 0;
    int w = tft.width()-2;
    int h = tft.height()-2;
    for(i = 0 ; i <= 16; i+=1) {
      tft.drawRoundRect(x, y, w, h, 5, color);
      x+=2;
      y+=3;
      w-=4;
      h-=6;
      color+=1100;
    }
    color+=100;
  }
}

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(4);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.print(p, 6);
  tft.println(" Want pi?");
  tft.println(" ");
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(" seconds.");
}

void mediabuttons() {
  // play
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(25, 10, 78, 60, 8, ST77XX_WHITE);
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_RED);
  delay(500);
  // pause
  tft.fillRoundRect(25, 90, 78, 60, 8, ST77XX_WHITE);
  tft.fillRoundRect(39, 98, 20, 45, 5, ST77XX_GREEN);
  tft.fillRoundRect(69, 98, 20, 45, 5, ST77XX_GREEN);
  delay(500);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_BLUE);
  delay(50);
  // pause color
  tft.fillRoundRect(39, 98, 20, 45, 5, ST77XX_RED);
  tft.fillRoundRect(69, 98, 20, 45, 5, ST77XX_RED);
  // play color
  tft.fillTriangle(42, 20, 42, 60, 90, 40, ST77XX_GREEN);
}*/
