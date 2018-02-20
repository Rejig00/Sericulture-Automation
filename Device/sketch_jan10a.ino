#define temp_low_fan 14                 //D5
#define temp_high_fan 0                 //D3
#define hum_low_fan 12                  //D6
#define hum_high_fan 13                 //D7
#define co2_fan 15                      //D8
#define led_pin 2                       //D4
#define aInput A0
#define MQ135_PULLDOWNRES 10000         //RL
#define default_ppm_CO2 406
#define incubation_temp 25
#define incubation_hum 77
#define instar1_temp 28
#define instar1_hum 87
#define instar2_temp 27
#define instar2_hum 85
#define instar3_temp 26
#define instar3_hum 80
#define instar4_temp 25
#define instar4_hum 73
#define instar5_temp 24
#define instar5_hum 70
//#define spinning_temp 25
//#define spinning_hum 70
#define preservation_temp 25
#define preservation_hum 80

#include <DHTesp.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include<ThingSpeak.h>
#include <Wire.h>
#include <rgb_lcd.h>

//variables
double rs;
double ro_co2;
double rs_ro_ratio_co2;
int co2_ppm;
int adcread;

//wifi
//const char* ssid = "PEACE";
//const char* password = "home0987";

//const char* ssid = "Omi";
//const char* password = "12345678";

//const char* ssid = "IoT";
//const char* password = "support123";

const char* ssid = "DataSoft_WiFi";
const char* password = "support123";


//const char* ssid = "Xproton";
//const char* password = "prio1234";

//mqtt broker
char* topic = "sericulture";
char* server = "192.168.4.144";
const char* mqtt_client = "ESP8266Client"; // MQTT Client Name.
const char* mqtt_user = "Nafiur"; // MQTT Broker user.
const char* mqtt_pass = "nafiur789"; // MQTT Broker password.

//thigspeak
unsigned long myChannelNumber = 377691;  
const char* myWriteAPIKey = "IIA4PI03GN2TF3HV";

//DHT
int t;
int h;

String currentState;
unsigned long previousMillis = 0;
unsigned long currentMillis;  
const int interval1 = 40;
const int interval2 = 20;  



DHTesp dht;
rgb_lcd lcd;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //pinMode(aInput , INPUT);
  pinMode(14 , OUTPUT);
  pinMode(12 , OUTPUT);
  pinMode(13 , OUTPUT);
  pinMode(15 , OUTPUT);
  pinMode(2 , OUTPUT);
  dht.setup(16);
  lcd.begin(16, 2);
  lcd.setRGB(0, 255, 0);
  ThingSpeak.begin(espClient);
  WiFi.mode(WIFI_STA);                  //Put the NodeMCU in Station mode.
  delay(10000);
  WiFi.disconnect();                    //Disconnect from previous connections.
  Serial.println("Scan Start");
  int n = WiFi.scanNetworks();          //Scan the number of available networks
  if(n==0){
    Serial.println("No Networks Found"); 
  }
  else{
    Serial.print(n);
    Serial.println(" Networks Found");
  }
  Serial.println("List of surrounding Network SSIDs…:");    
  for (int i = 0; i < n; i++)
  {
    Serial.print(i+1);
    Serial.print(".");
    Serial.println(WiFi.SSID(i));
  }
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  while(WiFi.status() != WL_CONNECTED) {  // while being connected
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to: ");
  Serial.println(WiFi.SSID());
  //Serial.print(". IP Address: ");
  //Serial.println(WiFi.localIP());
  //Serial.print("Current Signal Strength: ");
  //Serial.println(WiFi.RSSI());
  
  client.setServer(server, 1883);
  //client.setCallback(callback);
  adcread = analogRead(aInput);
  //Serial.println(adcread);
  rs = (double)((double)(1024*(long)MQ135_PULLDOWNRES)/adcread-(long)MQ135_PULLDOWNRES);
  ro_co2 = (double)rs/(5.1396*pow(default_ppm_CO2,(-.347)));
  digitalWrite(led_pin , HIGH);
  h = dht.getHumidity();
  t = dht.getTemperature();
  rs_ro_ratio_co2 = (double)rs/(double)ro_co2;
  co2_ppm = exp((log(rs_ro_ratio_co2/5.1396)/(-.347)));
}

int timeSinceLastRead = 0;
int realTime = 0;

void loop() {
  const int BUFFER_SIZE = JSON_OBJECT_SIZE(3);
  StaticJsonBuffer<BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String JSON;
  
  while (WiFi.status() != WL_CONNECTED) {  // FIX FOR USING 2.3.0 CORE (only .begin if not connected)
    Serial.println("Reconnecting WiFi...");
    WiFi.begin(ssid, password);       // connect to the network
  }  
  if (!client.connected()) {
    reconnect();
  }


  if(timeSinceLastRead > 5000) {
    h = dht.getHumidity();
    t = dht.getTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }
  adcread = analogRead(aInput);
  rs = (double)((double)(1024*(long)MQ135_PULLDOWNRES)/adcread-(long)MQ135_PULLDOWNRES);
  rs_ro_ratio_co2 = (double)rs/(double)ro_co2;
  co2_ppm = exp((log(rs_ro_ratio_co2/5.1396)/(-.347)));
  Serial.print("CO2_PPM:  ");
  Serial.print(co2_ppm);
  Serial.print("\tHumidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t);
  lcd.print("Hum: ");
  lcd.print(h);
  lcd.setCursor(0, 1);
  lcd.print("CO2: ");
  lcd.print(co2_ppm);
  timeSinceLastRead = 0;
  root["Temperature"] = t;
  root["Humidity"] = h;
  root["CO2"] = co2_ppm;
  root.prettyPrintTo(JSON);
  char attributes[100];
  JSON.toCharArray( attributes, 100 );
  client.publish( "sericulture", attributes );
  Serial.println(attributes);
  //ThingSpeak.setField(1,t);
  //ThingSpeak.setField(2,h);
  //ThingSpeak.setField(3,co2_ppm);
  //ThingSpeak.writeFields(myChannelNumber,myWriteAPIKey);
  }

  delay(1000);
  timeSinceLastRead += 1000;
  realTime += 1;
  JSON = "";
  
  if(realTime < (14*60)){
    incubation();
    currentState = "Incubation";
    Serial.println(currentState);
  }
  else if(realTime < (18*60) && realTime > (14*60)){
    instar1();
    currentState = "Instar 1";
    Serial.println(currentState);
  }
  else if(realTime < (21*60) && realTime > (18*60)){
    instar2();
    currentState = "Instar 2";
    Serial.println(currentState);
  }
  else if(realTime < (25*60) && realTime > (21*60)){
    instar3();
    currentState = "Instar 3";
    Serial.println(currentState);
  }
  else if(realTime < (31*60) && realTime > (25*60)){
    instar4();
    currentState = "Instar 4";
    Serial.println(currentState);
  }
  else if(realTime < (39*60) && realTime > (31*60)){
    instar5();
    currentState = "Instar 5";
    Serial.println(currentState);
  }
  else if(realTime > (39*60)){
    preservation();
    currentState = "Preservation";
    Serial.println(currentState);
  }
  if(co2_ppm > 1000){
     digitalWrite(co2_fan , HIGH); 
  }
  if(co2_ppm < 800){
    digitalWrite(co2_fan , LOW); 
  }
  client.loop();
}


void incubation(){
  if(t > incubation_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= incubation_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == incubation_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > incubation_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= incubation_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == incubation_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  young_light();
}

void instar1(){
  if(t > instar1_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= instar1_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == instar1_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > instar1_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= instar1_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == instar1_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  young_light();
}

void instar2(){
  if(t > instar2_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= instar2_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == instar2_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > instar2_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= instar2_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == instar2_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  young_light();
}

void instar3(){
  digitalWrite(led_pin , LOW);
  if(t > instar3_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= instar3_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == instar3_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > instar3_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= instar3_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == instar3_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  old_light();
}

void instar4(){
  if(t > instar4_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= instar4_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == instar4_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > instar4_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= instar4_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == instar4_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  old_light();
}

void instar5(){
  if(t > instar5_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= instar5_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == instar5_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > instar5_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= instar5_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == instar5_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  old_light();
}

void preservation(){
  if(t > preservation_temp){
    digitalWrite(temp_low_fan , HIGH);
    digitalWrite(temp_high_fan , LOW);
  }
  else if(t <= preservation_temp){
    digitalWrite(temp_low_fan , LOW);
    digitalWrite(temp_high_fan , HIGH);
    if(h == preservation_temp){
        digitalWrite(temp_high_fan , LOW);
    }
  }
  if(h > preservation_hum){
    digitalWrite(hum_low_fan , HIGH);
    digitalWrite(hum_high_fan , LOW);
  }
  else if(h <= preservation_hum){
    digitalWrite(hum_low_fan , LOW);
    digitalWrite(hum_high_fan , HIGH);
    if(h == preservation_hum){
        digitalWrite(hum_high_fan , LOW);
    }
  }
  old_light();
}

void young_light(){
  currentMillis = realTime;
  if(currentMillis - previousMillis >= interval2) {
    digitalWrite(led_pin , LOW);
  }
  if(currentMillis - previousMillis >= (interval2 + interval1)){
      digitalWrite(led_pin , HIGH);
      previousMillis = currentMillis;
  } 
}

void old_light(){
  currentMillis = realTime;
  if(currentMillis - previousMillis >= interval2) {
    digitalWrite(led_pin , HIGH);
  }
  if(currentMillis - previousMillis >= (interval2 + interval1)){
      digitalWrite(led_pin , LOW);
      previousMillis = currentMillis;
  } 
}



void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client,mqtt_user,mqtt_pass)) {
      Serial.println("connected");   
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(500);
    }
  }
}
