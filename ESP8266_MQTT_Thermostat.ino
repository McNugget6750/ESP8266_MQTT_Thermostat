#include "Wire.h"
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "cactus_io_BME280_I2C.h"
 
// ST7735 TFT module connections
#define TFT_RST   16     // TFT RST pin is connected to NodeMCU pin D4 (GPIO2)
#define TFT_CS    15     // TFT CS  pin is connected to NodeMCU pin D4 (GPIO0)
#define TFT_DC    0     // TFT DC  pin is connected to NodeMCU pin D4 (GPIO4)
// initialize ST7735 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)

#define I2C_SCL 4
#define I2C_SDA 2

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
BME280_I2C bme(0x76); // I2C using address 0x76
 
float p = 3.1415926;
 
void setup(void) {

  Serial.begin(115200); 
  while(!Serial){} // Waiting for serial connection
 
  Serial.println();
  Serial.println("Start I2C scanner ...");
  Serial.print("\r\n");
  byte count = 0;
  
  Wire.begin(I2C_SDA, I2C_SCL);
  
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0)
      {
      Serial.print("Found I2C Device: ");
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      delay(1);
      }
  }
  Serial.print("\r\n");
  Serial.println("Finish I2C scanner");
  Serial.print("Found ");
  Serial.print(count, HEX);
  Serial.println(" Device(s).");
  
delay(1000);

/**********************************************************************/

Serial.println("Bosch BME280 Pressure - Humidity - Temp Sensor | cactus.io");

uint8_t chip_address = bme.begin(I2C_SDA, I2C_SCL);
if (chip_address)
{
  Serial.println("Could not find a valid BME280 sensor, check wiring!");
  Serial.println(chip_address, HEX);
  
  while (1)
    yield();
}

bme.setTempCal(-1);// Temp was reading high so subtract 1 degree

bme.readSensor();

Serial.print(bme.getPressure_MB()); Serial.print(" mb\t"); // Pressure in millibars
Serial.print(bme.getHumidity()); Serial.print(" %\t\t");
Serial.print(bme.getTemperature_C()); Serial.print(" *C\t");
Serial.print(bme.getTemperature_F()); Serial.println(" *F");

 /********************************************************************/
  
  tft.init();   // initialize a ST7735S chip, black tab
 
  tft.setRotation(2);
  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  time = millis() - time;
  
  delay(500);
 
  // large block of text
  tft.fillScreen(ST7735_BLACK);

  display_PrintText("24h Cycle", 0, 0, 0, ST7735_WHITE);
}
 
void loop() {
  bme.readSensor();
  
  Serial.print(bme.getPressure_MB()); Serial.print(" mb\t"); // Pressure in millibars
  Serial.print(bme.getHumidity()); Serial.print(" %\t\t");
  Serial.print(bme.getTemperature_C()); Serial.print(" *C\t");
  Serial.print(bme.getTemperature_F()); Serial.println(" *F");
  
  // Add a 10 second delay so we can look at the data in 24h
  delay(10000); //just here to slow down the output
}

void display_PrintText(String textBuffer, int x, int y, int textSize, int color)
{
  if (textSize == 0) textSize = 1;
  tft.setTextColor(color);
  tft.setTextSize(textSize);
  tft.setCursor(x, y);
  tft.print(textBuffer.c_str());
}

