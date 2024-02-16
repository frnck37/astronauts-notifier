#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <Adafruit_SPITFT.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "heltec.h"
#include <TimeLib.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

float mylat = 47.344181;
float mylon = 0.661440;
float isslat, isslon;
int distance, number, count;
String payload;
String* name;    // Dynamic array for names
String* craft;   // Dynamic array for craft names
String risetime[5];
float duration[5];

float p = 3.1415926;

// Variables de travail
unsigned long epoch = 0;
int nujour = 0; //numero jour de la semaine avec 0 pour dimanche
String jour = "mon jour"; // dimanche, lundi, etc.
String heure = "mon heure ..";
char buffer[80]; // Stockage de la date complete

//Creation objet WIFI UDP puis du client NTP
WiFiUDP ntpUDP;

//Creation objet client NTP avec les parametres suivants :
// - pool de serveurs NTP
// - en option le décalage horaire en secondes, ici 3600 pour GMT+1, pour GMT+8 mettre 28800, etc.
// - en option l intervalle de mise à jour en millisecondes par défaut à 60 secondes
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

const char *proxyServer = "https://corsproxy.org/?"; // Replace with your proxy server URL

const String iss = "http://api.open-notify.org/iss-now.json";
const String ppl = "http://api.open-notify.org/astros.json";
String pas = "http://api.open-notify.org/iss-pass.json?";

const char* exoplanetApiUrl = "https://corsproxy.org/?https%3A%2F%2Fexoplanetarchive.ipac.caltech.edu%2FTAP%2Fsync%3Fquery%3Dselect%2Bcount(*)%2Bfrom%2Bpscomppars%26format%3Djson";

//const char* rocketLaunchURL = "https://fdo.rocketlaunch.live/json/launches/next/1";

const  String fetchDataFromAPI(const char *proxyUrl, const char *apiUrl); 

void getplanet(); // Function prototype exoplanetes  declaration
//void getrocket();

void setup() {
  
   uint16_t time = millis();

  time = millis() - time;
  
  Serial.begin(9600);
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Serial Enable*/);
  Heltec.display->init();

  Heltec.display->clear();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_24);   
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(65, 0, "Space Infos");
  Heltec.display->drawString(65, 18, "情報エリア");
  Heltec.display->display();
  delay(800);

  WiFiManager wifiManager;
  //wifiManager.autoConnect("Space-wifi-setup");
  bool connected = wifiManager.autoConnect("Space-wifi-Setup");
  pas = pas + "lat=" + (String)mylat + "&lon=" + (String)mylon;
  Serial.println(pas);
  
  timeClient.begin(); 
  timeClient.setTimeOffset(3600);  // Set your time zone offset in seconds (3600 = GMT+1)

  if (!connected) {
    // If not connected, display a message
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(0, 0, "do WiFi setup");
     Heltec.display->drawString(0, 17, "無線LAN");
    Heltec.display->display();
  }
  
}

void loop() {
  
  // Recup heure puis affichage
  timeClient.update();
  epoch = timeClient.getEpochTime(); // Heure Unix
  nujour = timeClient.getDay();    // jour de la semaine
  heure = timeClient.getFormattedTime(); // heure
  // Calcul de la date en convertissant le temps UNIX epoch
  time_t timestamp = epoch;
  struct tm * pTime = localtime( & timestamp );
  strftime( buffer,80, "%Y-%m-%d %H:%M", pTime );
  //Serial.println(buffer);

  switch (nujour) { // on determine le jour
      case 0: 
        jour = "dimanche";
        break;
      case 1:
        jour = "lundi";
        break;
      case 2: 
        jour = "mardi";
        break;
      case 3: 
        jour = "mercredi";
        break;
      case 4: 
        jour = "jeudi";
        break;
      case 5: 
        jour = "vendredi";
        break;
       case 6: 
        jour = "samedi";
        break;
    }
    
  displayDateTime(); // Display date and time before ISS position
  
   getJson(iss);  
  decodeLocJson();
  getDistance();
  issLocOLEDDisplay();
  issLocSerialDisplay();
  delay(5000);

  getJson(ppl);
  decodePeopleJson();
  displayPeopleSerial();
  displayPeopleOLED();  
  delay(5000); 
   
  //getrocket();
  getplanet();
  delay(5000); 
}


void displayDateTime() {
//  time_t now = time(nullptr);
//  struct tm *tm_info = localtime(&now);
//  char buffer[80]; // Buffer for date and time string
//  
//  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", tm_info); // Format the date and time
//  
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(60, 0, jour); // Display date and time
  Heltec.display->drawString(61, 16, buffer); // Display date and time
  //Heltec.display->drawString(60, 23, heure); // Display date and time
  Heltec.display->display();
  delay(4000); // Display for 4 seconds before clearing the screen
}
void issLocOLEDDisplay() {
  Heltec.display->clear();

  // Set font size and text alignment
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Display ISS position
  char temp[15];
  sprintf(temp, "%d.%02d, %d.%02d", (int)isslat, abs((int)(isslat * 100) % 100), (int)isslon, abs((int)(isslon * 100) % 100));
  Heltec.display->drawString(64, 1, "ISS :  " + String(temp));

  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);

  // Display distance
  char temp1[30];
  sprintf(temp1, "Dist: %d km", distance);
  Heltec.display->drawString(60, 16, String(temp1));

  Heltec.display->display();
  delay(1000); // Adjust the delay to control how long the information is displayed
}

void issLocSerialDisplay() {
  Serial.print("Distance");
  Serial.print(isslat, 4);
  Serial.print(",");
  Serial.println(isslon, 4);
  Serial.print("The ISS is about ");
  Serial.print(distance);
  Serial.println(" miles from you now.\nAnd moving fast!");
}

void getJson(String url) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient wifiClient;
    HTTPClient http;
    http.begin(wifiClient, url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      payload = http.getString();
    }
    http.end();
  }
}

void decodeLocJson() {
  DynamicJsonDocument root(1024);
  deserializeJson(root, payload);
  auto error = deserializeJson(root, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }
  isslat = root["iss_position"]["latitude"];
  isslon = root["iss_position"]["longitude"];
}
void getDistance() {
  float theta, dist, kilometers;
  theta = mylon - isslon;
  dist = sin(deg2rad(mylat)) * sin(deg2rad(isslat)) + cos(deg2rad(mylat)) * cos(deg2rad(isslat)) * cos(deg2rad(theta));
  dist = acos(dist);
  dist = rad2deg(dist);
  kilometers = dist * 60 * 1.852; // Convert from nautical miles to kilometers
  distance = kilometers;
}


float deg2rad(float n) {
  float radian = (n * 71) / 4068;
  return radian;
}

float rad2deg(float n) {
  float degree = (n * 4068) / 71;
  return degree;
}

void decodePeopleJson() {
  DynamicJsonDocument root(1024);
  deserializeJson(root, payload);
  auto error = deserializeJson(root, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return;
  }
  number = root["number"];
  
  // Dynamically allocate memory based on the number of people
  name = new String[number];
  craft = new String[number];
  
  for (int i = 0; i < number; i++) {
    const char* temp1 = root["people"][i]["name"];
    const char* temp2 = root["people"][i]["craft"];
    name[i] = (String)temp1;
    craft[i] = (String)temp2;
  }
}

void displayPeopleSerial() {
  Serial.print("Astronauts in space:/宇宙飛行士");
  Serial.print(number);
  for (int i = 0; i < number; i++) {
    Serial.print(name[i]);
    Serial.print(" - ");
    Serial.println(craft[i]);
  }
}

void displayPeopleOLED() {
  Heltec.display->clear();
 
  char temp2[50];
  sprintf(temp2," %d astronautes en vol  >", number);

  int textWidth = Heltec.display->getStringWidth(temp2);
  int xPos = 128; // Start xPos at the right edge of the display

  while (xPos > -textWidth) {
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->drawString(xPos, 10, temp2);
    Heltec.display->display();
    delay(10); // Adjust the delay to control scrolling speed
    xPos--;
  }

  delay(1000); // Add a delay before displaying individual astronaut names

  for (int i = 0; i < number; i++) {
    textWidth = Heltec.display->getStringWidth((String)i + " : " + craft[i] + " -- " +name[i] );
    xPos = 128; // Start xPos at the right edge of the display

    while (xPos > -textWidth) {
      Heltec.display->clear();
      Heltec.display->setFont(ArialMT_Plain_16);
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->drawString(xPos, 10, (String)i + " : " + craft[i] + " -- " +name[i]);
      Heltec.display->display();
      delay(10); // Adjust the delay to control scrolling speed
      xPos--;
    }

    // Add a delay between texts
    delay(100);
  }
}

//
//void getrocket(){
//  
//if (WiFi.status() == WL_CONNECTED) {
//    WiFiClient wifiClient;
//    HTTPClient http;
//    http.begin(wifiClient, proxyServer);
//    int httpCode = http.GET();
//    if (httpCode > 0) {
//      payload = http.getString();
//    }
//    http.end();
//  }
//
////  if (http.begin(wificlient, proxyUrl)) {
////    // Add the API URL as a parameter to your proxy server
////    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
////    String requestUrl = apiUrl;
////    int httpCode = http.GET(requestUrl);
////    
////    if (httpCode > 0) {
////      if (httpCode == HTTP_CODE_OK) {
////        String payload = http.getString();
////        http.end();
////        return payload;
////      }
////    }
//
//    // Fetch data from the rocket API using the proxy server
//  String apiData = fetchDataFromAPI(proxyServer, rocketLaunchURL);
//
//  // Parse JSON data here (you can use the ArduinoJson library)
//  JSON_ARRAY_SIZE(0) + 3*JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(4) + 2*JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(28);
//  // Extract the last number of confirmed exoplanets
//// Stream& input;
//
//DynamicJsonDocument doc(3072);
//
//DeserializationError error = deserializeJson(doc, apiData); // Use apiData here
//
//if (error) {
//  Serial.print(F("deserializeJson() failed: "));
//  Serial.println(error.f_str());
//  return;
//}
//
//bool valid_auth = doc["valid_auth"]; // false
//int count = doc["count"]; // 1
//int limit = doc["limit"]; // 1
//int total = doc["total"]; // 130
//int last_page = doc["last_page"]; // 130
//
//JsonObject result_0 = doc["result"][0];
//int result_0_id = result_0["id"]; // 3546
//const char* result_0_cospar_id = result_0["cospar_id"]; // nullptr
//const char* result_0_sort_date = result_0["sort_date"]; // "1695105000"
//const char* result_0_name = result_0["name"]; // "\"We Will Never Desert You\""
//
//JsonObject result_0_provider = result_0["provider"];
//int result_0_provider_id = result_0_provider["id"]; // 26
//const char* result_0_provider_name = result_0_provider["name"]; // "Rocket Lab"
//const char* result_0_provider_slug = result_0_provider["slug"]; // "rocket-lab"
//
//JsonObject result_0_vehicle = result_0["vehicle"];
//int result_0_vehicle_id = result_0_vehicle["id"]; // 18
//const char* result_0_vehicle_name = result_0_vehicle["name"]; // "Electron"
//int result_0_vehicle_company_id = result_0_vehicle["company_id"]; // 26
//const char* result_0_vehicle_slug = result_0_vehicle["slug"]; // "electron"
//
//JsonObject result_0_pad = result_0["pad"];
//int result_0_pad_id = result_0_pad["id"]; // 122
//const char* result_0_pad_name = result_0_pad["name"]; // "LC-1B"
//
//JsonObject result_0_pad_location = result_0_pad["location"];
//int result_0_pad_location_id = result_0_pad_location["id"]; // 20
//const char* result_0_pad_location_name = result_0_pad_location["name"]; // "Rocket Lab Launch Complex, ...
//// result_0_pad_location["state"] is null
//// result_0_pad_location["statename"] is null
//const char* result_0_pad_location_country = result_0_pad_location["country"]; // "New Zealand"
//const char* result_0_pad_location_slug = result_0_pad_location["slug"];
//
//JsonObject result_0_missions_0 = result_0["missions"][0];
//int result_0_missions_0_id = result_0_missions_0["id"]; // 5380
//const char* result_0_missions_0_name = result_0_missions_0["name"]; // "Acadia-2"
//const char* result_0_missions_0_description = result_0_missions_0["description"]; // "The second of 4 ...
//
//const char* result_0_mission_description = result_0["mission_description"]; // "The second of 4 missions ...
//const char* result_0_launch_description = result_0["launch_description"]; // "A Rocket Lab Electron ...
//// result_0["win_open"] is null
//const char* result_0_t0 = result_0["t0"]; // "2023-09-19T06:30Z"
//// result_0["win_close"] is null
//
//JsonObject result_0_est_date = result_0["est_date"];
//int result_0_est_date_month = result_0_est_date["month"]; // 9
//int result_0_est_date_day = result_0_est_date["day"]; // 19
//int result_0_est_date_year = result_0_est_date["year"]; // 2023
//// result_0_est_date["quarter"] is null
//
//const char* result_0_date_str = result_0["date_str"]; // "Sep 19"
//
//int result_0_tags_0_id = result_0["tags"][0]["id"]; // 25
//const char* result_0_tags_0_text = result_0["tags"][0]["text"]; // "Earth Observation Satellite"
//
//const char* result_0_slug = result_0["slug"]; // "acadia-2"
//// result_0["weather_summary"] is null
//// result_0["weather_temp"] is null
//// result_0["weather_condition"] is null
//// result_0["weather_wind_mph"] is null
//// result_0["weather_icon"] is null
//// result_0["weather_updated"] is null
//const char* result_0_quicktext = result_0["quicktext"]; // "Electron - \"We Will Never Desert You\" - ...
//
//int result_0_result = result_0["result"]; // -1
//bool result_0_suborbital = result_0["suborbital"]; // false
//const char* result_0_modified = result_0["modified"]; // "2023-09-16T14:17:06+00:00"
//
//  
// Heltec.display->clear();
//  Heltec.display->setFont(ArialMT_Plain_16);
//  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
//
//  // Print name and description line by line, scrolling if needed
//  int yOffset = 0;
//  while (yOffset < 32) { // Assuming 32 is the OLED height
//    Heltec.display->clear();
//    Heltec.display->drawString(0, yOffset, *name);
//    Heltec.display->drawString(0, yOffset + 10,result_0_mission_description);
//    Heltec.display->display();
//    delay(8); // Adjust the delay to control scrolling speed
//    yOffset++;
//  }
//  delay(5000);  // Adjust the delay between updates
//  }

  //exoplanètes 
  
String fetchExoplanetData(const char* apiUrl) {
  WiFiClient wifiClient; // Create a WiFiClient instance
  HTTPClient http;

  // Use the WiFiClient instance with begin to make the request
  if (http.begin(wifiClient, apiUrl)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      http.end();
      return payload;
    }
    http.end();
  }

  return "";
}
  
void getplanet() {    
  // Fetch data from the Exoplanet API
  String exoplanetData = fetchExoplanetData(exoplanetApiUrl);

  // Parse JSON response to extract the count of confirmed exoplanets
  StaticJsonDocument<64> doc;

  DeserializationError error = deserializeJson(doc, exoplanetData); // Use exoplanetData here

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  int root_0_count_ = doc[0]["count(*)"]; // 5514

  // Display the count of confirmed exoplanets on the Heltec OLED and serial   Serial.print(F(String(root_0_count_)));
  Serial.println(exoplanetData);
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->drawString(50, 0, "EXOPLANETES:");
  Heltec.display->drawString(40, 15, String(root_0_count_)); // Convert int to String
    Heltec.display->display();  
  delay(5000);  // Adjust the delay between updates
}
