//
// Love Box
//
#include "SH1106Wire.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "FastLED.h"
#include "CACert.h"

#define NUM_LEDS 18
#define ID_ADDR 142
#define READ_ADDR 144

//Object definitions
CRGB leds[NUM_LEDS];
SH1106Wire oled(0x3c, D2, D1);
WiFiClientSecure client;
const size_t capacity = JSON_OBJECT_SIZE(2) + 100;
DynamicJsonDocument doc(capacity);

//Consts
const int readMessagePin = D5;
const int LEDPin = D7;
const int SCREEN_WIDTH = 128;
const bool isMatt = false; //true for Matt, false for Rayanne's box
const char *ssid = isMatt ? _mattssid : _rayannessid;
const char *password = isMatt ? _mattpassword : _rayannepassword;
const String url = isMatt ? _mattURL : _rayanneURL;
const char *host = "love-note-backend.herokuapp.com";
const int redLED = isMatt ? 22 : 230;
const int greenLED = isMatt ? 227 : 108;
const int blueLED = isMatt ? 0 : 173;

//Other variables
int fadeAmount = 5;
int brightness = 0;
bool isConnectedToWifi = false;
bool wasMessageRead = true;
char idSaved = 0;
String line;
String modeSelector;

// Defined in "CACert" tab.
extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;

/**
 *  Connect to WiFi networked defined in secrets.h
 **/
void wifiConnect()
{
  Serial.print("WiFi status is: ");
  Serial.println(WiFi.status());
  
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(isMatt ? _mattssid : _rayannessid);

    oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Connecting to WiFi network...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
  }
  else {
    Serial.println("WiFi connected");
  }
}

/**
 *  Display the message on screen
 **/
void drawMessage(const String &message)
{
  oled.clear();

  //  if (modeSelector[0] == 't')
  //  {
  oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, message.c_str());
  //  }
  //  else
  //  {
  //    for (int i = 0; i <= message.length(); i++)
  //    {
  //      int x = i % 129;
  //      int y = i / 129;
  //
  //      if (message[i] == '1')
  //      {
  //        oled.draw1x2Glyph(x, y, 1);
  //      }
  //    }
  //  }
  oled.display();
}

/**
 * Parse the JSON we get from my server
 **/
void parseJSON(String line)
{
  const char *json = line.c_str();

  deserializeJson(doc, json);

  // Parameters
  int id = doc["id"];
  const char *message = doc["message"];

  //  String str = String(*message);

  Serial.print("id: ");
  Serial.println(id);
  Serial.print("Message:");
  Serial.println(message);
  //  Serial.println(str);

  //Check ID against what is stored in EEPROM, so we don't grab the message we just read
  //if matches and is above 0 (default is 0)
  //do nothing, keep checking
  //else it doesn't
  //store new ID in ROM
  //Draw message

  Serial.println("Comparing Message ID to ID in ROM");
  Serial.print("IDs are ");
  Serial.print(id);
  Serial.print(" and ");
  Serial.println(EEPROM.read(ID_ADDR));

  if (id == EEPROM.read(ID_ADDR))
  {
    Serial.println("IDs matched. Same message as before. Not displaying message");
    //Also turn the screen off now until we get a new message
    Serial.println("Turning off display");
    oled.clear();
    oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Turning off screen until new message arrives");
    oled.display();
    
    delay(15000);
    oled.displayOff();
    //Wait a full minute to ping again to save bandwidth to my server
    delay(45000);
  }
  else
  {

    Serial.println("IDs are different. New message");
    Serial.println("Saving new ID");
    EEPROM.write(ID_ADDR, id);

    Serial.println("Setting wasMessageRead to false in memory");
    EEPROM.write(READ_ADDR, false);
    wasMessageRead = false;

    EEPROM.commit();
    Serial.print("ID Saved: ");
    Serial.println(EEPROM.read(ID_ADDR));

    Serial.println("Drawing Message");
    oled.displayOn();
    drawMessage(message);
  }
}

/**
 * Is Run every minute
 * Pings my backend server for the newest message as a 
 * */
void waitForMessage()
{
  Serial.println("in message function");
  const int httpsPort = 443;

  Serial.print("Looking to host ");
  Serial.println(host);

  if (!client.connect(host, httpsPort))
  {
    Serial.println("Connection to host failed");
  }
  else
  {
    Serial.println("Server Certificated verified");
  }

  Serial.println("Requesting URL: " + url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("Request Sent");

  while (client.connected())
  {
    String temp = client.readStringUntil('\n');
    if (temp == "\r")
    {
      Serial.println("Headers Received");
      break;
    }
  }

  String line = client.readStringUntil('\r');
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  Serial.println("Parsing JSON");
  parseJSON(line);
}

void setup()
{
  Serial.begin(9600);
  oled.init();
  oled.flipScreenVertically();
  oled.setColor(WHITE);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_16);
  oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Hello :)");
  delay(1000);

  //Connect to WiFi
  wifiConnect();

  oled.clear();
  oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Setting up...");
  oled.display();

  // Synchronize time using SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  Serial.print("Setting time using SNTP");
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));

  // Load root certificate in DER format into WiFiClientSecure object
  bool res = client.setCACert_P(caCert, caCertLen);
  if (!res)
  {
    Serial.println("Failed to load root CA certificate!");
    while (true)
    {
      yield();
    }
  }

  //Register Pin for button to mark message as read
  pinMode(readMessagePin, INPUT_PULLUP);

  //LED Setup
  FastLED.addLeds<NEOPIXEL, LEDPin>(leds, NUM_LEDS);
  //Set LEDS to off
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();

  //EEPROM Stuff
  Serial.println("Starting EEPROM stuff");
  EEPROM.begin(512);
  idSaved = EEPROM.read(ID_ADDR);
  wasMessageRead = EEPROM.read(READ_ADDR);
  Serial.println("Retreived ID and messageRead state");
  Serial.println(idSaved);
  Serial.println(wasMessageRead);
}

/**
 *  Fades the LEDS in and out when you have an unread message
 **/
void fadeLEDS()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i].setRGB(redLED, greenLED, blueLED); // setRGB functions works by setting
                               // (RED value 0-255, GREEN value 0-255, BLUE value 0-255)
                               // RED = setRGB(255,0,0)
                               // GREEN = setRGB(0,255,0)
    leds[i].fadeLightBy(brightness);
  }
  FastLED.show();
  brightness = brightness + fadeAmount;
  // reverse the direction of the fading at the ends of the fade:
  if (brightness == 0 || brightness == 255)
  {
    fadeAmount = -fadeAmount;
  }
  delay(40); // This delay sets speed of the fade. I usually do from 5-75 but you can always go higher.
}

void loop()
{
  if (wasMessageRead)
  {
    Serial.println("Waiting for message");
    oled.clear();
    oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Waiting for a message");
    oled.display();

    waitForMessage();
  }
  else
  {
    //If the button is pressed, and the message is showing, then reset waiting for one
    //Why it's reversed and I'm waiting for LOW, I Have no idea
    if (digitalRead(readMessagePin) == LOW)
    {
      Serial.println("Setting wasMessageRead to true. Beginning to wait for a message again...");
      //Set LEDS to off
      for (int i = 0; i < NUM_LEDS; i++)
      {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      Serial.println("Setting wasMessageRead to true in memory");
      EEPROM.write(READ_ADDR, true);
      wasMessageRead = true;
      EEPROM.commit();
    }
    else
    {
      //Light up LEDS until Message is read
      fadeLEDS();
    }
  }
}