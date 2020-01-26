#include <Time.h>
#include <Timezone.h>

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, +120};
TimeChangeRule  CET = {"CET",  Last, Sun, Oct, 3, +60};

time_t local_time, utc;
Timezone czCET(CEST, CET);

void setup()  {
  Serial.begin(9600);
  // setTime(hr,min,sec,day,month,year)
  setTime(00,59,50,28,10,2018);
}


void loop(){

  utc = now();
  local_time = czCET.toLocal(utc); 
  digitalClockDisplay();
        
  delay(1000);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour(local_time));
  printDigits(minute(local_time));
  printDigits(second(local_time));
  Serial.print(" ");
  Serial.print(day(local_time));
  Serial.print(" ");
  Serial.print(month(local_time));
  Serial.print(" ");
  Serial.print(year(local_time)); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


