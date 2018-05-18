/*
 * This is the code for the project called
 * Google Assistant Controlled Scrolling Display
 * which is edited by Sachin Soni for his YouTube channel "techiesms".
 *      
 *Tutorial video of this project is available on the youtube channel whose link 
 *is attached here.
 *    
 *    
 *explore|learn|share
 *    techiesms
 *    
 *YouTube Channel :- https://www.youtube.com/user/sachinsms1990
 *
 *Website :- http://www.techiesms.com
 *      
 */
/************************* Necessary Libraries *********************************/
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define SCROLL_DELAY 75

/************************* Variables *********************************/
char* str;
String payload;
uint32_t present;
bool first_time;
uint16_t  scrollDelay;  // in milliseconds
#define  CHAR_SPACING  1 // pixels between characters

// Global message buffers shared by Serial and Scrolling functions
#define BUF_SIZE  75
char curMessage[BUF_SIZE];
char newMessage[BUF_SIZE];
bool newMessageAvailable = false;

ESP8266WiFiMulti WiFiMulti;

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define	MAX_DEVICES	8

/************************* Matrix Display Pins *********************************/
#define  CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

/************************* Adafruit.io Setup *********************************/
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "USERNAME"
#define AIO_KEY         "KEY"


/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe message = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/message");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

// SPI hardware interface
MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);
// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);





uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t)
// Callback function for data that is required for scrolling into the display
{
  static char		*p = curMessage;
  static uint8_t	state = 0;
  static uint8_t	curLen, showLen;
  static uint8_t	cBuf[8];
  uint8_t colData;

  // finite state machine to control what we do on the callback
  switch (state)
  {
    case 0:	// Load the next character from the font table
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state++;

      // if we reached end of message, reset the message pointer
      if (*p == '\0')
      {
        p = curMessage;			// reset the pointer to start of message
        if (newMessageAvailable)	// there is a new message waiting
        {
          strcpy(curMessage, str);	// copy it in
          newMessageAvailable = false;
        }
      }
    // !! deliberately fall through to next state to start displaying

    case 1:	// display the next part of the character
      colData = cBuf[curLen++];
      if (curLen == showLen)
      {
        showLen = CHAR_SPACING;
        curLen = 0;
        state = 2;
      }
      break;

    case 2:	// display inter-character spacing (blank column)
      colData = 0;
      curLen++;
      if (curLen == showLen)
        state = 0;
      break;

    default:
      state = 0;
  }

  return (colData);
}

void scrollText(void)
{
  static uint32_t	prevTime = 0;

  // Is it time to scroll the text?
  if (millis() - prevTime >= scrollDelay)
  {
    mx.transform(MD_MAX72XX::TSL);	// scroll along - the callback will load all the data
    prevTime = millis();			// starting point for next time
  }
}
void  no_connection(void)
{
  newMessageAvailable = 1;
  strcpy(curMessage, "No Internet! ");
  scrollText();


}
void setup()
{
  mx.begin();
  mx.setShiftDataInCallback(scrollDataSource);

  scrollDelay = SCROLL_DELAY;
  strcpy(curMessage, "Hello! ");
  newMessage[0] = '\0';

  Serial.begin(57600);
  Serial.print("\n[MD_MAX72XX Message Display]\nType a message for the scrolling display\nEnd message line with a newline");

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("SSID1", "PASS1");
  WiFiMulti.addAP("SSID2", "PASS2");
  WiFiMulti.addAP("SSID3", "PASS3");
  Serial.println("Connecting");
  newMessageAvailable = 1;
  present = millis();
  first_time = 1;
    // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&message);
 str = "   Ask Google assistant to change the msg!!!   ";
}

void loop()
{
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here


  
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(1))) {
    if (subscription == &message) {
      payload ="";
      Serial.print(F("Got: "));
      Serial.println((char *)message.lastread);
      str = (char*)message.lastread;
      payload = (String) str;
      payload += "       ";
      str = &payload[0];      
      newMessageAvailable = 1;
    }
  }

  scrollText();
}
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

