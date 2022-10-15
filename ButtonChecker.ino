//#include <WiFi.h>          //https://github.com/esp8266/Arduino
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include "FS.h"
#include "SPIFFS.h"	// ①ライブラリを読み込み

#include <DS3232RTC.h>
#include "iotSecrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>



#define FORMAT_SPIFFS_IF_FAILED true


#define Button_D0   D0
#define Button_D1   D1
#define Button_D2  D2
#define Button_D3   D3

#define OPEN 0
#define CLOSE 1

#define INPUT_CHAT_TIME  300

#define ALART_SIG 0
#define STATUS_SIG 1

#define EVENT_OFF 0
#define EVENT_ON 1

DS3232RTC myRTC(false);

char s2[80];//文字格納用
char time_message[20];//文字格納用

//struct tm timeInfo;//時刻を格納するオブジェクト

int gButton_D0_State = CLOSE;
int gButton_D1_State = CLOSE;
int gButton_D2_State = CLOSE;
int gButton_D3_State = CLOSE;

int gRecieveCount = 0;


#define FORMAT_SPIFFS_IF_FAILED true

String gSerialName;
String gSigName;

const char* AWS_IOT_PUBLISH_TOPIC = "node/airsensor";

const int WIFI_TIMEOUT_MS = 20000;
const int MQTT_TIMEOUT_MS = 5000;

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
//CRC8 crc;
//ST7032_asukiaaa lcd;

double temperature;
double humidity;
uint32_t particle;


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


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");

    while(file.available()){
        Serial.write(file.read());
        //Serial.println(file.read());
    }
    file.close();
}




void SerialNameReadFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    gSerialName = file.readStringUntil('\n');
    gSerialName.trim();
    Serial.println(gSerialName);

    #if 0
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    char s[30];//文字格納用
    int i=0;
    while(file.available()){
        gSerialName[i++] = file.read();


        //Serial.write(file.read());
        //Serial.println(file.read());
    }
    Serial.write(s);
    #endif

    file.close();
}



void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}


void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}



void setup_DoorChecker() {

  pinMode(Button_D0, INPUT_PULLUP);
  pinMode(Button_D1, INPUT_PULLUP);
  pinMode(Button_D2, INPUT_PULLUP);
  pinMode(Button_D3, INPUT_PULLUP);

  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  pinMode(D8, INPUT);


}

void currentTime() {
  tmElements_t tm;
  myRTC.read(tm);

  sprintf(time_message, " %04d/%02d/%02d %02d:%02d:%02d",
        tm.Year + 1970, tm.Month, tm.Day,
        tm.Hour, tm.Minute, tm.Second);//人間が読める形式に変換
}

void TASK_ButtonChecker(void *args) {
  // put your main code here, to run repeatedly:
  //delay(5000);

  String wrfile = "/log_test.txt";

  //File fw = SPIFFS.open(wrfile.c_str(), "w");// ⑥ファイルを書き込みモードで開く
  //const char * message;

  setup_DoorChecker();
  
  #if 1
  while(1)
  {
    //Button_D0
    if(!digitalRead(Button_D0) && gButton_D0_State == CLOSE)
    {
      gButton_D0_State = OPEN;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D0, OPEN\r\n",time_message,gRecieveCount);//人間が読める形式に変換
      gSigName = "Button_D0";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_ON);
      delay_with_client_loop(2 * 1000);
      
      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;

    }
    else if(digitalRead(Button_D0) && gButton_D0_State == OPEN)
    {
      gButton_D0_State = CLOSE;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D0, CLOSE\r\n",time_message,gRecieveCount);//人間が読める形式に変換
      gSigName = "Button_D0";

      connectAWS();
      //publishMessage();
      publishMessage_alart(ALART_SIG,EVENT_OFF);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }

    //Button_D1
    if(!digitalRead(Button_D1) && gButton_D1_State == CLOSE)
    {
      gButton_D1_State = OPEN;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D1, OPEN\r\n",time_message,gRecieveCount);
      gSigName = "Button_D1";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_ON);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }
    else if(digitalRead(Button_D1) && gButton_D1_State == OPEN)
    {
      gButton_D1_State = CLOSE;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D1, CLOSE\r\n",time_message,gRecieveCount);
      gSigName = "Button_D1";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_OFF);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }
    //Button_D2
    if(!digitalRead(Button_D2) && gButton_D2_State == CLOSE)
    {
      gButton_D2_State = OPEN;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D2, OPEN\r\n",time_message,gRecieveCount);
      gSigName = "Button_D2";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_ON);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }
    else if(digitalRead(Button_D2) && gButton_D2_State == OPEN)
    {
      gButton_D2_State = CLOSE;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D2, CLOSE\r\n",time_message,gRecieveCount);
      gSigName = "Button_D2";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_OFF);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }
    //Button_D3
    if(!digitalRead(Button_D3) && gButton_D3_State == CLOSE)
    {
      gButton_D3_State = OPEN;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D3, OPEN\r\n",time_message,gRecieveCount);
      gSigName = "Button_D3";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_ON);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }
    else if(digitalRead(Button_D3) && gButton_D3_State == OPEN)
    {
      gButton_D3_State = CLOSE;
      currentTime();
      sprintf(s2, "%s, No.%d, Button_D3, CLOSE\r\n",time_message,gRecieveCount);
      gSigName = "Button_D3";

      connectAWS();
      publishMessage_alart(ALART_SIG,EVENT_OFF);
      delay_with_client_loop(2 * 1000);

      appendFile(SPIFFS, "/log_test.txt", s2);
      delay(INPUT_CHAT_TIME);
      gRecieveCount++;
    }

    delay(100);

  }
  #endif

  #if 0
  while(1) {
    Serial.printf("%s - run\r\n",__func__);
    delay(5000);
  }
  #endif
}



struct tm timeInfo;//時刻を格納するオブジェクト
char s[20];//文字格納用

TaskHandle_t thp[2];//マルチスレッドのタスクハンドル格納用

void multiTaskSetup() {

  xTaskCreatePinnedToCore(TASK_ButtonChecker, "TASK_ButtonChecker", 8192, NULL, 4, NULL, 0); 
  xTaskCreatePinnedToCore(TASK_AWSIotMqtt, "TASK_AWSIotMqtt", 4096, NULL, 3, NULL, 0); 

}



void setup() {

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

// put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\n Starting");
  Serial.println("Ver_106");
  Serial.println("\n----DIR: /");
  listDir(SPIFFS,"/",0);
  SerialNameReadFile(SPIFFS,"/ID.txt");
  readFile(SPIFFS,"/log_test.txt");
 //multiTaskSetup();

  pinMode(D7, INPUT_PULLUP);
  delay(100);

  int longPushCount = 0;
  while(digitalRead(D7)==HIGH)
  {
    longPushCount++;
    delay(1000);
    if (longPushCount > 5)
    {
      WiFiManager wifiManager;
      
      //if (!wifiManager.startConfigPortal((char *)gSerialName.c_str())) {
      if (!wifiManager.startConfigPortal(gSerialName.c_str())) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
      }
    }
  }


  myRTC.begin();

  WiFi.begin();
  Serial.println("connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED) {

    //if you get here you have connected to the WiFi
    IPAddress ipadr = WiFi.localIP();
    Serial.println("connected(^^)");
    Serial.println("local ip");
    Serial.println(ipadr);
    Serial.println(WiFi.SSID());
    Serial.print("Serial name:");
    Serial.println(gSerialName);
    Serial.println("");
    
    Serial.println("Connected to the WiFi network!");
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定

    getLocalTime(&timeInfo);//tmオブジェクトのtimeInfoに現在時刻を入れ込む
    sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
            timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);//人間が読める形式に変換
    Serial.println(s);//時間をシリアルモニタへ出力

    setTime(timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
    myRTC.set(now());
  }

  multiTaskSetup();

}

void loop() {
  // put your main code here, to run repeatedly:

  delay(100);
}





