#include "SocialDisplay.h"

SocialDisplay sDisplay;

void setup() {
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  delay(10);
  
  sDisplay.setup();
}

void loop() {

//Fix loop
sDisplay.loop();

}
