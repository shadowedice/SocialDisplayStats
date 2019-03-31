#include "SocialDisplay.h"
#include "FontData.h"

//Twitch 63,33,225,77,33,45,33,63
//Twitter 143,222,252,124,126,63,30,4
//Instagram 126,129,153,165,165,153,131,126

SocialDisplay::SocialDisplay()
{
	ssid = "YOUR WIFI SSID";
	password = "YOUR WIFI PASSWORD";
	mqtt_server = "broker.mqtt-dashboard.com"; //recommend changing
  subTopic = "USER TOPIC HERE";

  numSocial = 0;
  currentSocial = 0;
  lastUpdate = 0;
  
  mxLED = NULL;
  displayState = ST_INIT;
}

void SocialDisplay::setup()
{
  //Setup Wifi
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  //Setup Mqtt
  client.setClient(espClient);
  client.setServer(mqtt_server, 1883);
  client.setCallback([this] (char* topic, byte* payload, unsigned int length) {this->MqttCallback(topic, payload, length);});

  //Setup MX LED display
  mxLED = new MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
  mxLED->begin();
  mxLED->setFont(fourWidthFont);
  mxLED->control(MD_MAX72XX::INTENSITY, 1);
}

void SocialDisplay::Reconnect()
{
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "SocialDisplay-" + String(subTopic);
    // Attempt to connect
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      //subscribe
      client.subscribe(subTopic);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
    }
}

void SocialDisplay::loop()
{
  //mqtt client loop
	if(!client.connected())
	{
		Reconnect();
	}
	client.loop();

  if(millis() - lastUpdate > SOCIAL_CYCLE_TIME)
  {
    if(currentSocial < numSocial - 1 && displayState == ST_WAIT){
      currentSocial++;
    }
    else{
      currentSocial = 0;
    }
    lastUpdate = millis();
  }
  displayValue();
}

void SocialDisplay::MqttCallback(char* topic, byte* payload, unsigned int length)
{
  DeserializationError error = deserializeJson(jsonDoc, (char*)payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  numSocial = jsonDoc["count"];
  for(uint8_t i=0; i < numSocial; i++)
  {
    for(uint8_t j=0; j < 8; j++)
    {
      socialArray[i].image[j] = jsonDoc["data"][i]["image"][j];
    }
    socialArray[i].value = jsonDoc["data"][i]["value"];
  }
}

void SocialDisplay::updateDisplay(uint16_t numDigits, struct digitData *d)
{
  uint8_t   curCol = 0;

  mxLED->control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mxLED->clear();

  for (int8_t i = numDigits - 1; i >= 0; i--)
  {
    for (int8_t j = d[i].charCols - 1; j >= 0; j--)
    {
      mxLED->setColumn(curCol++, d[i].charMap[j]);
    }
    curCol += CHAR_SPACING;
  }

  //Remove last space
  curCol--;
  for(int8_t i = 7; i >= 0; i--)
  {
    mxLED->setColumn(curCol++, socialArray[currentSocial].image[i]);
  }

  mxLED->control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

boolean SocialDisplay::displayValue()
// Display the required value on the LED matrix and return true if an animation is current
// Finite state machine will ignore new values while animations are underway.
// Needs to be called repeatedly to ensure animations are completed smoothly.
{
  uint16_t value = socialArray[currentSocial].value;
  boolean foundSig = false;

  // finite state machine to control what we do
  switch(displayState)
  {
    case ST_INIT: // Initialize the display - done once only on first call
      for (int8_t i = NUM_DIGITS - 1; i >= 0; i--)
      {
        // add initial digit "map", dont add leading 0s
        digit[i].oldValue = '0' + value % 10;
        if (value != 0 || (value == 0 && i == NUM_DIGITS-1))
        {
          digit[i].charCols = mxLED->getChar(digit[i].oldValue, CHAR_COLS, digit[i].charMap);
        }
        else
        {
          digit[i].charCols = CHAR_COLS;
          memset(digit[i].charMap, 0, sizeof(digit[i].charMap));
        }
        value = value / 10;
      }
      
      updateDisplay(NUM_DIGITS, digit);

      // Now we wait for a change
      displayState = ST_WAIT;
      break;

    case ST_WAIT: // not animating - save new value digits and check if we need to animate
      for (int8_t i = NUM_DIGITS - 1; i >= 0; i--)
      {
        // separate digits
        digit[i].newValue = '0' + value % 10;
        value = value / 10;

        if (digit[i].newValue != digit[i].oldValue)
        {
          // a change has been found - we will be animating something
          displayState = ST_ANIM;
          // initialize animation parameters for this digit
          digit[i].index = 0;
          digit[i].timeLastFrame = 0;
        }
      }

      if (displayState == ST_WAIT) // no changes - keep waiting
        break;
      // else fall through as we need to animate from now

    case ST_ANIM: // currently animating a change
      // work out the new intermediate bitmap for each character
      // 1. Get the 'new' character bitmap into temp buffer
      // 2. Shift this buffer down or up by current index amount
      // 3. Shift the current character by one pixel up or down
      // 4. Combine the new partial character and the existing character to produce a frame
      for (int8_t i = 0; i < NUM_DIGITS; i++)
      {
        if ((digit[i].newValue != digit[i].oldValue) && // values are different
           (millis() - digit[i].timeLastFrame >= ANIMATION_FRAME_DELAY)) // timer has expired
        {
          uint8_t newChar[CHAR_COLS] = { 0 };
          if(digit[i].newValue != '0' || foundSig)
          {
            mxLED->getChar(digit[i].newValue, CHAR_COLS, newChar);
            foundSig = true;
          }
          
          if (((digit[i].newValue > digit[i].oldValue) ||    // incrementing
             (digit[i].oldValue == '9' && digit[i].newValue == '0')) && // wrapping around on increase
             !(digit[i].oldValue == '0' && digit[i].newValue == '9'))   // not wrapping around on decrease
          {
            // scroll down
            for (uint8_t j = 0; j < digit[i].charCols; j++)
            {
              newChar[j] = newChar[j] >> (COL_SIZE - 1 - digit[i].index);
              digit[i].charMap[j] = digit[i].charMap[j] << 1;
              digit[i].charMap[j] |= newChar[j];
            }
          }
          else
          {
            // scroll up
            for (uint8_t j = 0; j < digit[i].charCols; j++)
            {
              newChar[j] = newChar[j] << (COL_SIZE - 1 - digit[i].index);
              digit[i].charMap[j] = digit[i].charMap[j] >> 1;
              digit[i].charMap[j] |= newChar[j];
            }
          }

          // set new parameters for next animation and check if we are done
          digit[i].index++;
          digit[i].timeLastFrame = millis();
          if (digit[i].index >= COL_SIZE )
            digit[i].oldValue = digit[i].newValue;  // done animating
        }
      }

      updateDisplay(NUM_DIGITS, digit);  // show new display

      // are we done animating?
      {
        boolean allDone = true;

        for (uint8_t i = 0; allDone && (i < NUM_DIGITS); i++)
        {
          allDone = allDone && (digit[i].oldValue == digit[i].newValue);
        }

        if (allDone) displayState = ST_WAIT;
      }
      break;

    default:
      displayState = 0;
  }

  return(displayState == ST_WAIT);   // animation has ended
}
