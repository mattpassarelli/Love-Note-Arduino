//
// Love Box
//
#include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include "secrets.h"

U8X8_SH1106_128X64_NONAME_HW_I2C oled(/* reset=*/U8X8_PIN_NONE);
bool isConnectedToWifi = false;
bool wasMessageRead = false;
char idSaved = -1;
String line;
String modeSelector;
const char *ssid = _ssid;
const char *password = _password;
const String url = _url;
WiFiClientSecure client;

// Defined in "CACert" tab.
extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;

void wifiConnect()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    oled.drawString(0, 0, "Connecting to WiFi network...");
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

  if (modeSelector[0] == 't')
  {
    oled.drawString(0, 0, message.c_str());
  }
  else
  {
    for (int i = 0; i <= message.length(); i++)
    {
      int x = i % 129;
      int y = i / 129;

      if (message[i] == '1')
      {
        oled.draw1x2Glyph(x, y, 1);
      }
    }
  }
  oled.display();
}

void waitForMessage()
{
  Serial.println("in message function");
  const int httpsPort = 443;
  const char *host = "api.github.com";
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

  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\""))
  {
    Serial.println("esp8266/Arduino CI successfull!");
  }
  else
  {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");
}

void setup()
{
  Serial.begin(9600);
  oled.begin();
  oled.setFont(u8x8_font_victoriamedium8_r);
  oled.clear();
  oled.drawString(0, 0, "Hello :)");

  delay(1000);

  wifiConnect();

  oled.clear();
  oled.drawString(0, 0, "Setting up...");
  oled.display();

  // Synchronize time useing SNTP. This is necessary to verify that
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

  EEPROM.begin(512);
  idSaved = EEPROM.get(142, idSaved);
  wasMessageRead = EEPROM.get(144, wasMessageRead);
}

void loop()
{
  if (!wasMessageRead)
  {
    Serial.println("Waiting for message");
    oled.clear();
    oled.drawString(0, 0, "Waiting for a message");
    oled.display();

    waitForMessage();
  }
  else
  {
    //Light up leds
    //Display message/
    //Find way to hide message
  }
}
