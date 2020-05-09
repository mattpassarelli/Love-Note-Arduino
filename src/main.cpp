//
// Love Box
//
#include "SH1106Wire.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include "secrets.h"
#include "FastLED.h"

#define NUM_LEDS 18

CRGB leds[NUM_LEDS];
SH1106Wire oled(0x3c, D2, D1);
const int SCREEN_WIDTH = 128;
bool isConnectedToWifi = false;
bool wasMessageRead = true;
char idSaved = -1;
String line;
String modeSelector;
const char *ssid = _ssid;
const char *password = _password;
const String url = _url;
WiFiClientSecure client;

int fadeAmount = 5;
int brightness = 0;

const int readMessagePin = D5;
const int LEDPin = D7;

// Defined in "CACert" tab.
extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;

void wifiConnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    oled.drawStringMaxWidth(0, 0, SCREEN_WIDTH, "Connecting to WiFi network...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
  }
}

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
  wasMessageRead = false;
}

void waitForMessage()
{
  Serial.println("in message function");
  const int httpsPort = 443;
  const char *host = "love-note-backend.herokuapp.com";
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

  String line = client.readString();
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  Serial.println("Drawing new message");
  drawMessage(line);
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

  // EEPROM.begin(512);
  // idSaved = EEPROM.get(142, idSaved);
  // wasMessageRead = EEPROM.get(144, wasMessageRead);
}

void fadeLEDS()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i].setRGB(255, 0, 0); // setRGB functions works by setting
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
      wasMessageRead = true;
    }
    else
    {
      //Light up LEDS until Message is read
      fadeLEDS();
    }
  }
}
