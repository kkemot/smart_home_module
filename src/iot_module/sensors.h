#pragma once

#define SENSOR_WARMING_TIME 50
#define PM25_NORM 25
#define PM10_NORM 40

float getTemperature(void);
float getHumidity(void);

void init_sensors(void);
int dustSensor_getData(float *p25, float *p10);
void dustSensor_enable(bool enable);
