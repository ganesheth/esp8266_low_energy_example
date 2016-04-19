#include <ESP8266WiFi.h>
#include <ESP8266WiFiGeneric.h>
#include "DHT.h"

#define DHTPIN 2   // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)


// Local Network Settings
char ssid[] = "yourssid";  // your network SSID (name)
char pass[] = "yourpassword";    // your network password
int keyIndex = 0;             // your network key Index number (needed only for WEP)

DHT dht(DHTPIN, DHTTYPE);
int status = WL_IDLE_STATUS;

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
IPAddress thingSpeakIPAddress(54, 164, 214, 198); //54.164.214.198, might try using IP instead of using hostname to reduce DNS resolution. It did not help in my tests.
String APIKey = "your thinkspeak write key";             // enter your channel's Write API Key

//Set your update interval here
const int updateThingSpeakInterval = 20 * 1000; // 20 second interval at which to update ThingSpeak


// Initialize WiFi Client
WiFiClient client;

//Static IP Address helps reduce association and setup time of wifi
IPAddress ip(192, 168, 0, 117);
IPAddress gateway(192, 168, 0, 100);
IPAddress subnet(255, 255, 255, 0);

//To debug time required to start
long setupStartMillis;

void setup() {

  setupStartMillis = millis();
  Serial.begin(115200);
  
  Serial.print(millis());
  Serial.println(":dht begin");
  dht.begin();
    
  Serial.print(millis());
  Serial.print(":connect to wifi ");Serial.println(ssid);

  WiFi.mode(WIFI_STA);  //So that AP is not initialized
  
  if (strcmp (WiFi.SSID().c_str(),ssid) != 0) {
     WiFi.begin(ssid, pass);
  } else {
    Serial.println("using stored data");
    WiFi.begin();
  }

  WiFi.config(ip, gateway, subnet);

  int timeout = millis()+10000;
  while ((WiFi.status() != WL_CONNECTED) && (timeout > millis())) {
    delay(1);
  }
  
  //This might help in reducing current consumption further
  WiFi.setPhyMode(WIFI_PHY_MODE_11B);
  WiFi.setOutputPower(19);

  if ((WiFi.status() != WL_CONNECTED)) {
     Serial.println("WiFi FAILed to connect");
  } else {
     Serial.print("WiFi connected in: "); Serial.print(millis()-setupStartMillis);
     Serial.print(", IP: "); Serial.print(WiFi.localIP());
     Serial.print(", Mac: "); Serial.print(WiFi.macAddress());
     Serial.println();
  }  
  Serial.print(millis());
  Serial.println(":setup done");
}


float h, t;
void measure() {
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
	h = 0;
	t = 0;
    return;
  }
}

void loop() {
    Serial.print(millis());
    Serial.println(":measure");
    measure();
    String temp = String(t);
    String hum = String(h);
    Serial.print(millis());
    Serial.println(":update thingspeak");
    updateThingSpeak("field1=" + temp + "&field2=" + hum);
    Serial.print(millis());
    Serial.println(":sleep");
	
	//Ok, this is the key part. We put the esp to deep sleep.
	//Connect XDP pin (D0) to RST so that the board will wake up
	//On waking up we want WiFi recalibration etc., so use WAKE_RF_DEFAULT
    ESP.deepSleep(20 * 1000000, WAKE_RF_DEFAULT);

}

void updateThingSpeak(String tsData) {

  if (client.connect(thingSpeakAddress, 80)) {
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + APIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);    
  }
}
