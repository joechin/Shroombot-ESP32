#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SparkFunCCS811.h>
#include <SparkFunBME280.h>
#include <Adafruit_NeoPixel.h>

// Env sensor block
#define DHTPIN A0
#define DHTTYPE DHT11
#define CCS811_ADDR 0x5B // Default I2C Address CCS811
#define BME280_ADDR 0x77 // Default I2C Address BME280

// LED Block
#define NUM_LEDS 30
#define LED_TYPE WS2812
#define DATA_PIN 13
#define MODE_BUTTON A0
#define FAN_BUTTON A1
#define HUMIDIFER_BUTTON A2
#define LED_BUTTON A3

// WIFI settings
const char *SSID = "Cisco21551";
const char *PWD = "boomboom";

Adafruit_NeoPixel ledStrip(NUM_LEDS,DATA_PIN,NEO_GRB);

// DHT dht(DHTPIN, DHTTYPE);
CCS811 myVOCSensor(CCS811_ADDR);
BME280 myBME280;

float humidity = 0;
float probeTemp = 0;
float eCO2 = 0;
float TVOC = 0;
float ambientPressure = 0;

String valueJSON;

WebServer apiServer(80);
StaticJsonDocument<1024> jsonDoc;
char buffer[1024];

void connectWifi() {
  Serial.print("Connecting to...");
  Serial.println(SSID);

  WiFi.begin(SSID,PWD);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup_routing() {
  apiServer.on("/TEMPERATURE", getTemperature());
  apiServer.on("/ECO2", getECO2());
  apiServer.on("/TVOC", getTVOC());
  apiServer.on("/AMBIENTPRESSURE", getAmbientPressure());
  apiServer.on("/HUMIDITY", getHumidity());

  // Start server
  apiServer.begin();
}

void create_json(char *tag, float value, char *unit) {
  jsonDoc.clear();
  jsonDoc["type"] = tag;
  jsonDoc["value"] = value;
  jsonDoc["unit"] = unit;

  serializeJson(jsonDoc, buffer);
}

void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDoc.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit;
}

void getTemperature() {
  Serial.println("Getting temperature.");
  create_json("temperature", probeTemp,"C");
  apiServer.send(200,"application/json",buffer);
}

void getECO2() {
  Serial.println("Getting eCO2.");
  create_json("ECO2", eCO2,"ppm");
  apiServer.send(200,"application/json",buffer);
}

void getTVOC() {
  Serial.println("Getting TVOC.");
  create_json("TVOC", TVOC,"ppm");
  apiServer.send(200,"application/json",buffer);
}
void getAmbientPressure() {
  Serial.println("Getting ambient pressure.");
  create_json("ambientpressure", ambientPressure,"Pa");
  apiServer.send(200,"application/json",buffer);
}
void getHumidity() {
  Serial.println("Getting humidity.");
  create_json("humidity", humidity,"%");
  apiServer.send(200,"application/json",buffer);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  // Initialize BME280
  myBME280.settings.commInterface = I2C_MODE;
  myBME280.settings.I2CAddress = BME280_ADDR;
  myBME280.settings.runMode = 3;
  myBME280.settings.tStandby = 0;
  myBME280.settings.filter = 4;
  myBME280.settings.tempOverSample = 5;
  myBME280.settings.pressOverSample = 5;
  myBME280.settings.humidOverSample = 5;

  Wire.begin();
  Serial.print("Starting BME280...");
  Serial.println(myBME280.begin(),HEX);

  if(!myVOCSensor.begin()) {
    Serial.println("Ccs811 failed to initialize. Check Connection.");
    while(1);
  }

  Serial.println("Begin LED strip");
  ledStrip.begin();
  ledStrip.show(); // Initiliaze to off.

  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(FAN_BUTTON, INPUT_PULLUP);
  pinMode(HUMIDIFER_BUTTON, INPUT_PULLUP);
  pinMode(LED_BUTTON, INPUT_PULLUP);

  // Serial.println("DHT Test.");
  // dht.begin();

  //deserializeJson(doc, F("{\"temp\":\"temp\",\"humidity\":\"humidity\",\"hic\":\"hic}"));

  //serializeValues();

}

void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;
  int nRepeat = 0;
  for(int a=0;a<nRepeat;a++) {
    for(int b=0;b<3;b++) {
      ledStrip.clear();
      for(int c=b;c<NUM_LEDS;c+=3) {
        int hue = firstPixelHue + c * 65536L / NUM_LEDS;
        uint32_t color = ledStrip.gamma32(ledStrip.ColorHSV(hue));
        ledStrip.setPixelColor(c,color);
      }
      ledStrip.show();
      delay(wait);
      firstPixelHue += 65536 / 90;
    }
  }
}

void pulseWhite(uint8_t wait) {
  for(int j=0; j<256; j++) { // Ramp up from 0 to 255
    // Fill entire strip with white at gamma-corrected brightness level 'j':
    ledStrip.fill(ledStrip.Color(0, 0, 0, ledStrip.gamma8(j)));
    ledStrip.show();
    delay(wait);
  }

  for(int j=255; j>=0; j--) { // Ramp down from 255 to 0
    ledStrip.fill(ledStrip.Color(0, 0, 0, ledStrip.gamma8(j)));
    ledStrip.show();
    delay(wait);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(myVOCSensor.dataAvailable()) {
    myVOCSensor.readAlgorithmResults();
    eCO2 = myVOCSensor.getCO2();
    TVOC = myVOCSensor.getTVOC();
    probeTemp = myBME280.readTempC();
    humidity = myBME280.readFloatHumidity();
    ambientPressure = myBME280.readFloatPressure();
    
  }
  Serial.print("eCO2: ");
  Serial.println(eCO2);
  Serial.print("TVOC: ");
  Serial.println(TVOC);
  Serial.print("Temperature: ");
  Serial.println(probeTemp);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  Serial.print("Ambient Pressure: ");
  Serial.println(ambientPressure);
  Serial.println("");

  // ledStrip.setPixelColor(1,255,255,255);
  // ledStrip.setBrightness(255);
  // ledStrip.show();
  uint32_t magenta = ledStrip.Color(255,0,255);
  ledStrip.fill(magenta,0,NUM_LEDS);
  ledStrip.setBrightness(100);
  ledStrip.show();
  // pulseWhite(5);
  delay(1000);
}

