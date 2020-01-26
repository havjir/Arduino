#include <SoftwareSerial.h>

SoftwareSerial mySerial(11, 10); // RX, TX
String command = ""; // Stores response of bluetooth device
                                     // which simply allows \n between each
                                     // response.
void setup() 
{
   // Open serial communications and wait for port to open:
   Serial.begin(9600);
   Serial.println("Type AT commands!");
   // SoftwareSerial "com port" data rate. JY-MCU v1.03 defaults to 9600.
   mySerial.begin(115200);
}

void loop()
{
   // Read device output if available.
   if (mySerial.available()) {
     command = "";
     //Serial.println("Data from ESP:");
     while(mySerial.available()) { // While there is more to be read, keep reading.
       command += (char)mySerial.read();
     }
     Serial.println(command);
     command = ""; // No repeats
     Serial.println("END from ESP:");
   }
  
   // Read user input if available.
   if (Serial.available()){
       command = "";
       //Serial.println("Data from Serial:");
       //delay(10); // The DELAY!
       while (Serial.available()){
        command += (char)Serial.read();
       }
       mySerial.print(command);
       //Serial.println("END write to ESP:");
   }
}

/*
AT+VERSION - Returns the software version of the module

AT+BAUDx - Sets the baud rate of the module (The command AT+BAUD8 sets the baud rate to 115200)
1 >> 1200 
2 >> 2400 
3 >> 4800 
4 >> 9600 (Default) 
5 >> 19200 
6 >> 38400 
7 >> 57600 
8 >> 115200 
9 >> 230400

AT+NAME<name> - Sets the name of the module Any name can be specified up to 20 characters

AT+PINxxxx - Sets the pairing password of the device. Any 4 digit number can be used, the default pincode is 1234

AT+PN - Sets the parity of the module (AT+PN >> No parity check)
*/
