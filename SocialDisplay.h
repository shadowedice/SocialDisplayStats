#ifndef SocialDisplay_h
#define SocialDisplay_h

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>


//MQTT defines
#define MAX_SOCIAL 10

//MAX72XX defines
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   14  // or SCK
#define DATA_PIN  13  // or MOSI
#define CS_PIN    15  // or SS

#define CHAR_SPACING  1 // pixels between characters
#define CHAR_COLS 4     // should match the fixed width character columns
#define ANIMATION_FRAME_DELAY 30  // in milliseconds

#define NUM_DIGITS 5

//Update times
#define SOCIAL_CYCLE_TIME 10000

struct digitData
{
  uint8_t oldValue, newValue;   // ASCII value for the character
  uint8_t index;                // animation progression index
  uint32_t  timeLastFrame;      // time the last frame started animating
  uint8_t charCols;             // number of valid cols in the charMap
  uint8_t charMap[CHAR_COLS];   // character font bitmap
};

enum dStates
{
  ST_INIT = 0,
  ST_WAIT,
  ST_ANIM
};

struct SocialStruct
{
	uint8_t image[8]; // holds the bitmap for the icons
	uint32_t value;   // number of followers/subs/likes etc
};

class SocialDisplay
{
public:
  SocialDisplay();
  void setup();
  void Reconnect();
  void loop();

private:
  void MqttCallback(char* topic, byte* payload, unsigned int length);
  void updateDisplay(uint16_t numDigits, struct digitData *d);
  boolean displayValue();

private:
  WiFiClient espClient;
  PubSubClient client;

  //Estimated worst case for 10 social media.
  StaticJsonDocument<1923> jsonDoc;

  char* ssid;
  char* password;
  char* mqtt_server;
  char* subTopic;

  SocialStruct socialArray[MAX_SOCIAL];
  uint8_t numSocial;
  uint8_t currentSocial;
  uint32_t lastUpdate;

  MD_MAX72XX* mxLED;
  digitData digit[NUM_DIGITS];
  uint8_t displayState;
}; 

#endif
