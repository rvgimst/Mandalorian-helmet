#include "DFPlayerMini_Fast.h"
#include <FastLED.h>

#define NUM_LEDS 44 
#define DATA_PIN A0
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define BRIGHTNESS 70
#define LED_INTRO_TIMEOUT     2000
#define LED_SOUNDBITE_TIMEOUT 5700
#define LED_ONELINER_TIMEOUT  3000
#define LED_THEME_TIMEOUT   198000 // (3*60 + 18)*1000

#define BUTTON_PIN 2
#define TOUCH_PIN 12 
#define LED_BUILTIN 13

// fixed folders with specific mp3 files
#define FOLDER_INTROS     1
#define FOLDER_SOUNDBITES 2
#define FOLDER_ONELINERS  3
#define FOLDER_THEME      4

#define VOLUME_DEFAULT 20
#define VOLUME_MUSIC 26
#if !defined(UBRR1H)
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX
#endif

// touch input struct
struct Touch { 
	 byte wasPressed = LOW; 
	 byte isPressed = LOW; 
}; 

// states of the Mando helmet
#define STATE_NONE  0
#define STATE_SOUND 1
#define STATE_LAMP  2

// Global variables
CRGB leds[NUM_LEDS];
uint8_t paletteIndex = 0;
unsigned long ledTimeout[5] = {0, LED_INTRO_TIMEOUT, LED_SOUNDBITE_TIMEOUT, LED_ONELINER_TIMEOUT, LED_THEME_TIMEOUT};

bool once = true;
byte lastButtonState; // init in Setup()
unsigned long debounceDuration = 50; // millis
unsigned long lastTimeButtonStateChanged = 0;
enum { btnNone, btnSingleClick, btnDoubleClick, btnLongPress };

DFPlayerMini_Fast myMP3; // our mp3 player
byte folderCnt[5] = {0, 5, 8, 108, 1}; // hard coded for now
String folderName[5] = {"root", "intros", "soundbites", "oneliners", "theme"};
byte oneLinerCycle = 5; // after this number of one-liners we play a soundbite
int lastTypePlaying = 0; // FOLDER_INTRO | FOLDER_SOUNDBITES | FOLDER_ONELINERS
unsigned long lastTimePlayStart = 0; // time when last sound was started - for LED duration

Touch touch;

byte mandoState = STATE_NONE;

// Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.
DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160};
  
CRGBPalette16 myPal = Sunset_Real_gp;

void setup() {
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOUCH_PIN, INPUT);
  lastButtonState = digitalRead(BUTTON_PIN);

  randomSeed(analogRead(A1));

#if !defined(UBRR1H)
  mySerial.begin(9600);
  myMP3.begin(mySerial/*, true*/);
#else
  Serial1.begin(9600);
  myMP3.begin(Serial1, true);
#endif

  // Init LED strip
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS); // set master brightness control
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
  FastLED.clear();
  
  Serial.print("Setting volume to ");
  Serial.println(VOLUME_DEFAULT);
  myMP3.volume(VOLUME_DEFAULT);

//  Serial.println((String)myMP3.numTracksInFolder(2)+" tracks found in folder 2");
  delay(2000);
}

void playFromFolder(int folder) {
  mandoState = STATE_SOUND;
  int playTrack = random(folderCnt[folder])+1;
  lastTypePlaying = folder;
  lastTimePlayStart = millis();
  // Serial.println((String)myMP3.numTracksInFolder(folder)+" tracks found in folder "+folder+" ("+folderName[folder]+")");
  Serial.println((String)"Playing track "+playTrack+" from folder "+folder+" ("+folderName[folder]+")");
  Serial.println((String)"LEDtimeout= "+ledTimeout[folder]+"ms");
  myMP3.playFolder(folder, playTrack);
}
  
void playNextSound() {
  static byte oneLinersPlayed = 0;
  
  // for (int i=0; i<=5; i++) {
  //   Serial.println((String)myMP3.numTracksInFolder(i)+" tracks found in folder "+i); //+" ("+folderName[i]+")");
  //   delay(50);
  // }

  if (oneLinersPlayed == oneLinerCycle) {
    myMP3.volume(VOLUME_MUSIC);
    playFromFolder(FOLDER_SOUNDBITES);
    oneLinersPlayed = 0;
  } else {
    myMP3.volume(VOLUME_DEFAULT);
    playFromFolder(FOLDER_ONELINERS);
    oneLinersPlayed++;
  }
}

void showLEDEffect() {
  // create a sine wave with period of 2 sec (30bpm) to change brightness of the strip
  // beatsin8(bpm, minvalue, maxvalue, phase offset, timebase
  uint8_t sinBeat = beatsin8(41, 30, 255, 0, 0);
    // Serial.println((String)"LEDs: time passed since PlayStart= "+timePassed);

    // CRGB ColorFromPalette( const CRGBPalette16& pal,
    //                     uint8_t index,
    //                     uint8_t brightness=255,
    //                     TBlendType blendType=LINE
  EVERY_N_MILLISECONDS(20) {
    for (int i=0; i<NUM_LEDS; i++) {
      leds[i] = ColorFromPalette(myPal, paletteIndex, sinBeat, LINEARBLEND);
    }
    paletteIndex++; // loops over 255 for the type is uint8_t
  }

    // // MOVING PALETTE EFFECT
    // //fill_palette(led array, nLEDS, startIndex, indexDelta, palette,
    // //             brightness, blendType:LINEARBLEND|NOBLEND)
    // fill_palette(leds, NUM_LEDS, paletteIndex, 1 /*255/NUM_LEDS*/, myPal, BRIGHTNESS, LINEARBLEND);
  
    // EVERY_N_MILLISECONDS(20) {
    //   paletteIndex++;
    // }
}

void showLampEffect() {
    // CRGB ColorFromPalette( const CRGBPalette16& pal,
    //                     uint8_t index,
    //                     uint8_t brightness=255,
    //                     TBlendType blendType=LINE
  EVERY_N_MILLISECONDS(200) {
    for (int i=0; i<NUM_LEDS; i++) {
      leds[i] = ColorFromPalette(myPal, paletteIndex, BRIGHTNESS, LINEARBLEND);
    }
    paletteIndex++; // loops over 255 for the type is uint8_t
  }
}

int chkTouch() {
  int retVal = btnNone;
  const  unsigned long LongPressTimeout = 1000;
  static unsigned long msecLast = 0;
         unsigned long msec = millis ();

  //TODO: double touch
  touch.isPressed = (digitalRead(TOUCH_PIN) == HIGH);

  if (touch.wasPressed && !touch.isPressed) { // change to release
    Serial.println("Touch released");
    if (msec - msecLast < LongPressTimeout) { // click only when short press
      retVal = btnSingleClick;
    }
  }
  else if (!touch.wasPressed && touch.isPressed) { // change to press
    Serial.println("Touch pressed");
    msecLast = msec;
  }
  else if (touch.wasPressed && touch.isPressed) { // holding
    if (msecLast != 0 && msec - msecLast >= LongPressTimeout) { // long press
      retVal = btnLongPress;
      msecLast = 0;
    }
  }
  touch.wasPressed = touch.isPressed;
  
  return retVal;
}

int chkButton() {
  const  unsigned long ButTimeout  = 250;
  static unsigned long msecLast;
         unsigned long msec = millis ();

  if (msecLast && (msec - msecLast) > ButTimeout)  {
    msecLast = 0;
    return btnSingleClick;
  }

  if (millis() - lastTimeButtonStateChanged > debounceDuration) {
    byte buttonState = digitalRead(BUTTON_PIN);
    if (lastButtonState != buttonState) {
      lastTimeButtonStateChanged = millis();
      lastButtonState = buttonState;

      if (LOW == buttonState)  {   // press
        if (msecLast)  { // 2nd press
          msecLast = 0;
          return btnDoubleClick;
        }
        else
          msecLast = (0 == msec ? 1 : msec);
      }
    }
  }

  return btnNone;

  // if (millis() - lastTimeButtonStateChanged > debounceDuration) {
  //   byte buttonState = digitalRead(BUTTON_PIN);
    
  //   if (buttonState != lastButtonState) {
  //     Serial.println("Button state changed");
  //     lastTimeButtonStateChanged = millis();
  //     lastButtonState = buttonState;
  //     if (buttonState == LOW) {
  //       // Button is pushed down. We play another random sound
  //       Serial.println("Button pushed");
  //       playNextSound();
  //     }
  //   }
  // }

}

void loop() {
  static unsigned long timePassed = 0;

  // play random track at startup
  if (once) {
    Serial.println("Playing startup sound");
    myMP3.volume(VOLUME_DEFAULT);
    playFromFolder(FOLDER_INTROS);
    once = false;
  }
  
  // BUTTON TRIGGER
  switch(chkButton()) {
    case btnSingleClick:
      Serial.println("SINGLE button click (Push)");
      myMP3.volume(VOLUME_DEFAULT);
      playNextSound();
      break;
    case btnDoubleClick:
      Serial.println("DOUBLE button click (Push)");
      myMP3.volume(20);
      playFromFolder(FOLDER_THEME);    
      break;
  }

  // TOUCH TRIGGER
  switch(chkTouch()) {
    case btnSingleClick:
      Serial.println("SINGLE button click (Push)");
      myMP3.volume(VOLUME_DEFAULT);
      playNextSound();
      break;
    case btnLongPress:
      Serial.println("LONG button Press");
      // toggle lamp mode on/off
      mandoState = STATE_LAMP == mandoState ? STATE_NONE : STATE_LAMP;
      break;
  }

  switch(mandoState) {
    case STATE_SOUND:
      timePassed = millis() - lastTimePlayStart;
      if (timePassed < ledTimeout[lastTypePlaying]) { // estimate while sound is playing
        showLEDEffect();
      }
      else {
        mandoState = STATE_NONE;
      }
      break;
    case STATE_LAMP:
      // LED Lamp effect, no sound
      showLampEffect();
      
      // FOR TESTING: Or play theme with LED effects
      // myMP3.volume(20);
      // playFromFolder(FOLDER_THEME);      
      break;
    default:
      break;
  }

  if (mandoState == STATE_NONE) {
    EVERY_N_MILLISECONDS(5) {
      fadeToBlackBy(leds, NUM_LEDS, 1);
    }
    //FastLED.clear();
  }

  FastLED.show();
}
