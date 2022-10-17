#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <DS3232RTC.h>

#include "FS.h"
#include "SPIFFS.h"	// ①ライブラリを読み込み

#include "InputChecker.h"

#include "aws_iot.h"


#define FORMAT_SPIFFS_IF_FAILED true




DS3232RTC myRTC(false);

char s2[80];//文字格納用
char time_message[20];//文字格納用


struct tm timeInfo;//時刻を格納するオブジェクト
char s[20];//文字格納用

TaskHandle_t thp[2];//マルチスレッドのタスクハンドル格納用

void multiTaskSetup() {

  xTaskCreatePinnedToCore(TASK_ButtonChecker, "TASK_ButtonChecker", 8192, NULL, 4, NULL, 0); 
  xTaskCreatePinnedToCore(TASK_AWSIotMqtt, "TASK_AWSIotMqtt", 4096, NULL, 3, NULL, 0); 

}


void currentTime() {
  tmElements_t tm;
  myRTC.read(tm);

  sprintf(time_message, " %04d/%02d/%02d %02d:%02d:%02d",
        tm.Year + 1970, tm.Month, tm.Day,
        tm.Hour, tm.Minute, tm.Second);//人間が読める形式に変換
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

  pinMode(D7, INPUT);
  delay(100);

  int longPushCount = 0;
  while(digitalRead(D7)==HIGH)
  {
    longPushCount++;
    Serial.println("D7 is HIGH");
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

    //gAWS_topic = "topic/" + gSerialName;
    gAWS_topic = AWS_IOT_PUBLISH_TOPIC + gSerialName;
    Serial.println(gAWS_topic);
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





