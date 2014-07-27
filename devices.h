#include <time.h>


#define ONE_DAY_MILLIS 86400000
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define elapsedSecsToday(_time_)  (_time_ % SECS_PER_DAY)

/*
 * Structure to handle any device on a schedule
 * That is a device that is on or off depending on the time
 *
 * TODO: Overrides for manual control where it won't mess up
 * the schedule
 */
struct DeviceOnSchedule{
  int schedule[4];
  int pin;
  boolean state;
};


boolean checkSchedule(DeviceOnSchedule &device)
{
  unsigned long startSeconds = device.schedule[0] * (long)3600 + device.schedule[1] * 60;
  unsigned long stopSeconds = device.schedule[2] * (long)3600 + device.schedule[3] * 60;	
	unsigned long currentSeconds = Time.hour() * (long)3600 + Time.minute() * 60;

  if((currentSeconds > startSeconds) & (stopSeconds > currentSeconds))
  {
    digitalWrite(device.pin, HIGH);
    device.state = true;
		return true;
  }
  digitalWrite(device.pin, LOW);
  device.state = false; 
	return false;
}


/*
 * Function to control devices on a schedule
 */
void deviceOn(DeviceOnSchedule device)
{
  digitalWrite(device.pin, HIGH);
  device.state = true; 
}

void deviceOff(DeviceOnSchedule device)
{
    digitalWrite(device.pin, LOW);
    device.state = false; 
}


