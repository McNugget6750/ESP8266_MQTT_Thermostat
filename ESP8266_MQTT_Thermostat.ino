#include <ESP8266WiFi.h>
#include <stdio.h>
#include <time.h>
#include "Wire.h"
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include "cactus_io_BME280_I2C.h"
 
// ST7735 TFT module connections
#define TFT_RST   16     // TFT RST pin is connected to NodeMCU pin D4 (GPIO16)
#define TFT_CS    15     // TFT CS  pin is connected to NodeMCU pin D4 (GPIO15)
#define TFT_DC    0     // TFT DC  pin is connected to NodeMCU pin D4 (GPIO0)

// Bosch BME280 Connections (and other I2C sensors
#define I2C_SCL 4       // I2C SCL pin for the BME280
#define I2C_SDA 2       // I2C SDA pin for the BME280

#define NUM_CONSOLE_LINES 18 // How many lines are in the console

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
BME280_I2C bme(0x76); // I2C using address 0x76
 
float p = 3.1415926;

String consoleBuffer[19];
uint8_t consoleHead = 0;
uint8_t consoleTail = 0;

/*
 * Working variables for the thermostat
 */
float currentTempC = 0, lastTempC = 0;
float currentTempF = 0, lastTempF = 0;
float currentPressure = 0, lastPressure = 0;
float currentHumidity = 0, lastHumidity = 0;
time_t currentTime = 0, lastTime = 0;
bool connectionStatus = false, lastConnectionStatus = false;
float currentVoltage = 0, lastVoltage = 0;

/*
 * WIFI Configuration and variables
 */

const char* ssid = "wifi";
const char* password = "password";
int timezone = -8;
int dst = 0;

void setup(void) 
{
  for (uint8_t i = 0; i < NUM_CONSOLE_LINES; i++)
  {
    consoleBuffer[i] = String("");
  }
  
  uint8_t line = 0;
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
/********************************************************************/
// Initialize TFT Display
  tft.init();   // initialize a ST7735S chip, black tab
  tft.setRotation(2);   // Set the proper rotation of the screen to your device
  tft.fillScreen(ST7735_BLACK); // Reset the screen memory to a black background
  
  display_DrawTopStatusBar(false, -120, " "); // Draw the initial status bar.

  /*for (int test = 0; test < 50; test++)
  {
    addLine2Console(String("test ") + String(test));
    delay(500);
  }*/
  
/********************************************************************/
// Connect to WiFi
  addLine2Console(String("Initializing WiFi..."));
  WiFi.mode(WIFI_STA);
  addLine2Console(String("Setting user & passwd..."));
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  addLine2Console(String("Connecting..."));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    addLine2Console(String("waiting..."));
    delay(1000);
  }
  addLine2Console(String("Connected!"));
  addLine2Console(String("  "));

/********************************************************************/
// Connect NTP Server to Update Internal Time
  addLine2Console(String("Query time server..."));
  configTime(timezone * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("\nWaiting for time");
  addLine2Console(String("Waiting for time"));
  while (!time(nullptr)) {
    Serial.print(".");
    addLine2Console(String("waiting..."));  
    delay(1000);
  }
  Serial.println("Time and date updated.");
  addLine2Console(String("Time updated."));
  delay(2000); // Let the time settle. It seems it takes a certain amount of seconds to propergate
  
  struct     tm *ts;
  char       buf[80];
  // Print the current time to the display
  time_t now = time(nullptr);
  ts = localtime(&now);
  strftime(buf, 80, "%T", ts);
  addLine2Console(String(buf));
  addLine2Console(String("  "));
  delay(2000);
  
/**********************************************************************/
  addLine2Console(String("I2C Scanner:"));
// System check for sensors
  Serial.println();
  Serial.println("Start I2C scanner ...");
  Serial.print("\r\n");
  byte count = 0;
  addLine2Console(String("Begin Scan..."));
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
      addLine2Console(String("Device at: 0x") + String(i, HEX));
      count++;
      delay(1);
      }
  }
  Serial.print("\r\n");
  Serial.println("Finish I2C scanner");
  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" Device(s).");
  addLine2Console(String("I2C scan done."));
  addLine2Console(String(count) + String(" devices found."));
  delay(1000);

/**********************************************************************/
// Initialize BME280
  
  Serial.println("Bosch BME280 Pressure - Humidity - Temp Sensor | cactus.io");
  addLine2Console(String("Init Bosch BME280."));
  
  uint8_t chip_address = bme.begin(I2C_SDA, I2C_SCL);
  if (chip_address)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    Serial.println(chip_address, HEX);
    addLine2Console(String("Chip ID unknown:"));
    addLine2Console(String("ID: ") + String(chip_address, HEX));
    addLine2Console(String("  "));
    addLine2Console(String("HALT!"));
    
    while (1)
      yield();
  }

  addLine2Console(String("BME280 initialized."));
  addLine2Console(String("  "));
  addLine2Console(String("Set temperature offset."));
  bme.setTempCal(-1);// Temp was reading high so subtract 1 degree
  addLine2Console(String("Reading sensor:"));
  bme.readSensor();
  
  currentTempC = bme.getTemperature_C();
  currentTempF = bme.getTemperature_F();
  currentPressure = bme.getPressure_MB();
  currentHumidity = bme.getHumidity();

  addLine2Console(String("Temp C:") + String(currentTempC));
  addLine2Console(String("Temp F:") + String(currentTempF));
  addLine2Console(String("Pressure:") + String(currentPressure));
  addLine2Console(String("Humidity:") + String(currentHumidity));

  lastTempC = currentTempC;
  lastTempF = currentTempF;
  lastPressure = currentPressure;
  lastHumidity = currentHumidity;

  delay(3000);
  tft.fillScreen(ST7735_BLACK); // Reset the screen memory to a black background
  drawGraphBox();
}
 
void loop()
{
  struct     tm *ts;
  char       buf[26];
  // Print the current time to the display
  time_t now = time(nullptr);
  Serial.print(ctime(&now));
  //Serial.println(now % 86400); // Number of seconds since midnight
  /* Format and print the time, "ddd yyyy-mm-dd hh:mm:ss zzz" */
  ts = localtime(&now);
  
  strftime(buf, 26, "%r", ts);
  String h = ts->tm_hour < 10 ? (String("0") + String(ts->tm_hour)) : String(ts->tm_hour);
  String m = ts->tm_min < 10 ? (String("0") + String(ts->tm_min)) : String(ts->tm_min);
  //display_DrawTopStatusBar(true, WiFi.RSSI(), h + String(":") + m); // Draw the initial status bar.

  // Refresh the graph at 00:00:00 (hh:mm:ss) also known as Midnight
  if (ts->tm_hour == 0 && ts->tm_min == 0 && ts->tm_sec == 0)
    drawGraphBox();
    
  display_DrawTopStatusBar(true, WiFi.RSSI(), String(buf)); // Draw the initial status bar.

  // Read the sensor
  bme.readSensor();
  
  currentTempC = bme.getTemperature_C();
  currentTempF = bme.getTemperature_F();
  currentPressure = bme.getPressure_MB();
  currentHumidity = bme.getHumidity();
  
  Serial.print(currentPressure); Serial.print(" mb\t"); // Pressure in millibars
  Serial.print(currentHumidity); Serial.print(" %\t");
  Serial.print(currentTempC); Serial.print(" *C\t");
  Serial.print(currentTempF); Serial.println(" *F");

  display_UpdateSensorValues();
  drawHumidityGraph(currentHumidity, now % 86400);
  drawPressureGraph(currentPressure, now % 86400);
  drawTemperatureGraph(currentTempC, now % 86400);
  
  lastTempC = currentTempC;
  lastTempF = currentTempF;
  lastPressure = currentPressure;
  lastHumidity = currentHumidity;


  delay(1000); //just here to slow down the output
}

void display_UpdateSensorValues(void)
{
  uint8_t xPos = 2;
  uint8_t yPos = 10;
  uint8_t yDelta = 20;
  uint8_t i = 0;

  display_PrintTextAdv(String(currentTempC) + String(" *C"), xPos, yPos + yDelta * i++, 1, 4, ST7735_RED, ST7735_BLACK);
  display_PrintTextAdv(String(currentTempF) + String(" *F"), xPos, yPos + yDelta * i++, 1, 2, ST7735_RED, ST7735_BLACK);
  display_PrintTextAdv(String(currentHumidity) + String(" %"), xPos, yPos + yDelta * i++, 1, 4, ST7735_BLUE, ST7735_BLACK);
  display_PrintTextAdv(String(currentPressure) + String(" mb"), xPos, yPos + yDelta * i++, 1, 2, ST7735_GREEN, ST7735_BLACK);
}

// Draws the bar at the top showing connection status and current time / date
void display_DrawTopStatusBar(bool isConnected, uint32_t wifiRSSI, String timeNow)
{
  tft.fillRect(0, 0, 159, 9, ST7735_BLUE);
  display_PrintText(timeNow, 6*3, 1, 0, ST7735_WHITE);
  
  if (isConnected) 
  {
    if (wifiRSSI < -90)
      display_PrintText(".___", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -90 && wifiRSSI < -80)
      display_PrintText("..__", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -80 && wifiRSSI < -70)
      display_PrintText("...|", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -70 && wifiRSSI < -67)
      display_PrintText("..||", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -67 && wifiRSSI < -50)
      display_PrintText(".|||", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -50 && wifiRSSI < -30)
      display_PrintText("||||", 6*17, 1, 0, ST7735_WHITE);
    else if (wifiRSSI >= -30)
      display_PrintText("####", 6*17, 1, 0, ST7735_WHITE);
  }
  else
    display_PrintText("____", 6*17, 1, 0, ST7735_WHITE);
}

void display_PrintText(String textBuffer, int x, int y, int textSize, int color)
{
  if (textSize == 0) textSize = 1;
  tft.setTextColor(color);
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(textBuffer.c_str());
}

void display_PrintTextAdv(String textBuffer, int x, int y, int textSize, int font, int forgroundColor, int backgroundColor)
{
  if (textSize == 0) textSize = 1;
  tft.setTextColor(forgroundColor, backgroundColor);
  tft.setTextSize(textSize);
  tft.drawString(textBuffer.c_str(), x, y, font);
}

void drawGraphBox(void)
{
  tft.fillRect(0, 100, 128, 60, ST7735_BLACK); // Delete the entire graph
  tft.drawFastHLine(0, 157, 128, ST7735_WHITE);
  tft.drawPixel(126, 156, ST7735_WHITE);
  tft.drawPixel(126, 158, ST7735_WHITE);
  tft.drawPixel(125, 155, ST7735_WHITE);
  tft.drawPixel(125, 159, ST7735_WHITE);
  tft.drawFastVLine(2, 100, 60, ST7735_WHITE);
  tft.drawPixel(1, 101, ST7735_WHITE);
  tft.drawPixel(3, 101, ST7735_WHITE);
  tft.drawPixel(0, 102, ST7735_WHITE);
  tft.drawPixel(4, 102, ST7735_WHITE);

  tft.drawFastVLine(32, 100, 57, TFT_DARKGREY); // 6am marker
  tft.drawFastVLine(64, 100, 57, TFT_LIGHTGREY); // 12PM marker
  tft.drawFastVLine(96, 100, 57, TFT_DARKGREY); // 6PM marker

  display_PrintText("6am", 23, 92, 1, ST7735_DARKGREY);
  display_PrintText("12PM", 52, 92, 1, ST7735_LIGHTGREY);
  display_PrintText("6PM", 87, 92, 1, ST7735_DARKGREY);

  for (uint32_t i = 0; i < 86400; i+=3600)
    drawTemperatureGraph(20.5f, i);
}

void drawTemperatureGraph(float temperature, uint32_t secondsSinceMidnight)
{
  // 86400 seconds in a day - also number of samples
  // 125 pixel to draw
  uint8_t xPos = 3 + 0.001446759f * secondsSinceMidnight; // calculate the location of the pixel with a 3 pixel offset
  uint8_t yPos = 1.5f * temperature;
  tft.drawPixel(xPos, 160 - yPos, ST7735_RED);
}

void drawPressureGraph(float pressure, uint32_t secondsSinceMidnight)
{
  // Standard air pressure at sea level is 1013.25 mb. 
  // The highest air pressure recorded was 1084 mb in Siberia. 
  // The lowest air pressure, 870 mb, was recorded in a typhoon in the Pacific Ocean.
  // Source: https://sciencing.com/range-barometric-pressure-5505227.html
  // I choose to draw +-50mbar around 1013mb
  
  // 86400 seconds in a day - also number of samples
  // 125 pixel to draw
  uint8_t xPos = 3 + 0.001446759f * secondsSinceMidnight;
  uint8_t yPos = 0.6f * (pressure - (963)); // A range of 100mb in 60 pixels
  tft.drawPixel(xPos, 160 - yPos, ST7735_GREEN);
}

void drawHumidityGraph(float humidity, uint32_t secondsSinceMidnight)
{
  // 86400 seconds in a day - also number of samples
  // 125 pixel to draw
  uint8_t xPos = 3 + 0.001446759f * secondsSinceMidnight;
  uint8_t yPos = 0.6 * humidity; // we need to resolve 100% in 60 pixels
  tft.drawPixel(xPos, 160 - yPos, ST7735_BLUE);
}

void addLine2Console(String line)
{
  uint8_t head = consoleHead;
  // This is clearly a hack to clear the memory. I'm not sure what happens here, to be honest.
  // I thought String() has a copy constructor but it doesn't appear to work.
  consoleBuffer[consoleHead++] = String(line) + String("                "); 
  consoleHead %= NUM_CONSOLE_LINES;

  consoleTail = NUM_CONSOLE_LINES - consoleHead;

  for (uint8_t i = 0; i < NUM_CONSOLE_LINES; i++)
  {
    //display_PrintText(String(i), 0, 10 + 8 * i, 1, ST7735_WHITE);
    display_PrintTextAdv(consoleBuffer[head], 0, 152 - 8 * i, 1, 1, ST7735_WHITE, ST7735_BLACK);
    head--;
    if (head == 255)
      head = NUM_CONSOLE_LINES;
  }
}

