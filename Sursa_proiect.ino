#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SHT1x.h>
#include "ccs811.h"

#define dataPin  D4
#define clockPin D3
SHT1x sht1x(dataPin, clockPin);
LiquidCrystal_I2C lcd(0x3F, 16,2);
CCS811 ccs811(D3);

#include <ESP8266WiFi.h>        // Include the Wi-Fi library

//Didn't added ssid, pass and apiKey for security purposes :)

const char* ssid     = "";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "";     // The password of the Wi-Fi network
String apiKey = "";
const char* server = "api.thingspeak.com";

WiFiClient client;

void setup() {

   Serial.begin(115200);
   delay(10);
   
   Serial.println("Connecting to ");
   Serial.println(ssid);


   WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
 {
        delay(500);
        Serial.print(".");
 }
  Serial.println("");
  Serial.println("WiFi connected");

  lcd.begin();
  
  // ------- Quick 3 blinks of backlight for start  -------------
  for(int i = 0; i< 3; i++)
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight();

   // Enable I2C
  Wire.begin(); 
  
  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok= ccs811.begin();
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Print CCS811 versions
  Serial.print("setup: hardware    version: "); Serial.println(ccs811.hardware_version(),HEX);
  Serial.print("setup: bootloader  version: "); Serial.println(ccs811.bootloader_version(),HEX);
  Serial.print("setup: application version: "); Serial.println(ccs811.application_version(),HEX);
  
  // Start measuring
  ok= ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");

}

void loop() {

  float temp_c;
  float humidity;

  temp_c = sht1x.readTemperatureC();
  humidity = sht1x.readHumidity();

  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2,&etvoc,&errstat,&raw);

  lcd.setCursor(0,0);
  String temperatura = "Temperatura:" + String(temp_c) + "C";
  lcd.print(temperatura);
  lcd.backlight();

  lcd.setCursor(0,1);
  String umiditate = "Umiditate:" + String(humidity) + "%";
  lcd.print(umiditate);
  lcd.backlight();

  delay(5000);
  lcd.clear();

  lcd.setCursor(0,0);
  String co = "CO2:" + String(eco2) + " ppm";
  lcd.print(co);
  lcd.backlight();

  lcd.setCursor(0,1);
  String tvoc = "tVOC:" + String(etvoc) + " ppb";
  lcd.print(tvoc);
  lcd.backlight();

  delay(5000);
  
  if (isnan(temp_c) || isnan(humidity) || isnan(eco2) || isnan(etvoc)) 
     {
         Serial.println("Failed to read from sensor!");
          return;
     }

  if (client.connect(server,80))   //   "184.106.153.149" or api.thingspeak.com
  {  
     String postStr = apiKey;
     postStr +="&field1=";
     postStr += String(temp_c);
     postStr +="&field2=";
     postStr += String(humidity);
     postStr +="&field3=";
     postStr += String(eco2);
     postStr +="&field4=";
     postStr += String(etvoc);
     postStr += "\r\n\r\n";

     client.print("POST /update HTTP/1.1\n");
     client.print("Host: api.thingspeak.com\n");
     client.print("Connection: close\n");
     client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
     client.print("Content-Type: application/x-www-form-urlencoded\n");
     client.print("Content-Length: ");
     client.print(postStr.length());
     client.print("\n\n");
     client.print(postStr);

     Serial.print("Temperature: ");
     Serial.print(temp_c);
     Serial.print(" degrees Celcius, Humidity: ");
     Serial.print(humidity);
     Serial.print("%, CO2: ");
     Serial.print(eco2);
     Serial.print(" ppm, tVOC: ");
     Serial.print(etvoc);
     Serial.println(" ppb. Send to ThingSpreak.");
  }
  client.stop();

  Serial.println("Waiting...");

  delay(60000);
}
