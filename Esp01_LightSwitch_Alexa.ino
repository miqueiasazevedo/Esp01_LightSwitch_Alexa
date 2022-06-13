#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>     
#endif

#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <WiFiManager.h>     

#include <Espalexa.h>

#include <ArduinoOTA.h>

#include <PubSubClient.h>

#define relay 0
#define touchIn 2
//#define touchOut 3

bool touchInState = LOW;
bool auxTouchInState = LOW;

//WifiManager

//End WifiManager

//Espalexa
  Espalexa espAlexa;
  void Funcion_relay(uint8_t brightness);

// MQTT Broker
  const char *mqtt_broker = "192.168.0.200";
  const char *topic = "quarto/luz";
  const char *mqtt_username;
  const char *mqtt_password;
  const int mqtt_port = 1884;
  

  WiFiClient espClient;
  PubSubClient client(espClient);

  unsigned long lastChageState = 1000;

  unsigned long lastConnectivityMqttCheckTime = 120000;
  int tryMqttConnect = 0;

  void mqttReconnect() {
    String client_id = "esp8266-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str())) {
        Serial.println("Public emqx mqtt broker connected");
        client.subscribe(topic);
    } else {
        Serial.print("failed with state ");
        Serial.print(client.state());
        delay(2000);
    }
  }
//End MQTT Broker


//=======================================================================
//                               Setup
//=======================================================================
void setup() {
  //Pinmode definitions
    pinMode(relay, OUTPUT);
    digitalWrite(relay, LOW);

    pinMode(touchIn, INPUT);
 
  //WifiManager  
    Serial.begin(115200);
    WiFiManager wifiManager;
    
    wifiManager.autoConnect("LuzQuarto");
    Serial.println("connected...yeey :)");
  //End WifiManager

  //EspAlexa
    espAlexa.addDevice("Luz", Funcion_relay);
    espAlexa.begin();
  //End EspAlexa


  // OTA
  
    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname("LuzQuarto");
    
    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else { // U_FS
        type = "filesystem";
      }
  
      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      Serial.println("Start updating " + type);
    });
    
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });
    
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  //End OTA

  //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

  //publish and subscribe
    client.publish(topic, "off");
    client.subscribe(topic);

} 


//=======================================================================
//                               Loop
//=======================================================================
void loop() {
  
  ArduinoOTA.handle();
  
  //Touch

    if(millis() - lastChageState > 1000 ){

      auxTouchInState = digitalRead(touchIn);
  
      if(auxTouchInState != touchInState){
        digitalWrite(relay, !digitalRead(relay));

        if (client.connected() ) {
          
          if(digitalRead(relay)==LOW){
            client.publish(topic, "off");
          }else{
            client.publish(topic, "on");
          }
          
        }
  
        lastChageState = millis();
        
        touchInState = auxTouchInState;
        
      }
      
    } else{
      touchInState = digitalRead(touchIn);
    }
    
  //End Touch

  //EspAlexa
    espAlexa.loop();
    delay(1);
  //End EspAlexa

  //MQTT
    if(WiFi.status() == WL_CONNECTED)  {
          
      if (!client.connected() ) {
        if(millis() - lastConnectivityMqttCheckTime > 120000 && tryMqttConnect <= 10){
          mqttReconnect();
          tryMqttConnect++;
        }
        if(tryMqttConnect == 10){
          lastConnectivityMqttCheckTime = millis();
          tryMqttConnect = 0;
        }
      }
      
      client.loop();
      
    }else delay(500);
  //End MQTT
 
}

// Função para o pin D3
  void Funcion_relay(uint8_t brightness) {
    if (brightness) {
      digitalWrite(relay, HIGH);
      if (client.connected() ) {
        client.publish(topic, "on");
      }
    }
    else {
      digitalWrite(relay, LOW);
      if (client.connected() ) {
        client.publish(topic, "off");
      }
    }
  }


//Callback function - MQTT Brocker
  void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");

    String messageTemp; 
    
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      messageTemp += (char)payload[i];
    }
    
    if (messageTemp == "off") {
      Serial.println("Luz desligada");
      digitalWrite(relay, LOW);
    }
    else{
      Serial.println("Luz ligada");
      digitalWrite(relay, HIGH);
    }
  }
//End Callback function - MQTT Brocker
