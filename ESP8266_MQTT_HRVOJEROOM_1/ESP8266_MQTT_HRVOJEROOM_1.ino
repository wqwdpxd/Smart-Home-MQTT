#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>

#define SW1 13
#define REL1 12
#define DEBOUNCE_TIME 300
#define TOPIC_REL1 "lights/hrvoje/1"
#define TOPIC_SW1 "switches/hrvoje/1"
#define CLIENT_NAME "SONOFF_HRVOJE_1"
#define ON_MSG "on"
#define OFF_MSG "off"
#define PUBLISH_DELAY 50
#define ERROR_DELAY 10

#define DEBUG true


const char* ssid = "PoliNET";
const char* password = "12345678";
const char* mqtt_server = "192.168.1.30";

WiFiClient espClient;
PubSubClient client(espClient);

char msg[50];
long lastTime1=0;
long lastPublish=0;
boolean lastActive1=HIGH;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  dPrintln("");
  dPrint("Connecting to ");
  dPrintln(ssid);

  WiFi.begin(ssid, password);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    dPrint(".");
  }

  dPrintln("");
  dPrintln("WiFi connected");
  dPrintln("IP address: ");
  dPrintln(WiFi.localIP().toString());
}

void callback(char* topic_c, byte* payload_c, unsigned int length) {
  dPrint("Message arrived [");
  dPrint(topic_c);
  dPrint("] ");
  String payload="";
  String topic=topic_c;
  for (int i = 0; i < length; i++) {
    payload+=(char)payload_c[i];
  }
  dPrintln("");
  dPrintln("Processed: " + payload + ", topic:"+topic);

  if(topic.equals(TOPIC_REL1)){
    if(payload.equals(ON_MSG)){
      digitalWrite(REL1,HIGH);
      dPrintln("REL1 on");
    }else if(payload.equals(OFF_MSG)){
      digitalWrite(REL1,LOW);
      dPrintln("REL1 off");
    }
  }
client.loop();  
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    dPrint("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CLIENT_NAME,"dominik","dominik325")) {
      dPrintln("connected");
      // Once connected, publish an announcement...

      client.subscribe(TOPIC_REL1,0);     
    } else {
      dPrint("failed, rc=");
      dPrint((String)client.state());
      dPrintln(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  //digitalWrite(LED_BUILTIN,LOW);
}

void dPrint(String text){
  if(DEBUG) Serial.print(text);
}

void dPrintln(String text){
  if(DEBUG) Serial.println(text);
}

void startOTAServer(){
  ArduinoOTA.setHostname(CLIENT_NAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    dPrintln("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    dPrintln("END OTA!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    dPrintln("Progress: " + (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    dPrintln("Error[%u]: " + error);
    if (error == OTA_AUTH_ERROR) dPrintln("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) dPrintln("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) dPrintln("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) dPrintln("Receive Failed");
    else if (error == OTA_END_ERROR) dPrintln("End Failed");
  });
  ArduinoOTA.begin();
  
}


void setup() {
  //pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  //digitalWrite(LED_BUILTIN, HIGH);
  pinMode(SW1,INPUT);
  pinMode(REL1, OUTPUT);
  if(DEBUG) Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  startOTAServer();
}

void updateSwitches(){
  boolean temp1=digitalRead(SW1);
  long currentTime=millis();
  
  if(temp1!=lastActive1&&currentTime>lastTime1+DEBOUNCE_TIME&&currentTime>lastPublish+PUBLISH_DELAY){
    boolean stateStable=true;
    long startTime=millis();
    while(millis()<startTime+ERROR_DELAY){
      if(temp1!=digitalRead(SW1)) stateStable=false;      
    }
    if(stateStable){
      lastPublish=currentTime;
      lastTime1=currentTime;
      lastActive1=temp1;
      if(lastActive1==HIGH){
        client.publish(TOPIC_SW1,OFF_MSG);
        client.loop();  
        dPrintln("Publish SW1 OFF");
      }else{
        client.publish(TOPIC_SW1,ON_MSG);
        client.loop();  
        dPrintln("Publish SW1 ON");
      }
    }
  }
}


void loop() {

  updateSwitches();
  //debugIt();
  if (!client.connected()) {
    reconnect();
  }
  
  
  client.loop();  
  ArduinoOTA.handle();    

}

void debugIt(){
  //boolean temp1=digitalRead(SW1);
  //boolean temp2=digitalRead(SW2);
  //long currentTime=millis();

  
}
