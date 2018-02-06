#include <ESP8266WiFi.h>
#include "configuration.h"
#include "settings.h"
#include "webServer.h"
#include "sensors.h"
#include "filters.h"

WiFiClient client;
extern struct Settings settings;

simpleFilter filter_pm25;
simpleFilter filter_pm10;

void setup() {
  int button_cnt = 0;

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();

  //hardware_configuration
  pinMode(LED_PIN, OUTPUT);
  pinMode(USER_BUTTON, INPUT_PULLUP);

  init_file_system();

  if (load_settings()) {
    Serial.println("Configuration loaded.");
  }
  else {
    Serial.println("Resore default configuration.");
    set_default_settings();
    save_settings();
  }
  print_settings();

  while (digitalRead(USER_BUTTON) == BUTTON_PRESSED) {
    digitalWrite(LED_PIN, LED_ON);
    button_cnt ++;
    delay(1000);
    if (button_cnt > 5)
      break;
  }
  //Blinking LED
  for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, LED_ON);
      delay(50);
      digitalWrite(LED_PIN, LED_OFF);
      delay(50);
    }

  digitalWrite(LED_PIN, LED_OFF);

  //start normal mode
  if (button_cnt == 0) {
    Serial.println("Start normal mode");
    startHttpServer();
  }
  //restore default settings and start setup mode
  else if ( button_cnt > 5) {
    Serial.println("Set default settings and start setup mode");
    set_default_settings();
    save_settings();
    startHttpServer();
  }
  //start setup mode only
  else {
    Serial.println("Start setup mode only");
    changeHttpServerMode(WIFI_AP);
  }
  init_sensors();

  filter_pm25.setFilterSize(15);
  filter_pm10.setFilterSize(15);
  filter_pm25.clear();
  filter_pm10.clear();
}

uint32_t counter = 0;
void loop() {

  delay(100);

  handleHttpClients();
  handleApConfigurator();
  heartBeatModulation(counter);

  if (counter == 0) {
   dustSensor_enable(true);
  }
  else if(counter < (SENSOR_WARMING_TIME * 10)) {
    char temp[10];
    float pm25, pm10;
    if (counter % 10 == 0) {
      if (dustSensor_getData(&pm25, &pm10) == 0) {
         filter_pm25.add(pm25);
         filter_pm10.add(pm10);
      }
    }
  } else if (counter  == (SENSOR_WARMING_TIME * 10)) {
    char temp[10];
    float pm25, pm10;
    float temperature, humidity;

    dustSensor_getData(&pm25, &pm10);
    dustSensor_enable(false);

    pm25 = filter_pm25.get();
    pm10 = filter_pm10.get();
    filter_pm25.clear();
    filter_pm10.clear();

    //PM25
    Serial.print("PM2.5 norm=");
    sprintf(temp,"%.1f", (pm25*100)/PM25_NORM);
    Serial.print(temp);
    Serial.print("% / ");
    //PM10
    Serial.print("PM10 norm=");
    sprintf(temp,"%.1f", (pm10*100)/PM10_NORM);
    Serial.print(temp);
    Serial.println("%");

    temperature = getTemperature();
    humidity = getHumidity();

    if ((temperature < -100) || (temperature > 125) || (humidity < 0) || (humidity >100)) {
       Serial.println("Wrong sensor datas");
    } else {
      Serial.print("Temperature: ");
      sprintf(temp,"%.1f", temperature);
      Serial.print(temp);
      Serial.print("C / ");
      Serial.print(" Humidity: ");
      sprintf(temp,"%.0f", humidity);
      Serial.print(temp);
      Serial.println("%");

      //sent to ThinkSpeak
      if (isConnectedSTA()) {
        send_data_ThingSpeak(temperature, humidity, (pm25*100)/PM25_NORM, (pm10*100)/PM10_NORM);
      }
    }
  } else {
    if (settings.wifi_mode == 2) { //Sleep mode -> wifi mode: 2 - WIFI_STA_DEEP_SLEEP;
      uint32_t sleep_time = settings.sleep_time - SENSOR_WARMING_TIME;
      Serial.print("sleep = ");
      Serial.println(sleep_time,DEC);
      ESP.deepSleep(sleep_time * 1000000);
    } else {
      //Serial.println("wait...");
    }
  }

  counter++;
  if (counter > (settings.sleep_time * 10)) {
    counter = 0;
  }

}


void send_data_ThingSpeak(float data_1, float data_2, float data_3, float data_4) {
  char temp[20];

  if (client.connect(TS_SERVER_NAME,80)) {
    String API_KEY = settings.ts_api_key;
    String postStr = API_KEY;
    postStr +="&field1=";
    sprintf(temp,"%.1f", data_1);
    postStr += String(temp);
    postStr +="&field2=";
    sprintf(temp,"%.0f", data_2);
    postStr += String(temp);
    postStr +="&field3=";
    sprintf(temp,"%.1f", data_3);
    postStr += String(temp);
    postStr +="&field4=";
    sprintf(temp,"%.1f", data_4);
    postStr += String(temp);
    postStr += "\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+API_KEY+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
    Serial.println(postStr);
    Serial.println("Sent to Thingspeak.");
  }
  client.stop();
}

//changes LED state
void heartBeatModulation(uint32_t time_counter) {
  WiFiMode_t currentWiFiMode = getWiFiMode();
  int time_stamp = time_counter % 100;

  if (currentWiFiMode == WIFI_AP_STA) {
    if (time_stamp == 0)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 10)
      digitalWrite(LED_PIN, LED_OFF);
    else if (time_stamp == 15)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 25)
      digitalWrite(LED_PIN, LED_OFF);

    if (isConnectedSTA()) {
      if (time_stamp == 30)
        digitalWrite(LED_PIN, LED_ON);
      else if (time_stamp == 32)
        digitalWrite(LED_PIN, LED_OFF);
      else if (time_stamp == 34)
        digitalWrite(LED_PIN, LED_ON);
      else if (time_stamp == 36)
        digitalWrite(LED_PIN, LED_OFF);
    }
    else {
      if (time_stamp == 0)
        reconnectLastMode();
    }
  } else if (currentWiFiMode == WIFI_STA) {
     if (time_stamp == 0)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 10)
      digitalWrite(LED_PIN, LED_OFF);

    if (isConnectedSTA()) {
      if (time_stamp == 15)
        digitalWrite(LED_PIN, LED_ON);
      else if (time_stamp == 17)
        digitalWrite(LED_PIN, LED_OFF);
      else if (time_stamp == 19)
        digitalWrite(LED_PIN, LED_ON);
      else if (time_stamp == 21)
        digitalWrite(LED_PIN, LED_OFF);
    }
    else {
      if (time_stamp == 0)
        reconnectLastMode();
    }
  }  else if (currentWiFiMode == WIFI_AP) {
    if (time_stamp == 0)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 10)
      digitalWrite(LED_PIN, LED_OFF);
    else if (time_stamp == 15)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 25)
      digitalWrite(LED_PIN, LED_OFF);
   else if (time_stamp == 30)
      digitalWrite(LED_PIN, LED_ON);
    else if (time_stamp == 40)
      digitalWrite(LED_PIN, LED_OFF);
  }
}

