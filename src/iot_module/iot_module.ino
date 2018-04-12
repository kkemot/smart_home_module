#include <ESP8266WiFi.h>
#include "configuration.h"
#include "settings.h"
#include "webServer.h"
#include "sensors.h"
#include <ESP8266Influxdb.h>

WiFiClient client;

extern struct Settings settings;

void heartBeatModulation(uint32_t time_counter);
void send_data_InfluxDB(float data_1, float data_2, float data_3, float data_4);

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
    Serial.println("Restore default configuration.");
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
}

uint32_t counter = 0;
void loop() {

  delay(100);

  handleHttpClients();
  handleApConfigurator();
  heartBeatModulation(counter);

  if (counter == 0) {
    char temp[10];
    float temperature, humidity;

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
        send_data_ThingSpeak(temperature, humidity, 1, 1);
        send_data_InfluxDB(temperature, humidity, 1, 1);
        
      }
    }
  } else {
    if (settings.wifi_mode == 2) { //Sleep mode -> wifi mode: 2 - WIFI_STA_DEEP_SLEEP;
      uint32_t sleep_time = settings.sleep_time;
      Serial.print("sleep = ");
      Serial.println(sleep_time,DEC);
      ESP.deepSleep(sleep_time * 1000000);
    }
  }

  counter++;
  if (counter > (settings.sleep_time * 10)) {
    counter = 0;
  }

}


void send_data_ThingSpeak(float data_1, float data_2, float data_3, float data_4) {
  char temp[20];

  // There is no API KEY
  if(strlen(settings.ts_api_key) == 0) {
    Serial.println("NO API KEY - skipped data sending to ThingSpeak");
    return;
  }

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

void send_data_InfluxDB(float data_1, float data_2, float data_3, float data_4) {
  // There is no server address
  if(strlen(settings.influxdb_server_address) == 0) {
    Serial.println("UNKNOW InfluxDB server - skipped data sending to database");
    return;
  }

  Influxdb influxdb(settings.influxdb_server_address, settings.influxdb_server_port);
  influxdb.opendb(settings.influxdb_db_name, settings.influxdb_user, settings.influxdb_pass);


//Test 1

  // Writing data with influxdb HTTP API
  // https://influxdb.com/docs/v0.9/guides/writing_data.html
  Serial.println("Writing data to host " + String(settings.influxdb_server_address) + ":" +
                 settings.influxdb_server_port + "'s database=" + settings.influxdb_db_name);
  String data = "analog_read,method=HTTP_API,pin=A0 value=" + String(10.7);
  influxdb.write(data);
  Serial.println(influxdb.response() == DB_SUCCESS ? "HTTP write success"
                 : "Writing failed - HTTP API");


 // Writing data using FIELD object
  // Create field object with measurment name=analog_read
  FIELD dataObj("analog_read");
  dataObj.addTag("method", "Field_object"); // Add method tag
  dataObj.addTag("pin", "A0"); // Add pin tag
  dataObj.addField("value", analogRead(11.5)); // Add value field
  Serial.println(influxdb.write(dataObj) == DB_SUCCESS ? "Object write success"
                 : "Writing failed - FIELD object");

  
  Serial.println("----> End function");
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

