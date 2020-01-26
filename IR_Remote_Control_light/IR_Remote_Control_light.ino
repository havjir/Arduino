// IR Remote Control
#include <IRLib.h>

#define MY_PROTOCOL NEC
#define BT_OFF 0xFFA25D
#define BT_0   0xFF6897
#define BT_1   0xFF30CF
#define BT_2   0xFF18E7
#define BT_3   0xFF7A85
#define BT_4   0xFF10EF
#define BT_5   0xFF38C7
#define BT_6   0xFF5AA5
#define BT_7   0xFF42BD
#define BT_8   0xFF4A5B
#define BT_9   0xFF52AD
#define BT_MODE   0xFF629D
#define BT_MUTE   0xFFE21D
#define BT_PLAY   0xFF22DD
#define BT_BACK   0xFF02FD
#define BT_FWD    0xFFC23D
#define BT_EQ     0xFFE01F
#define BT_MINUS  0xFFA857
#define BT_PLUS   0xFF906F
#define BT_RND    0xFF9867
#define BT_USD    0xFFB04F


long Previous;   // Stores previous code to handle NEC repeat codes
String str_btn;

//Create a receiver object to listen on pin 11
IRrecv My_Receiver(11);
 
//Create a decoder object
IRdecode My_Decoder;


 
void setup()
{
  Serial.begin(9600);
  My_Receiver.enableIRIn(); // Start the receiver
  Serial.println("Setup");

}
 
void loop() {
//Continuously look for results. When you have them pass them to the decoder
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();    //Decode the data
    if(My_Decoder.decode_type==MY_PROTOCOL) {
    My_Decoder.DumpResults(); //Show the results on serial monitor
    if(My_Decoder.value==0xFFFFFFFF)
         My_Decoder.value=Previous;
    switch(My_Decoder.value) {
       case BT_0: str_btn = "BT_0";
       case BT_1: str_btn = "BT_1";
       case BT_2: str_btn = "BT_2";
       case BT_3: str_btn = "BT_3";
       case BT_4: str_btn = "BT_4";
       case BT_5: str_btn = "BT_5";
       case BT_6: str_btn = "BT_6";
       case BT_7: str_btn = "BT_7";
       case BT_8: str_btn = "BT_8";
       case BT_9: str_btn = "BT_9";
    }
    }
    My_Receiver.resume();     //Restart the receiver
  }
}
