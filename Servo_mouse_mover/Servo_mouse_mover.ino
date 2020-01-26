/*
Servo Connection:
BROWN - GND
RED - +5V
ORANGE - ServoPin (9)
*/
 
#include <Servo.h>

int servoPin = 9;
Servo servo; 


int pos_max = 175;         // Maximum position of servo [degrees]
int pos_min = 140;         // Minimum position of servo [degrees]
int servo_pause = 60;      // Delay in servo loop [ms]
int angle = 2;             // Angle step in servo loop [degrees]
int pos = 160;             // Initial servo position [degrees]
long cycle_delay = 20000;  // Delay between movement up and down [ms]

////////////////////////////////////////////////////////////////

void setup() 
{ 
  servo.attach(servoPin);
  servo.write(pos);
} 


////////////////////////////////////////////////////////////////
void loop() 
{ 

  while( pos < pos_max ) {
    pos = pos + angle;
    servo.write(pos);
    delay(servo_pause);
  }

  delay(cycle_delay);
  
  while( pos > pos_min ) {
    pos = pos - angle;
    servo.write(pos);
    delay(servo_pause);
  }

  delay(cycle_delay);
} 
