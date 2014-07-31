/*
    Aquarispark v1.00
    By: Tim Soderstrom
*/
#include "OneWire.h"
#include "DallasTemperature.h"

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

/* Polling and update timeouts */
const int sensorPollingInterval = 5;
const int lcdUpdateInterval = 5;
const int alertTimeout = 5;

/* Timezone */
const int timezone = -5;

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

OneWire oneWire(temperatureProbes);
DallasTemperature sensors = DallasTemperature(&oneWire);

// Arrays to hold temperature devices 
// DeviceAddress insideThermometer, outsideThermometer;
DeviceAddress tankThermometer;

DeviceOnSchedule light = 
{
      { 8, 0, 21, 0 }, // 8am - 9pm CST
//  { 3, 0, 16, 0 }, // 8am - 9pm CST (converted to UTC)
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
boolean heater = FALSE;
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

  //Sync the Time
  Time.zone(timezone);
  Spark.syncTime();
    
  // Subscribe variables to the cloud
  Spark.variable("temperature", &currentTemp, DOUBLE);
  Spark.variable("heater", &heater, INT);
  Spark.variable("light", &light.state, INT);
  Spark.variable("stats", &stats, STRING);
    
  // Turn off that incredibly bright LED
  RGB.control(true);
  RGB.color(0, 255, 0);
  RGB.brightness(32);
}

void loop()
{
//  currentMillis = millis() + (timezone * SECS_PER_HOUR * msInSecond);
    currentMillis = millis();

  /* If it's been longer than the polling interval, poll sensors */
  if (currentMillis - lastSensorPoll > sensorPollingInterval * msInSecond)
  {
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
      Serial.println("Light On");
    else
      Serial.println("Light Off");
    
    // Stats Output for API
    sprintf(stats, "%.2f", currentTemp);
    strcat(stats, ":");
    strcat(stats, boolToChar(heater));
    strcat(stats, ":");
    strcat(stats, boolToChar(light.state));
    
    lastSensorPoll = currentMillis;
  }
  if (currentMillis - lastTimeSync > oneDayMillis)
  {
    // Request time synchronization from the Spark Cloud
        Serial.println("Syncing Time");
    Spark.syncTime();
    lastTimeSync = millis();
  }
}


boolean collectTemperatures()
{
  sensors.requestTemperatures();
  if(sensors.getAddress(tankThermometer, 0))
  {
    currentTemp = sensors.getTempC(tankThermometer);
        Serial.print("Current Temperature: ");
        Serial.println(currentTemp);
    /* Check to see if we hit a new low or high temp */
    if(currentTemp > maxTemp)
      maxTemp = currentTemp;
    if(currentTemp < minTemp)
      minTemp = currentTemp;
    return true;
  }
  return false;
}

void controlHeater()
{
  /* If temperature is too high, turn off heater */
  if(currentTemp > highTemp)
  {
      Serial.println("Heater Off");
    digitalWrite(heaterPin, LOW);
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
      Serial.println("Heater On");
    digitalWrite(heaterPin, HIGH);
    heater = true;
  }
}

char* boolToChar(boolean b)
{
    if(b)
        return "1";
    return "0";
}
