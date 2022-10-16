#include "iotSecrets.h"



void delay_with_client_loop(unsigned long ms) {
  unsigned long start = millis();
  while((millis() - start) < ms) {
    client.loop();
    //Serial.println("AWS looping");
  }
  //Serial.println("AWS loop end");
}

void connectWiFi() {
  if(WiFi.status() == WL_CONNECTED) {
    return;
  }
  
  WiFi.begin();
  Serial.println("Connecting to Wi-Fi");


  int wifi_connecting = 0;
  while ((WiFi.status() != WL_CONNECTED) && (wifi_connecting <= WIFI_TIMEOUT_MS)){
    delay(500);
    wifi_connecting += 500;
    Serial.print(".");
  }
  if(WiFi.status() != WL_CONNECTED) {    
    Serial.println("Wi-Fi Timeout");
    WiFi.disconnect();

    delay(30000);
    ESP.restart();
    return;
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  return;
}

void connectAWS() {
  if(client.connected()){
    return;
  }
  
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  client.setTimeout(1000);

  Serial.println("Connecting to AWS IOT");

  int client_connecting = 0;
  //while ((!client.connect(THINGNAME)) && (client_connecting <= MQTT_TIMEOUT_MS)) {
  while ((!client.connect(gSerialName.c_str())) && (client_connecting <= MQTT_TIMEOUT_MS)) {
    Serial.print(".");
    delay(100);
    client_connecting += 100;
  }
  if(!client.connected()){
    WiFi.disconnect(true, true);
    
    Serial.println("AWS IoT Timeout!");

    delay(30000);
    ESP.restart();
    return;
  }

  Serial.println("AWS IoT Connected!");
  
  return;
}

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["THINGNAME"] = gSerialName.c_str();
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["particle"] = particle;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  bool published = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  if(published) {
    Serial.println("Publish Success");
  }
  else {
    Serial.println("Publish fail");
  }
}


void publishMessage_alart(int alart_type,int onoff_event)
{
  StaticJsonDocument<200> doc;
  doc["THINGNAME"] = gSerialName.c_str();

  doc["SIGNAL_NAME"] = gSigName.c_str();

  if (alart_type == ALART_SIG)
  {
    doc["alart_type"] = "ALART_SIG";
  }
  else if (alart_type == STATUS_SIG)
  {
    doc["alart_type"] = "STATUS_SIG";
  }
  
  if (onoff_event == EVENT_ON)
  {
    doc["onoff_event"] = "EVENT_ON";
  }
  else if (onoff_event == EVENT_OFF)
  {
    doc["onoff_event"] = "EVENT_OFF";
  }
  

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  bool published = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  if(published) {
    Serial.println("Publish Success");
  }
  else {
    Serial.println("Publish fail");
  }
}




void setup_AWSIotMqtt() {

    //WiFi.mode(WIFI_STA);
    //WiFi.setAutoConnect(false);
    
    // Configure WiFiClientSecure to use the AWS IoT device credentials
    net.setCACert(AWS_CERT_CA);
    net.setCertificate(AWS_CERT_CRT);
    net.setPrivateKey(AWS_CERT_PRIVATE);
}

void TASK_AWSIotMqtt(void *args) {

  //delay(5000);

  #if 1
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(5000);
  }
  setup_AWSIotMqtt();
  while(1) {
    //connectWiFi();
    //connectAWS();
    //publishMessage();
    //showLcd();
    //delay_with_client_loop(60 * 1000);
    delay(100);
  }
  #endif

  #if 0
  while(1)
  {
    Serial.printf("%s - run\r\n",__func__);
    delay(9000);
  }
  #endif
    
}

