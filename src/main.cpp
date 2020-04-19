//
// Love Box
//
#include <U8x8lib.h>
U8X8_SH1106_128X64_NONAME_HW_I2C oled(/* reset=*/U8X8_PIN_NONE);

void setup()
{
  Serial.begin(115200);
  oled.begin();
  oled.setFont(u8x8_font_victoriamedium8_r);
  oled.clear();
  oled.drawString(0,0,"Hello World!");
}

void loop()
{
}
