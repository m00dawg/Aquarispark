/*
    Aquarispark v1.00
    By: Tim Soderstrom
*/
//#include "particle-dallas-temperature.h"
//#include "DallasTemperature.h"
#include "Particle-OneWire.h"
#include "DS18B20.h"
#include "MQTT.h"


/* Custom Functions */
#include "devices.h"

/*
 * -------
 * DEFINES
 * ------- 
 */

/* States */
#define ON 1
#define OFF 0

/*
 * ---------
 * CONSTANTS
 * ---------
 */
const int msInSecond = 1000; //1000 ms = 1 msInSecond
const int oneDayMillis = 24 * 60 * 60 * 1000;

/* 
 * -----------
 * CONFIG OPTS
 * -----------
 */

/* Pins */
const int temperatureProbes = D2;
const int heaterPin = A0;
const int lightPin = A2;

/* Polling */
//const int sensorPollingInterval = 5;
const int sensorPollingInterval = 20;

/* Timezone */
const int timezone = -5;
//const int timezone = -6;

/* MQTT Server */
byte mqtt_server[] = { 192,168,100,2 };
int mqtt_port = 1883;

/* 
 Temperature range to cycle heater in Celsius
 lowTemp = Temp reached before heater turns on
 highTemp = Temp reached before heater turns off
 alertHighTemp = Tank too hot even with heater off
 alertLowTemp = Tank too cold even with heater on
 */
//const float lowTemp = 24.25;
//const float highTemp = 24.5;
const float lowTemp = 26.25;
const float highTemp = 26.5;
const float alertHighTemp = 27.0;
const float alertLowTemp = 23.0;

/*
 * -------
 * OBJECTS
 * ------- 
 */

// Temperature Sensors
DS18B20 sensors = DS18B20(temperatureProbes);
int sensorAttempts=0;

// MQTT
void mqtt_callback(char* topic, byte* payload, unsigned int length);
MQTT mqtt(mqtt_server, mqtt_port, mqtt_callback);

// Light Schedule 
DeviceOnSchedule light = 
{
  { 10, 0, 17, 0 }, // 10am - 5pm CST
  lightPin,
  true                 // State
};            

/*
 * ----------------
 * STATUS VARIABLES
 * ----------------
 */

/* Number of millimsInSeconds since bootup */
unsigned long currentMillis = 0;
unsigned long lastTimeSync = millis();

/* Poll Timeouts for various things */
unsigned long lastSensorPoll = 0;

/* Min and max temperatures seen */
float maxTemp = 0;
float minTemp = 100;
double currentTemp = 0;

/* Heater Status */
boolean heater = false;
int heaterCycles = 0;

//String stats;
char stats[12];

/* Switches from C to F for display */
boolean displayCelsius = true;

void setup()
{ 
  Serial.begin(9600); 
  Serial.println("Aquariduino");

  pinMode(lightPin, OUTPUT);
  digitalWrite(lightPin, HIGH);
  pinMode(heaterPin, OUTPUT);
  digitalWrite(heaterPin, LOW);
  pinMode(temperatureProbes, INPUT);

  //Sync the Time
  Time.zone(timezone);
  Particle.syncTime();
    
  // Subscribe variables to the cloud
  //Particle.variable("temperature", &currentTemp, DOUBLE);
  //Particle.variable("heater", &heater, INT);
  //Particle.variable("light", &light.state, INT);
//  Particle.variable("light", boolToInt(&light.state), INT);
  //Particle.variable("stats", &stats, STRING);
  Particle.variable("temperature", currentTemp);
  Particle.variable("heater", heater);
  Particle.variable("light", light.state);
    
  // Turn off that incredibly bright LED
  RGB.control(true);
  RGB.color(0, 255, 0);
  RGB.brightness(32);

  // Connect to MQTT
  mqtt.connect("Aquarispark");
}

void loop()
{
    currentMillis = millis();
    Serial.print("Time: ");
    Serial.print(Time.hour());
    Serial.print(":");
    Serial.println(Time.minute());
        
    if(collectTemperatures())
        controlHeater();
    else
        Serial.println("NO SENSORS");
    
    // Check to see if it's time for light to turn on/off
    if(checkSchedule(light))
    {
      Serial.println("Light On");
      mqtt.publish("aquarispark/light", "1");
    }
    else
    {
      Serial.println("Light Off");
      mqtt.publish("aquarispark/light", "0");
    }
    
    lastSensorPoll = currentMillis;
  
  if (currentMillis - lastTimeSync > oneDayMillis)
  {
    // Request time synchronization from the Particle Cloud
    Serial.println("Syncing Time");
    Particle.syncTime();
    lastTimeSync = millis();
  }

  delay((long)sensorPollingInterval * msInSecond);
}


boolean collectTemperatures()
{
  //sensors.requestTemperatures();
 // if(sensors.getAddress(tankThermometer, 0))
 // {
//        currentTemp = sensors.getTemperature(tankThermometer);
    if(!sensors.search())
    {
        sensors.resetsearch();
        currentTemp = sensors.getTemperature();
        while (!sensors.crcCheck() && sensorAttempts < 4)
        {
            Serial.println("Caught bad value.");
            sensorAttempts++;
            Serial.print("Attempts to Read: ");
            Serial.println(sensorAttempts);
            if (sensorAttempts == 3)
                delay(1000);
            sensors.resetsearch();
            currentTemp = sensors.getTemperature();
            continue;
        }
        sensorAttempts = 0;
        Serial.print("Current Temperature: ");
        Serial.println(currentTemp);
        /* Check to see if we hit a new low or high temp */
        if(currentTemp > maxTemp)
            maxTemp = currentTemp;
        if(currentTemp < minTemp)
            minTemp = currentTemp;
        mqtt.publish("aquarispark/temp", String(currentTemp));
        return true;
    }
  return false;
}

void controlHeater()
{
  /* If temperature is too high, turn off heater */
  if(currentTemp > highTemp)
  {
    digitalWrite(heaterPin, LOW);
    mqtt.publish("aquarispark/heater", "0");
    Serial.println("Heater Off");
    /* If the heater was on, we know we have completed a 
     * cycle, so let's count it */
    if(heater)
      ++heaterCycles;
    heater = false;
    return;
  }
  /* If the temperature is too low, turn on heater */
  if(currentTemp < lowTemp)
  {
    digitalWrite(heaterPin, HIGH);
    mqtt.publish("aquarispark/heater", "1");
    Serial.println("Heater On");
    heater = true;
  }
}
/*
char* boolToChar(boolean b)
{
    if(b)
        return '1';
    return '0';
}

int boolToInt(boolean b)
{
    if(b)
        return 1;
    return 0;
}
*/

/* Currently do nothing because we don't need to receive messages */
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
}
