#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

const int LED_PIN = D1;
const int LED_COUNT = 25;
bool allSet = false;

void fadeToColor();
void fadeToBlack();

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

struct Pattern
{
  String patternName;
  int speed = 1000;
  int colorStart[3] = {255, 0, 0};
  int colorEnd[3] = {0, 0, 255};
  Pattern(String name) : patternName(name) {}                                                                                                                                                            // Default constructor with name only
  Pattern(String name, int speed, int startR, int startG, int startB, int endR, int endG, int endB) : patternName(name), speed(speed), colorStart{startR, startG, startB}, colorEnd{endR, endG, endB} {} // Parameterized constructor
  String printJson()
  {
    String json = "{\"patternName\":\"" + patternName + "\",\"colorStart\":[ " + String(colorStart[0]) + ", " + String(colorStart[1]) + ", " + String(colorStart[2]) + " ],\"colorEnd\":[ " + String(colorEnd[0]) + ", " + String(colorEnd[1]) + ", " + String(colorEnd[2]) + " ]" + ",\"speed\":" + String(speed) + "}";
    return json;
  }
};

struct mainStrip
{
  int lastSetColor[3] = {0, 0, 0};
  int color[3] = {0, 0, 0};
  int brightness = 255;
  Pattern pattern = Pattern("fullColor");
  bool stripOn = false;

  String printJson()
  {
    String json = "{\"color\":[ " + String(color[0]) + ", " + String(color[1]) + ", " + String(color[2]) + " ],\"brightness\": " + String(brightness) + ", \"pattern\":" + pattern.printJson() + ",\"stripOn\": " + (stripOn ? "true" : "false") + "}";
    return json;
  }
  void setLastColor(int color[3])
  {
    lastSetColor[0] = color[0];
    lastSetColor[1] = color[1];
    lastSetColor[2] = color[2];
  }
  void setLastColor(int r, int g, int b)
  {
    lastSetColor[0] = r;
    lastSetColor[1] = g;
    lastSetColor[2] = b;
  }
};

mainStrip stripState; // Create an instance of the struct to hold the current state of the strip

void setup()
{
  Serial.begin(115200);
  strip.begin();
  strip.clear();
  strip.show();

  fadeToColor();
  stripState.pattern.colorStart[0] = 0;
  stripState.pattern.colorStart[1] = 0;
  stripState.pattern.colorStart[2] = 0;
  stripState.pattern.colorEnd[0] = 0;
  stripState.pattern.colorEnd[1] = 255;
  stripState.pattern.colorEnd[2] = 0;
  fadeToColor();
  fadeToBlack();
}

void printHelp()
{
  Serial.println("Available commands:");
  Serial.println("\tstripOn: true/false");
  Serial.println("\tcolor: <Red> <Green> <Blue>");
  Serial.println("\tbrightness: <0-100>");
  Serial.println("\tcurrentPattern || return current Settings\n");
  Serial.println("\thelp: prints this message\n");

  Serial.println("Patterns:");

  Serial.println("\tfullColor: all LEDs are the same color");
  Serial.println("\trainbow: LEDs cycle through a rainbow of colors");
  Serial.println("\tchase: a fixed amount of LEDs chases across the strip");
  Serial.println("\tcolorWipe: The strip is filled with one color at a time, moving from one end to the other");
  Serial.println("\tfadeToColor: first fades to startColor and then fades to endColor");
  Serial.println("\trunningLights: LEDs run back and forth");
  Serial.println("\tfadeToBlack: The strip fades to black, one LED at a time");

  Serial.println("Examples:");

  Serial.println("\t\t{\"stripOn\":true, \"color\":[155,0,0], \"brightness\":90, \"pattern\":{\"patternName\":\"fullColor\"}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[0,0,150], \"brightness\":150, \"pattern\":{\"patternName\":\"fullColor\"}}");
  Serial.println("\t\t{\"stripOn\":true, \"brightness\":90, \"pattern\":{\"patternName\":\"rainbow\", \"speed\":200}}");
  Serial.println("\t\t{\"stripOn\":true, \"brightness\":150, \"pattern\":{\"patternName\":\"rainbow\", \"speed\":1000}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[150,0,0], \"brightness\":150, \"pattern\":{\"patternName\":\"chase\", \"speed\":50}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[0,0,150], \"brightness\":150, \"pattern\":{\"patternName\":\"chase\", \"speed\":200}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[255,0,0], \"brightness\":100, \"pattern\":{\"patternName\":\"colorWipe\", \"speed\":210}}");
  Serial.println("\t\t{\"stripOn\":false, \"color\":[255,255,0], \"brightness\":100, \"pattern\":{\"patternName\":\"colorWipe\", \"speed\":1000}}");
  Serial.println("\t\t{\"stripOn\":true, \"pattern\":{\"patternName\":\"colorWipe\"}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[55,10,100], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToColor\", \"colorStart\":[100,10,0], \"colorEnd\":[0,10,100], \"speed\":1000}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[55,10,100], \"brightness\":80, \"pattern\":{\"patternName\":\"fadeToColor\", \"colorStart\":[50,50,0], \"colorEnd\":[0,100,150], \"speed\":100}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"runningLights\", \"speed\":500}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"runningLights\", \"speed\":200}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToBlack\", \"speed\":140}}");
  Serial.println("\t\t{\"stripOn\":true, \"color\":[100,50,0], \"brightness\":40, \"pattern\":{\"patternName\":\"fadeToBlack\", \"speed\":20}}");
  Serial.println("\t\t{\"stripOn\":false}");

  Serial.println("If some settings are left out the last settings will be used");
  Serial.println("If stripOn is false, the strip will turn off. You can still change the color, pattern and brightness for the next time it is turned on");
}

void serialEvent()
{
  // get data from serial port into char array

  char incomingData[256];
  int index = 0;
  while (Serial.available() > 0 && index < 255)
  {
    delay(2);
    incomingData[index] = Serial.read();
    index++;
  }
  incomingData[index] = '\0'; // Null-terminate the string

  if (String(incomingData).equals("help"))
  {
    printHelp();
    return;
  }

  if (String(incomingData).equals("currentPattern"))
  {
    Serial.println(stripState.printJson());
    return;
  }

  allSet = false;

  // convert incoming String to JSON object
  JsonDocument doc;
  deserializeJson(doc, incomingData);

  // print json
  //serializeJsonPretty(doc, Serial);
  //Serial.println();
  if (doc == NULL)
  {
    Serial.println("Error");
    return;
  }

  // check if "stripOn" is false and turn off the strip
  if (doc.containsKey("stripOn"))
  {
    stripState.stripOn = doc["stripOn"].as<bool>();
  }

  // update strip state based on incoming JSON
  if (doc.containsKey("color"))
  {
    String color = doc["color"].as<String>();
    stripState.color[0] = doc["color"][0].as<int>();
    stripState.color[1] = doc["color"][1].as<int>();
    stripState.color[2] = doc["color"][2].as<int>();
  }
  if (doc.containsKey("brightness"))
  {
    stripState.brightness = doc["brightness"].as<int>();
  }
  if (doc.containsKey("pattern"))
  {
    if (doc["pattern"].containsKey("patternName"))
    {
      stripState.pattern.patternName = doc["pattern"]["patternName"].as<String>();
    }

    if (stripState.pattern.speed != doc["pattern"]["speed"].as<int>())
      stripState.pattern.speed = doc["pattern"]["speed"].as<int>();

    if (stripState.pattern.colorStart[1] != doc["pattern"]["colorStart"][0])
      stripState.pattern.colorStart[0] = doc["pattern"]["colorStart"][0].as<int>();
    if (stripState.pattern.colorStart[2] != doc["pattern"]["colorStart"][1])
      stripState.pattern.colorStart[1] = doc["pattern"]["colorStart"][1].as<int>();
    if (stripState.pattern.colorStart[3] != doc["pattern"]["colorStart"][2])
      stripState.pattern.colorStart[2] = doc["pattern"]["colorStart"][2].as<int>();
    if (stripState.pattern.colorEnd[1] != doc["pattern"]["colorEnd"][0])
      stripState.pattern.colorEnd[0] = doc["pattern"]["colorEnd"][0].as<int>();
    if (stripState.pattern.colorEnd[2] != doc["pattern"]["colorEnd"][1])
      stripState.pattern.colorEnd[1] = doc["pattern"]["colorEnd"][1].as<int>();
    if (stripState.pattern.colorEnd[3] != doc["pattern"]["colorEnd"][2])
      stripState.pattern.colorEnd[2] = doc["pattern"]["colorEnd"][2].as<int>();
  }

  Serial.println(stripState.printJson());
}

/////////////////////////////////////////// LED Strip Functions ///////////////////////////////////////////

int Wheel(byte WheelPos)
{

  // scale WheelPos from 0 to 255 with LED_COUNT
  WheelPos = map(WheelPos, 0, LED_COUNT, 0, 255);

  byte r, g, b;
  switch (WheelPos / 85)
  {
  case 0:
    r = 255 - WheelPos % 85; // Red decreases as WheelPos increases
    g = WheelPos % 85;       // Green increases as WheelPos increases
    b = 0;                   // Blue stays at 0
    break;
  case 1:
    r = 0;                   // Red stays at 0
    g = 255 - WheelPos % 85; // Green decreases as WheelPos increases
    b = WheelPos % 85;       // Blue increases as WheelPos increases
    break;
  case 2:
    r = WheelPos % 85;       // Red increases as WheelPos increases
    g = 0;                   // Green stays at 0
    b = 255 - WheelPos % 85; // Blue decreases as WheelPos increases
    break;
  }
  return strip.Color(r, g, b); // Return the color value for the current position in the rainbow
}

//////////////////////////FULL COLOR FUNCTION///////////////////////////////
void fullColor()
{
  for (int i = 0; i < LED_COUNT; i++)
  {
    strip.setPixelColor(i, strip.Color(stripState.color[0], stripState.color[1], stripState.color[2])); // Set the first pixel to the specified color
  }
  strip.setBrightness(stripState.brightness); // Set the brightness of the strip
  strip.show();                               // Update the LED strip with the new color and brightness
}

//////////////////////////RAINBOW FUNCTION///////////////////////////////
void rainbow()
{
  for (int i = 0; i < LED_COUNT; i++)
  {                                             // Cycle through all colors of the rainbow
    strip.setPixelColor(i, Wheel((i)));         // Set each pixel to a color in the rainbow
    strip.setBrightness(stripState.brightness); // Set the brightness of the strip
    strip.show();                               // Update the LED strip with the new colors
    delay(stripState.pattern.speed);            // Wait for delay before moving on to the next color
  }
}

//////////////////////////CHASE FUNCTION///////////////////////////////
void chase()
{
  int lightSection = 3; // Number of pixels in each section of the chase pattern

  // run around the led strip with 3 leds on
  
  strip.setBrightness(stripState.brightness); // Set the brightness of the strip
  int i = 0;

  for (; i < LED_COUNT + lightSection; i++) // Cycle through all pixels in the strip plus the number of pixels in each section
  {
    if (i < LED_COUNT)
      strip.setPixelColor(i % LED_COUNT, strip.Color(stripState.color[0], stripState.color[1], stripState.color[2]));
    if (i >= 0)
      strip.setPixelColor((i - lightSection) % LED_COUNT, strip.Color(0, 0, 0));
    strip.show();                    // Update the LED strip with the new colors
    delay(stripState.pattern.speed); // Wait for x milliseconds before moving on to the next color
  }
}

////////////////////////////////////////////////////COLORWIPE  FUNCTION///////////////////////////////
void colorWipe()
{
  auto c = strip.Color(stripState.color[0], stripState.color[1], stripState.color[2]);
  strip.setBrightness(stripState.brightness); // Set the brightness of the strip
  for (int i = 0; i < strip.numPixels(); i++) // Cycle through all pixels in the strip
  {
    strip.setPixelColor(i, c);       // Set the color of each pixel to the specified color
    strip.show();                    // Update the LED strip with the new colors
    delay(stripState.pattern.speed); // Wait for a specified amount of time before moving on to the next pixel
  }
}

////////////////////////////////////////////////////FADE FUNCTION///////////////////////////////
void fadeToColor()
{
  // first fades to startColor and then fades to endColor
  // Calculate the number of steps for each color component
  int stepsR1 = abs(stripState.lastSetColor[0] - stripState.pattern.colorStart[0]);
  int stepsG1 = abs(stripState.lastSetColor[1] - stripState.pattern.colorStart[1]);
  int stepsB1 = abs(stripState.lastSetColor[2] - stripState.pattern.colorStart[2]);

  int stepsR2 = abs(stripState.pattern.colorStart[0] - stripState.pattern.colorEnd[0]);
  int stepsG2 = abs(stripState.pattern.colorStart[1] - stripState.pattern.colorEnd[1]);
  int stepsB2 = abs(stripState.pattern.colorStart[2] - stripState.pattern.colorEnd[2]);

  int maxcount1 = max(stepsR1, max(stepsG1, stepsB1)); // Calculate the maximum number of steps
  int maxcount2 = max(stepsR2, max(stepsG2, stepsB2)); // Calculate the maximum number of steps

  if (stripState.pattern.colorStart[0] + stripState.pattern.colorStart[1] + stripState.pattern.colorStart[2] != 0)
  {
    for (int i = 0; i < maxcount1; i++)
    {
      int r = (stripState.lastSetColor[0] != stripState.pattern.colorStart[0]) ? (stripState.lastSetColor[0] > stripState.pattern.colorStart[0]) ? stripState.lastSetColor[0] - 1 : stripState.lastSetColor[0] + 1 : stripState.pattern.colorStart[0];
      int g = (stripState.lastSetColor[1] != stripState.pattern.colorStart[1]) ? (stripState.lastSetColor[1] > stripState.pattern.colorStart[1]) ? stripState.lastSetColor[1] - 1 : stripState.lastSetColor[1] + 1 : stripState.pattern.colorStart[1];
      int b = (stripState.lastSetColor[2] != stripState.pattern.colorStart[2]) ? (stripState.lastSetColor[2] > stripState.pattern.colorStart[2]) ? stripState.lastSetColor[2] - 1 : stripState.lastSetColor[2] + 1 : stripState.pattern.colorStart[2]; 


      stripState.lastSetColor[0] = r;
      stripState.lastSetColor[1] = g;
      stripState.lastSetColor[2] = b;

      for (int j = 0; j < LED_COUNT; j++)
      {
        strip.setPixelColor(j, strip.Color(r, g, b));
      }
      strip.show();
      delay(stripState.pattern.speed / (maxcount1 + maxcount2));
    }
  }

  for (int i = 0; i < maxcount2; i++)
  {
      int r = (stripState.lastSetColor[0] != stripState.pattern.colorEnd[0]) ? (stripState.lastSetColor[0] > stripState.pattern.colorEnd[0]) ? stripState.lastSetColor[0] - 1 : stripState.lastSetColor[0] + 1 : stripState.pattern.colorEnd[0];
      int g = (stripState.lastSetColor[1] != stripState.pattern.colorEnd[1]) ? (stripState.lastSetColor[1] > stripState.pattern.colorEnd[1]) ? stripState.lastSetColor[1] - 1 : stripState.lastSetColor[1] + 1 : stripState.pattern.colorEnd[1];
      int b = (stripState.lastSetColor[2] != stripState.pattern.colorEnd[2]) ? (stripState.lastSetColor[2] > stripState.pattern.colorEnd[2]) ? stripState.lastSetColor[2] - 1 : stripState.lastSetColor[2] + 1 : stripState.pattern.colorEnd[2]; 


    stripState.lastSetColor[0] = r;
    stripState.lastSetColor[1] = g;
    stripState.lastSetColor[2] = b;


    for (int j = 0; j < LED_COUNT; j++)
    {
      strip.setPixelColor(j, strip.Color(r, g, b));
    }
    strip.show();
    delay(stripState.pattern.speed / (maxcount1 + maxcount2));
  }
}

//////////////////////////////////////////////////////RUNNING LIGHTS FUNCTION//////////////////////////////////////////////////////

void runningLights()
{

  int lightSection = 3;

  strip.setBrightness(stripState.brightness);
  int i = 0;

  for (; i < LED_COUNT + lightSection; i++)
  {
    if (i < LED_COUNT)
      strip.setPixelColor(i % LED_COUNT, strip.Color(stripState.color[0], stripState.color[1], stripState.color[2]));
    if (i >= 0)
      strip.setPixelColor((i - lightSection) % LED_COUNT, strip.Color(0, 0, 0));
    strip.show();
    delay(stripState.pattern.speed / 2);
  }
  i--;
  for (; i > 0; i--)
  {
    if (i < LED_COUNT)
      strip.setPixelColor(i % LED_COUNT, strip.Color(0, 0, 0));
    if (i >= 0)
      strip.setPixelColor((i - lightSection) % LED_COUNT, strip.Color(stripState.color[0], stripState.color[1], stripState.color[2]));
    strip.show();
    delay(stripState.pattern.speed / 2);
  }
  strip.clear();
  strip.show();
}

///////////////////////////////FADE TO BLACK FUNKTION ///////////////////////////////

void fadeToBlack()
{
  // Calculate the number of steps for each color component
  int stepsR1 = abs(stripState.lastSetColor[0] - 0);
  int stepsG1 = abs(stripState.lastSetColor[1] - 0);
  int stepsB1 = abs(stripState.lastSetColor[2] - 0);

  int maxcount1 = max(stepsR1, max(stepsG1, stepsB1));

  if (stripState.pattern.colorStart[0] + stripState.pattern.colorStart[1] + stripState.pattern.colorStart[2] != 0)
  {
    for (int i = 0; i < maxcount1; i++)
    {
      int r = (stripState.lastSetColor[0] > 0) ? stripState.lastSetColor[0] - 1 : 0;
      int g = (stripState.lastSetColor[1] > 0) ? stripState.lastSetColor[1] - 1 : 0;
      int b = (stripState.lastSetColor[2] > 0) ? stripState.lastSetColor[2] - 1 : 0; 


      stripState.lastSetColor[0] = r;
      stripState.lastSetColor[1] = g;
      stripState.lastSetColor[2] = b;

      for (int j = 0; j < LED_COUNT; j++)
      {
        strip.setPixelColor(j, strip.Color(r, g, b));
      }
      strip.show();
      delay(stripState.pattern.speed / maxcount1);
    }
  }
}

void loop()
{
  // controll LED Strip
  if (!allSet)
  {
    if (stripState.stripOn)
    {
      if (stripState.pattern.patternName == "fullColor")
      {
        fullColor();
        stripState.setLastColor(stripState.color);
        allSet = true;
      }
      else if (stripState.pattern.patternName == "rainbow")
      {
        rainbow();
        stripState.setLastColor(stripState.color);
        allSet = true;
      }
      else if (stripState.pattern.patternName == "chase")
      {
        chase();
        stripState.setLastColor(0, 0, 0);
      }
      else if (stripState.pattern.patternName == "colorWipe")
      {
        colorWipe();
        stripState.setLastColor(stripState.color);
        allSet = true;
      }
      else if (stripState.pattern.patternName == "fadeToColor")
      {
        fadeToColor();
        stripState.setLastColor(stripState.pattern.colorEnd);
        allSet = true;
      }
      else if (stripState.pattern.patternName == "runningLights")
      {
        runningLights();
        stripState.setLastColor(0, 0, 0);
      }
      else if (stripState.pattern.patternName == "fadeToBlack")
      {
        fadeToBlack();
        stripState.setLastColor(0, 0, 0);
        allSet = true;
      }
    }
    else
    {
      strip.clear();                    // turn off all LEDs if the strip is not on
      strip.show();                     // update the strip to show the changes
      stripState.setLastColor(0, 0, 0); // set the last color to black (off)
      allSet = true;                    // set allSet to true to indicate that the strip is not on
    }
  }
}

/*
fullColor:
  all LEDs are the same color
rainbow:
  LEDs cycle through a rainbow of colors
chase:
  a fixed amount of LEDs chases across the strip
colorWipe:
  The strip is filled with one color at a time, moving from one end to the other
fadeToColor:
  first fades to startColor and then fades to endColor
runningLights:
  LEDs run back and forth
fadeToBlack:
  The strip fades to black, one LED at a time
*/

/*

{"stripOn":true, "color":[155,0,0], "brightness":90, "pattern":{"patternName":"fullColor"}}
{"stripOn":true, "color":[0,0,150], "brightness":150, "pattern":{"patternName":"fullColor"}}
{"stripOn":true, "color":[55,10,0], "brightness":90, "pattern":{"patternName":"rainbow", "speed":50}}
{"stripOn":true, "color":[0,0,0], "brightness":30, "pattern":{"patternName":"raindow", "speed":200}}
{"stripOn":true, "color":[55,10,0], "brightness":90, "pattern":{"patternName":"chase", "speed":50}}
{"stripOn":true, "color":[0,10,100], "brightness":100, "pattern":{"patternName":"chase", "speed":50}}
{"stripOn":true, "color":[55,10,100], "brightness":100, "pattern":{"patternName":"colorWipe", "speed":210}}
{"stripOn":false, "color":[55,10,100], "brightness":10, "pattern":{"patternName":"colorWipe", "speed":210}}
{"stripOn":true, "color":[55,10,100], "brightness":10, "pattern":{"patternName":"colorWipe", "speed":210}}
{"stripOn":true, "color":[55,10,100], "brightness":40, "pattern":{"patternName":"fadeToColor", "colorStart":[100,10,0], "colorEnd":[0,10,100], "speed":1000}}
{"stripOn":true, "color":[55,10,100], "brightness":80, "pattern":{"patternName":"fadeToColor", "colorStart":[50,50,0], "colorEnd":[0,100,150], "speed":100}}
{"stripOn":true, "color":[100,50,0], "brightness":40, "pattern":{"patternName":"runningLights", "speed":500}}
{"stripOn":true, "color":[55,10,0], "brightness":80, "pattern":{"patternName":"runningLights", "speed":140}}
{"stripOn":true, "pattern":{"patternName":"fadeToBlack", "speed":140}}
{"stripOn":false}

*/