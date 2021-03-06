#define MOONBASE_BOARD
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


#ifdef MOONBASE_BOARD
#include <Wire.h>
#include <HTS221.h>
#include <LPS25H.h>
#include "PC8563.h"

#else
#include <DHT.h>
#define DHTTYPE DHT22   // DHT Shield uses DHT 11
#define DHTPIN D4       // DHT Shield uses pin D4
#endif


//const char* DEVNAME = "VCW100” ; const char* ISSUEID  = ZGKL01“”; const char* ssid = ""; const char* password = “”;
extern const char* DEVNAME; extern const char* ISSUEID; extern const char* ssid; extern const char* password;

void httpsRequest(float temp, float humid, float pressure);

IPAddress server(120,138,27,109);

bool humidityPresent, pressurePresent;

#ifndef MOONBASE_BOARD
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
enum WiFiStateEnum { DOWN, STARTING, UP }    ;
WiFiStateEnum WiFiState = DOWN;
unsigned long WiFiStartTime = 0;
int secs_waiting=0;

void read_sensor() {
  // Wait at least 2 seconds seconds between measurements.
  // If the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor.
  // Works better than delay for things happening elsewhere also.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // Save the last time you read the sensor
    previousMillis = currentMillis;

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
#else
    // Reading temperature and humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();        // Read humidity as a percent
    temperature = dht.readTemperature();  // Read temperature as Celsius

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from sensors!");
      return;
    }
#endif

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
  }
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch(event) {
        case WIFI_EVENT_STAMODE_GOT_IP:
            Serial.println("WiFi connected");
            Serial.println("IP address: ");
            Serial.println(WiFi.localIP());
            WiFiState = UP;
            break;
        case WIFI_EVENT_STAMODE_DISCONNECTED:
            WiFiState = DOWN;
            Serial.println("WiFi lost connection");
            break;
    }
}

void WifiTryUp() {
    // delete old config

    WiFi.disconnect(true);

    delay(1000);



    WiFi.begin(ssid, password);

    Serial.println();
    Serial.println();
    Serial.println("Wait for WiFi... ");

}

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
    

#else
  humidityPresent = 1;
  pressurePresent = 0;
  dht.begin();
  Serial.println("WeMos DHT Server");
#endif

  _ESP_id = ESP.getChipId();  // uint32 -> unsigned long on arduino
  Serial.println(_ESP_id,HEX);
  Serial.println("");
  WiFi.onEvent(WiFiEvent);
  // Initial read
  read_sensor();
  secs_waiting = 0;
}

void loop(void)
{
  if (secs_waiting < 59) {
    delay(1000);
    secs_waiting++;
    return;
  }
  secs_waiting=0;
  read_sensor();
  
  if (WiFiState == UP) 
  	{ httpsRequest(temperature,humidity, pressure); }

  // Connect to your WiFi network
  if (WiFiState == DOWN) {
    WifiTryUp();
    WiFiState = STARTING;
    WiFiStartTime = millis();
  }
  if (WiFiState == STARTING) {
    if (millis() - WiFiStartTime > 120000) { WiFiState = DOWN; Serial.println("WiFi Connect Timeout"); }
  }


}




// this method makes a HTTP connection to the server:
void httpRequest(float temp, float humid, float pressure) {

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(server, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  String url = "/updateweatherstation.php?action=updateraw";
  url += "&devid=";
  url += String(_ESP_id,HEX);
  url += "&devname=";
  url += DEVNAME;
  url += "&issueid=";
  url += ISSUEID;
  url += "&dateutc=now";
  if (humidityPresent) {
    url += "&indoortemp=";
    url += temp;
    url += "&indoorhumidity=";
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
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    delay(10);
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection"); 

    // note the time that the connection was made:
  lastConnectionTime = millis();
 
}



// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";

// this method makes a HTTP connection to the server:
void httpsRequest(float temp, float humid, float pressure) {

  // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  Serial.print("connecting HTTPS ");
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

 // We now create a URI for the request
  String url = "/updateweatherstation.php?action=updateraw";
  url += "&devid=";
  url += String(_ESP_id,HEX);
  url += "&devname=";
  url += DEVNAME;
  url += "&issueid=";
  url += ISSUEID;
  url += "&dateutc=now";
  if (humidityPresent) {
    url += "&indoortemp=";
    url += temp;
    url += "&indoorhumidity=";
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

/*   
  Serial.print("01"); Serial.println(client.readStringUntil('\n'));
  Serial.print("02"); Serial.println(client.readStringUntil('\n'));
  Serial.print("03"); Serial.println(client.readStringUntil('\n'));
  Serial.print("04"); Serial.println(client.readStringUntil('\n'));
  Serial.print("05"); Serial.println(client.readStringUntil('\n'));
  Serial.print("06"); Serial.println(client.readStringUntil('\n'));
  Serial.print("07"); Serial.println(client.readStringUntil('\n'));
  Serial.print("08"); Serial.println(client.readStringUntil('\n'));
  Serial.print("09"); Serial.println(client.readStringUntil('\n'));
  Serial.print("10"); Serial.println(client.readStringUntil('\n'));
  Serial.print("11"); Serial.println(client.readStringUntil('\n'));
  Serial.print("12"); Serial.println(client.readStringUntil('\n'));
  Serial.print("13"); Serial.println(client.readStringUntil('\n'));
  Serial.print("14"); Serial.println(client.readStringUntil('\n'));
  Serial.print("15"); Serial.println(client.readStringUntil('\n'));
  Serial.print("16"); Serial.println(client.readStringUntil('\n'));
*/     



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


  Serial.println("Finished");
  // read until return -1?????
  //while (client.read() >= 0) {Serial.print("."); }
  
  //Serial.println("reply was:");
  //Serial.println("==========");
  //Serial.println(line);
  //Serial.println("==========");
  //Serial.println("closing connection");


}

