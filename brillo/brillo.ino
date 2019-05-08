#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>

//define pins
Elegoo_TFTLCD tft(A3, A2, A1, A0, A4);
TouchScreen ts = TouchScreen(8, A3, A2, 9, 300);

//dimensions
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920

//colors
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x03E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

/*  Generate hex gradients, convert them to rgb565 codes
    http://www.perbang.dk/rgbgradient/
    http://www.rinkydinkelectronics.com/calc_rgb565.php */

//  ugly green -> red gradient
//uint16_t gradient[8] = {0xC986,0xBA26,0xAAA6,0x9B26,0x8BA6,0x7446,0x64C6,0x5546};
//  "Moderate Raspberry" -> "Very Dark Purple"
uint16_t gradient[8] = {0xA1CB, 0x91AC, 0x798C, 0x694B, 0x592A, 0x40E8, 0x28A6, 0x1863};

//  initialize buttons
Elegoo_GFX_Button listButtons[6];

//  make these the same size, original list is kept. Title should be =< 16 chars
const int LIST_SIZE = 12;
//char listOriginal[8][32] = {"Test List","One", "Two", "Three", "Four", "Five", "01234567890123456789012345678901", "%END"};
char listOriginal[LIST_SIZE][32] = {"Jasmine - Morning Routine", "Out of bed :(", "Get clean shirt/pants/underwear", "Change clothes", "Eat Breakfast", "Brush Teeth", "Take Medicine", "Brush Hair", "Put on socks", "Check backpack - homework?", "Put on shoes", "Get out the door"};
char listButtonLabels[LIST_SIZE][32];

bool timeout;
uint8_t pressed = 1;
uint8_t pressedLast = 1;
uint8_t pressDuration;
uint8_t discreteDuration;
uint8_t prevDiscreteDuration;

#define GRADIENT_SIZE (sizeof(gradient)/sizeof(gradient[0]))
//#define LIST_SIZE (sizeof(listOriginal)/sizeof(listOriginal[0]))

#define BUTTON_X 120  //  why does this need such a big offset?
#define BUTTON_Y 31   //  why does this need such a big offset? rotation?
#define BUTTON_W 232
#define BUTTON_H 46
#define BUTTON_SPACING_X 4
#define BUTTON_SPACING_Y 4
#define BUTTON_TEXTSIZE 1

/******
  Functions
******/

//  define a button
void initButton(uint8_t row, uint16_t outlineColor, uint16_t fillColor, uint16_t textColor, char text[32]) {
  //  draw buttons black when at the end of the list
  if (listButtonLabels[row][0] == NULL) {
    outlineColor = BLACK;
    fillColor = BLACK;
    textColor = BLACK;
  }
  listButtons[row].initButton(&tft, BUTTON_X, BUTTON_Y + row * (BUTTON_H + BUTTON_SPACING_Y),
                              //                              BUTTON_W, BUTTON_H, outlineColor, fillColor, textColor, text, BUTTON_TEXTSIZE);
                              BUTTON_W, BUTTON_H, outlineColor, fillColor, textColor, " ", BUTTON_TEXTSIZE);
}

void drawText(uint8_t button = 0, uint16_t color = WHITE) {
  // set text to size 1 if length > 16, re-center
  if (strlen(listButtonLabels[button]) > 16) {
    if (button == 0) {
      tft.setCursor(21, 24);
    } else {
      tft.setCursor(21, button * (BUTTON_SPACING_Y + BUTTON_H) + 28);
    }
    tft.setTextSize(1);
    tft.setTextColor(color);
  } else {
    tft.setCursor(21, button * (BUTTON_SPACING_Y + BUTTON_H) + 23);
    tft.setTextSize(2);
  }
  tft.setTextColor(color);
  tft.print(listButtonLabels[button]);
}

void progressBar() {
  //check number of remaining items
  uint8_t progress = 0;
  for (uint8_t i = 0; i < LIST_SIZE; i++) {
    if (listButtonLabels[i][0] == NULL) {
      progress++;
    }
  }
  // compare against LIST_SIZE to determine progress
  
  if (progress == 0) {
    tft.fillRoundRect(8, 8, 224, 38, 8, RED);
    tft.drawRoundRect(8, 8, 224, 38, 8, WHITE);
    drawText();
  } else {
    tft.fillRoundRect(8, 8, 224, 38, 8, RED);
    tft.fillRoundRect(8, 8, progress*(244/LIST_SIZE), 38, 8, GREEN);
    tft.drawRoundRect(8, 8, 224, 38, 8, WHITE);
    drawText();
  }
}

void refreshButtons(uint8_t button = 1, bool reinitialize = 0, bool multi = 0) {
  // refresh the button and all buttons after
  if (multi) {
    for (uint8_t i = button; i < 6; i++) {
      if (reinitialize) {
        initButton(i, WHITE, gradient[0], WHITE, " ");
      }
      if (i > 0) {
        listButtons[i].drawButton();
      }
      drawText(i);
    }
  } else {
    // refresh a single button
    if (reinitialize) {
      initButton(button, WHITE, gradient[0], WHITE, " ");
    }
    if (button > 0) {
      listButtons[button].drawButton();
    }
    drawText(button);
  }
}

void removeItem(char list[LIST_SIZE][32], int n) {
  for (int i = n; i < LIST_SIZE; i++) {
    if (i == LIST_SIZE - 1) {
      list[i][0] = NULL;
    } else {
      for (int j = 0; j < 32; j++) {
        list[i][j] = list[i + 1][j];
      }
    }
  }
  progressBar();
}



/******
  Initial Setup
******/

void setup(void) {
  Serial.begin(115200);
  tft.reset();
  tft.begin(0x9341);
  tft.setRotation(2);
  tft.fillScreen(BLACK);

  // copy list from original to initialize it
  for (uint8_t i = 0; i < LIST_SIZE; i++) {
    for (uint8_t j = 0; j < 32; j++) {
      listButtonLabels[i][j] = listOriginal[i][j];
    }
  }
  progressBar();
  refreshButtons(0, 1, 1);
}


/******
  Continuous loop
******/

void loop(void) {
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);

  if (listButtonLabels[1][0] == NULL) {
    //all done - victory screen & reset
    tft.fillScreen(WHITE);
    setup();
  }

  if (p.z > 100 && p.z < 1000) {
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = (tft.height() - map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));

    if (pressed > 0  && prevDiscreteDuration != discreteDuration) {
      refreshButtons(pressed);
    }
  } else {

    if (listButtons[pressed].justReleased()) {
      refreshButtons(pressed, 1);
      pressed = 0;
    } else {
      pressDuration = 0;
    }
  }

  prevDiscreteDuration = discreteDuration;
  discreteDuration = map(pressDuration, 0, 8, 0, GRADIENT_SIZE);

  // if held for the specified duration
  if (discreteDuration > 8) {
    // item falls off list
    removeItem(listButtonLabels, pressed);
    refreshButtons(pressed, 1, 1);
    discreteDuration = 0;
    pressDuration = 0;
  } else if (discreteDuration > 1) {
    // otherwise increase its position on the gradient
    initButton(pressed, WHITE, gradient[discreteDuration - 1], WHITE, " ");
  }

  // determine whether a button is pressed or not using contains
  for (uint8_t row = 0; row < 6; row++) {
    if (listButtons[row].contains(p.x, p.y)) {
      listButtons[row].press(true);
      if (pressed != row) {
        pressed = row;
        pressDuration = 0;
      } else {
        pressDuration++;
      }
    } else {
      listButtons[row].press(false);
    }
  }

  // global delay to reduce input/graphical jitter
  delay(40);
}
