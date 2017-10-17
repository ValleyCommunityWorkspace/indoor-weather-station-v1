
// #define MOONBASE_BOARD
//#define DHT_BOARD
#define SHT30_BOARD
/* WeMos DHT Server
 *
 * Connect to WiFi and respond to http requests with temperature and humidity
 *
 * Based on Adafruit ESP8266 Temperature / Humidity Webserver
 * https://learn.adafruit.com/esp8266-temperature-slash-humidity-webserver
 *
 * Depends on Adafruit DHT Arduino library
 * https://github.com/adafruit/DHT-sensor-library
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "CaptiveConfig.h"
#include "Button.h"
#include "SequenceLED.h"
#include "UrlEncode.h"

#ifdef MOONBASE_BOARD
#include <Wire.h>
#include <HTS221.h>
#include <LPS25H.h>
#include "PC8563.h" 
#endif

#ifdef  DHT_BOARD
#include <DHT.h>
#define DHTTYPE DHT22   // DHT Shield uses DHT 11
#define DHTPIN D4       // DHT Shield uses pin D4
#endif

#ifdef  SHT30_BOARD
#include <WEMOS_SHT3X.h>

SHT3X sht30(0x45);
#endif

//extern const char* DEVNAME; extern const char* ISSUEID; extern const char* ssid; extern const char* password;

//#include <list>
//extern std::list<WiFiEventHandler> sCbEventList;   // From ESP8266WiFiGeneric.cpp

IPAddress server(120,138,27,109);
bool humidityPresent, pressurePresent;


#ifdef  DHT_BOARD
// Initialize DHT sensor
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);
#endif

float pressure;         // Raw float values from the sensor
char str_pressure[10];  // Rounded sensor values and as strings
float humidity, temperature;                 // Raw float values from the sensor
char str_humidity[10], str_temperature[10];  // Rounded sensor values and as strings

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;            // When the sensor was last read
const long interval = 5000;                  // Wait this long until reading again
unsigned long lastConnectionTime = 0;             // last time you connected to the server, in milliseconds
enum WiFiStateEnum { DOWN, STARTING, UP, CONFIG }    ;
WiFiStateEnum WiFiState = DOWN;
unsigned long WiFiStartTime = 0;
int secs_waiting=0;
int need_WickedNetworksPost=0;
bool WiFiEventEnable = true;

char saved_ssid[255] = "";        /// replaced by user in ConfigAP
char saved_password[255] = "";  /// replaced by user in ConfigAP
char saved_email[255] = "<email>";  /// replaced by user in ConfigAP
char saved_location[255] = "<location>";  /// replaced by user in ConfigAP
char DEVNAME[255] = "VCW100";
char ISSUEID[255]  = "ZGKL01";

void read_sensor() {
  // Wait at least 2 seconds seconds between measurements.
  // If the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor.
  // Works better than delay for things happening elsewhere also.
  unsigned long currentMillis = millis();

  //if (currentMillis - previousMillis >= interval) {
  // // Save the last time you read the sensor
  //  previousMillis = currentMillis;

#ifdef MOONBASE_BOARD
    pc_time tm;
    if (!PC8563_RTC.read(tm)) {
      Serial.println("Failed to read from RTC!");
    } else {
      Serial.print("Time: ");
      Serial.print(tm.hour);
      Serial.print(":");
      Serial.print(tm.minute);
      Serial.print(":");
      Serial.print(tm.second);
      Serial.print(" ");
      Serial.print(tm.day);
      Serial.print("/");
      Serial.print(tm.month);
      Serial.print("/");
      Serial.println(tm.year);
    }
    if (humidityPresent) {
      humidity = smeHumidity.readHumidity();
      temperature = smeHumidity.readTemperature();
    }
    if (pressurePresent)
      pressure = smePressure.readPressure();
#endif
#ifdef  DHT_BOARD
    // Reading temperature and humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();        // Read humidity as a percent
    temperature = dht.readTemperature();  // Read temperature as Celsius

#endif

#ifdef SHT30_BOARD
     //static SHT3X sht30(0x45);

     byte ret = 0;
     if((ret = sht30.get()) == 0) {  // 550 ms
      humidity = sht30.humidity;
      temperature = sht30.cTemp;
     } else {
      Serial.print("SHT30 Read Error:");
      Serial.println(ret);
       humidity = 0.0;
       temperature = 0.0;
     }
#endif


    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from sensors!");
      return;
    }

    // Convert the floats to strings and round to 2 decimal places

    if (pressurePresent) {
      dtostrf(pressure, 1, 2, str_pressure);
      Serial.print("Pressure: ");
      Serial.print(str_pressure);
      Serial.print(" mBAR\t");
    }
    if (humidityPresent) {
      dtostrf(humidity, 1, 2, str_humidity);
      dtostrf(temperature, 1, 2, str_temperature);
      Serial.print("Humidity: ");
      Serial.print(str_humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.print(str_temperature);
      Serial.println(" °C");
    }
  // }
}

void WiFiEvent(WiFiEvent_t event) {
    if (WiFiEventEnable) {
      Serial.printf("[WiFi-event] event: %d\n", event);
  
      switch(event) {
          case WIFI_EVENT_STAMODE_GOT_IP:
              Serial.println("WiFi connected");
              Serial.println("IP address: ");
              Serial.println(WiFi.localIP());
              WiFiState = UP;
              secs_waiting = 60;  // skip ahead
              break;
          case WIFI_EVENT_STAMODE_DISCONNECTED:
              WiFiState = DOWN;
              Serial.println("WiFi lost connection");
              break;
      }
    }
}

void WifiInitialTry() {

    WiFi.enableSTA(false);
    delay(250);
    WiFi.mode(WIFI_STA); 
    delay(250);
    
    WiFi.begin(WiFi.SSID().c_str(),WiFi.psk().c_str());
    Serial.println("Try Initial Wifi... ");
    Serial.println(WiFi.SSID());
    Serial.println(WiFi.psk());
}


//void WifiTryUp(const char* ssid, const char* password) {
//    // delete old config
//
//    WiFi.onEvent(WiFiEvent);
//
//    WiFi.disconnect(true);
//
//    delay(250);  // was 1000
//
//    WiFi.begin(ssid, password);
//
//    Serial.println();
//    Serial.println();
//    Serial.println("Wait for WiFi... ");
//    Serial.println(ssid);
//    Serial.println(password);
//
//}

unsigned long _ESP_id;
void setup(void)
{
  // Open the Arduino IDE Serial Monitor to see what the code is doing
  Serial.begin(115200);
  Serial.println("");

#ifdef MOONBASE_BOARD
    Serial.println("VCW sensor Server");
    Wire.begin();
    pressurePresent= smePressure.begin();
    if (!pressurePresent)
        Serial.println("- NO LPS25 Pressure Sensor found");
    humidityPresent = smeHumidity.begin();
    if (!humidityPresent)
        Serial.println("- NO HT221 Temperature/Humidity Sensor found");
    if (!PC8563_RTC.begin())
      Serial.println("- NO PC8563 RTC found");
    pc_time tm;
    tm.year=2016;
    tm.month=8;
    tm.day=12;
    tm.hour=21;
    tm.minute=12;
    tm.second=30;
    PC8563_RTC.write(tm);
    

#endif
#ifdef  DHT_BOARD
  humidityPresent = 1;
  pressurePresent = 0;
  dht.begin();
  Serial.println("WeMos DHT Server");
#endif

#ifdef  SHT30_BOARD
  humidityPresent = 1;
  pressurePresent = 0;
  Serial.println("WeMos SHT30 Server");
#endif

  _ESP_id = ESP.getChipId();  // uint32 -> unsigned long on arduino
  Serial.println(_ESP_id,HEX);
  Serial.println("");
  // Initial read
  // read_sensor();
  secs_waiting = 50;   // initially fall through everything!
  
  //WiFi.persistent(false);
  
}


void loop(void)
{

    static CaptiveConfig *captiveConfig(nullptr);
    static bool haveConfig(true);

    static unsigned long loop_previouMillis = millis();    // run once
    unsigned long loop_currentMillis = millis();           // run every loop

    static Button buttonD3(D3);
    static SequenceLED ledD4(D4,HIGH);  // Off

    static unsigned long WiFiLastUPTime = millis();
    static bool newConnection = true;

    static bool need_Register = false;

    // RUN ONCE INITIAL SETUP
    static bool needRegisterWiFiCallback(true);
    if (needRegisterWiFiCallback) {
        WiFi.onEvent(WiFiEvent);
        needRegisterWiFiCallback = false;
    }


    ledD4.update();  // in loop.

    if (buttonD3.pushed(4000)) {
      Serial.println("Config Mode button pushed!");
      ledD4.startSequence("0101",500,true);
      haveConfig = false;
      WiFiState = CONFIG; 
      WiFiEventEnable = false;
    }

    delay(10);

    if (loop_currentMillis - loop_previouMillis <= 1000) {
      return;    /// NEXT loop()
    } else {
        loop_previouMillis = loop_currentMillis;
      /// Fall through to next, once-per second code.
    }

    /// ONCE PER SECOND CODE::
    
      
      if (!haveConfig) {
          if (captiveConfig == nullptr) {
              WiFi.mode(WIFI_AP_STA);          
              delay(100);
              WiFiState = CONFIG;
              newConnection = true;
              captiveConfig = new CaptiveConfig;
          }
          Serial.println("haveConfig...");
          if (captiveConfig->haveConfig()) {
  
              // TODO: Store the config somewhere useful
              auto desiredConfig(captiveConfig->getConfig());
              Serial.print("Got configuration!  SSID: ");
              Serial.print(desiredConfig.ssid);
              Serial.print(" passphrase: ");
              Serial.println(desiredConfig.passphrase);
              Serial.print(" email: ");
              Serial.println(desiredConfig.email);
              Serial.print(" location: ");
              Serial.println(desiredConfig.location);

              desiredConfig.ssid.toCharArray(saved_ssid, 254);
              desiredConfig.passphrase.toCharArray(saved_password,254);
              desiredConfig.email.toCharArray(saved_email, 254);
              desiredConfig.location.toCharArray(saved_location,254);
  
              haveConfig = true;
              secs_waiting = 60;  // jump ahead

              need_Register = true;
              
              delete captiveConfig;
              captiveConfig = nullptr;

              WiFi.mode(WIFI_STA); 
              WiFi.persistent(true);
              WiFiEventEnable = true;
              WiFiState = STARTING;
              WiFi.begin(saved_ssid,saved_password);
              WiFi.persistent(false);
       
              
              ledD4.stopSequence();
          }
      }
    

   // Connect to your WiFi network 
    if (WiFiState == DOWN) {
      WifiInitialTry();
      WiFiState = STARTING;
      WiFiStartTime = millis();
    }
    if (WiFiState == STARTING) {
      if (millis() - WiFiStartTime > 120000) { 
        WiFiState = DOWN; 
        Serial.println("WiFi Connect Timeout"); 
        ledD4.startSequence("101010111111",100,true);
        }
    }

    if (WiFiState == UP) {
      WiFiLastUPTime = millis();
    }

    if (need_Register & WiFiState == UP) {
      if(httpsRegisterAndRedir(_ESP_id,saved_email,saved_location)) { //uint32, const char*, const char*
        need_Register = false;
      }
    }


    if (millis() - WiFiLastUPTime > 20000 && WiFiState != CONFIG ) { 
        Serial.println("WiFi Not Up for 20 seconds"); 
        ledD4.startSequence("101010111111111111",50,false);
    }

    /// ONCE PER 60 SECONDS CODE:

  if (secs_waiting < 59) {
    secs_waiting++;
    return;
  }
  secs_waiting=0;
  

 


    if (WiFiState == UP) { 
      if(httpsRequestAndRedir(temperature,humidity, pressure)) {
         ledD4.stopSequence();
      } else {
        Serial.println("connection failed LED");
        ledD4.startSequence("101011111111111111",50,true);
      }
    }

 

  if (need_WickedNetworksPost == 1) {
    httpsPOSTWickedNetworksHotspotLogin();
    need_WickedNetworksPost = 0;
  }

}


bool httpsRequestAndRedir(float temp, float humid, float pressure) {

  bool retvalue = false;

  read_sensor();

  retvalue = httpsRequest(temperature,humidity, pressure);

  if (need_WickedNetworksPost == 1) {
      delay(1000);
      Serial.println("Doing Wicked Network Hotspot Login");
      httpsPOSTWickedNetworksHotspotLogin();
      need_WickedNetworksPost = 0;
      Serial.println("Re-requesting sensor measurements");
      delay(250);
      return(httpsRequest( temp,  humid, pressure ));

  } else {
  
      return(retvalue);
  }

}


// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";

// this method makes a HTTP connection to the server:
bool httpsRequest(float temp, float humid, float pressure) {

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting HTTPS ");
  Serial.println(server);
  if (!client.connect(server, 443)) {
    Serial.println("connection failed");
    return false;
  }

  if (client.verify(fingerprint, "www.shac.org.nz")) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

 // We now create a URI for the request
   String url = "/user/monitor/";
  url += String(_ESP_id,HEX);
  url += "/reading/?";
  if (humidityPresent) {
    url += "&temp=";
    url += temp;
    url += "&humid=";
    url += humid;
  } 
  if (pressurePresent) {
    url += "&indoorpressure=";
    url += pressure;
  }
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + "www.shac.org.nz" + "\r\n" + 
               "User-Agent: indoor-wx-v0.1" + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");

  //delay(100);

  unsigned long _timeout = 1000;
  unsigned long _startMillis = millis();
  String httpCode = "";
  //while (client.connected()) {     // client.connected seems to not always work! - returns FALSE at this point sometimes.
  
  while (millis() - _startMillis < _timeout) {
    String line = client.readStringUntil('\n');
    yield();
    Serial.print("Header Line: ");
    Serial.println(line);

    
    if (line.startsWith("HTTP/1.1")) {
        httpCode = line.substring(9,13);
    }

    // SPECIAL - DETECT VCW WICKED NETWORKS LOGIN REDIRECTION PAGE
    if (line.indexOf("Location: https://secure.wickednetworks.co.nz/login") > -1 ) {
          Serial.println("SPECIAL WICKED NETWORKS REDIR");
          // set var
          need_WickedNetworksPost = 1;
    }
    
    if (line == String("\r") ) {
      Serial.println("headers received");
      break;
    }
  }
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    yield();
    if (line.startsWith("GMT:")) {
      Serial.print("Got GMT: ");
      Serial.println(line);
    } else {
      Serial.print("Body Line: ");
      Serial.println(line);
    }
  }


  Serial.print("Finished: ");
  Serial.println(httpCode);


  return(true);

}

bool httpsRegisterAndRedir(uint32 _ESP_id,const char * saved_email, const char * saved_location) { 

  bool retvalue = false;

  retvalue = httpsRegister( _ESP_id, saved_email, saved_location);

  if (need_WickedNetworksPost == 1) {
    delay(1000);
    Serial.println("Register: Doing Wicked Network Hotspot Login");
    httpsPOSTWickedNetworksHotspotLogin();
    need_WickedNetworksPost = 0;
    Serial.println("Register: Re-requesting registration");
    delay(250);
    return(httpsRegister( _ESP_id, saved_email, saved_location));
  } else {

    return(retvalue);
  }
  
}


bool httpsRegister(uint32 _ESP_id,const char * saved_email, const char * saved_location) { 


  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting HTTPS ");
  Serial.println(server);
  if (!client.connect(server, 443)) {
    Serial.println("connection failed");
    return false;
  }

  if (client.verify(fingerprint, "www.shac.org.nz")) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

 // We now create a URI for the request
  String url = "/user/monitor/";
  url += String(_ESP_id,HEX);
  url += "/register/?";
  url += "&email=";
  url += urlencode(saved_email);
  url += "&location=";
  url += urlencode(saved_location);
 
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + "www.shac.org.nz" + "\r\n" + 
               "User-Agent: indoor-wx-v0.1" + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");

  //delay(100);

  unsigned long _timeout = 1000;
  unsigned long _startMillis = millis();
  String httpCode = "";
  //while (client.connected()) {     // client.connected seems to not always work! - returns FALSE at this point sometimes.
  
  while (millis() - _startMillis < _timeout) {
    String line = client.readStringUntil('\n');
    yield();
    Serial.print("Header Line: ");
    Serial.println(line);

    
    if (line.startsWith("HTTP/1.1")) {
        httpCode = line.substring(9,13);
    }

    // SPECIAL - DETECT VCW WICKED NETWORKS LOGIN REDIRECTION PAGE
    if (line.indexOf("Location: https://secure.wickednetworks.co.nz/login") > -1 ) {
          Serial.println("SPECIAL WICKED NETWORKS REDIR");
          // set var
          need_WickedNetworksPost = 1;
    }
    
    if (line == String("\r") ) {
      Serial.println("headers received");
      break;
    }
  }
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    yield();
    if (line.startsWith("GMT:")) {
      Serial.print("Got GMT: ");
      Serial.println(line);
    } else {
      Serial.print("Body Line: ");
      Serial.println(line);
    }
  }


  Serial.print("Finished: ");
  Serial.println(httpCode);



  

  return(true);
  
}





void httpsPOSTWickedNetworksHotspotLogin() {

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting HTTPS ");

  const char* server = "secure.wickednetworks.co.nz";
  Serial.println(server);
  if (!client.connect(server, 443)) {
    Serial.println("connection failed");
    return;
  }

  if (client.verify(fingerprint, "www.shac.org.nz")) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate doesn't match");
  }

   String payload = "free_subscription=&password=uZC7FUX8&username=dev_subscription&dst=index.php%3Faction%3Dwelcome&x=109&y=113";
   //String payload = "free_subscription&password=uZC7FUX8&username=dev_subscription";
   client.println("POST /login HTTP/1.1");
  client.println("Host: secure.wickednetworks.co.nz");
  client.println("Cache-Control: no-cache");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.println(payload);


  unsigned long _timeout = 1000;
  unsigned long _startMillis = millis();
  //while (client.connected()) {     // client.connected seems to not always work! - returns FALSE at this point sometimes.
  
  while (millis() - _startMillis < _timeout) {
    String line = client.readStringUntil('\n');
    yield();
    Serial.print("Header Line: ");
    Serial.println(line);
    
    if (line == String("\r") ) {
      Serial.println("headers received");
      break;
    }
  }
  
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    yield();
    if (line.startsWith("GMT:")) {
      Serial.print("Got GMT: ");
      Serial.println(line);
    } else {
      Serial.print("Body Line: ");
      Serial.println(line);
    }
  }
               


}

