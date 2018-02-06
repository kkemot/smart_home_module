#include <ESP8266WiFi.h>
#include <SHT1x.h>
#include <SDS011.h>

#include "sensors.h"
#include "configuration.h"

SHT1x sht1x(sht_dataPin, sht_clockPin);
static float temperature;
static float humidity;

SDS011 my_sds;

void init_sensors(void) {
  my_sds.begin(dust_rx, dust_tx);
  my_sds.wakeup();
}

void dustSensor_enable(bool enable) {
  if (enable == true) {
    Serial.println("Wakeup dust sensor !");
    my_sds.wakeup();
  } else {
     Serial.println("Sleep dust sensor !");
     my_sds.sleep();
  }
}

//read data and deactivate dust sensor
int dustSensor_getData(float *p25, float *p10) {

  int error = my_sds.read(p25, p10);
  return error;
}

void printSensorResoults(void) {
  char temp[10];
  int error;
  float pm25, pm10;
  // Read values from the sensor
  temperature = sht1x.readTemperatureC();
  humidity = sht1x.readHumidity();

  // Print the values to the serial port
  Serial.print("Temperature: ");
  sprintf(temp,"%.1f", temperature);
  Serial.print(temp);
  Serial.print("C / ");
  Serial.print(" Humidity: ");
  sprintf(temp,"%.0f", humidity);
  Serial.print(temp);
  Serial.println("%");

  if ((temperature < -100) || (temperature > 125) || (humidity < 0) || (humidity >100)) {
     Serial.println("Wrong sensor datas");
  }
  else {
     Serial.println("data ok");
  }


  if (my_sds.read(&pm25, &pm10) != 0) {
    pm25 = 0;
    pm10 = 0;
    Serial.println("Dust sensor data is unavailable");
  }
  else {
    Serial.println("P2.5: "+String(pm25));
    Serial.println("P10:  "+String(pm10));

    //PM25
    Serial.print("PM2.5 norm=");
    sprintf(temp,"%.1f", (pm25*100)/PM25_NORM);
    Serial.print(temp);
    Serial.println("%");
    //PM10
    Serial.print("PM10 norm=");
    sprintf(temp,"%.1f", (pm10*100)/PM10_NORM);
    Serial.print(temp);
    Serial.println("%");
  }
}

float getTemperature(void) {
  return sht1x.readTemperatureC();
}

float getHumidity(void) {
  return sht1x.readHumidity();
}

