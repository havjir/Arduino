

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FIRST INITIALIZE WiFi AND THEN IR RECEIVER !!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// IR Remote Control
#include <IRremote.h>

#define MY_PROTOCOL NEC
#define BT_OFF   0xFFA25D    // Line 1: ON/OFF
#define BT_MD    0xFF629D    // Mode
#define BT_MT    0xFFE21D    // Mute
#define BT_PLAY  0xFF22DD    // Line 2: Play / Pause
#define BT_REW   0xFF02FD    // Rew
#define BT_FWD   0xFFC23D    // Fwd
#define BT_EQ    0xFFE01F    // Line 3: EQ
#define BT_MNS   0xFFA857    // Minus
#define BT_PLS   0xFF906F    // Plus
#define BT_0     0xFF6897    // Line 4: 0
#define BT_SFL   0xFF9867    // Shuffle
#define BT_USD   0xFFB04F    // USD
#define BT_1     0xFF30CF    // Line 5: 1
#define BT_2     0xFF18E7    // 2
#define BT_3     0xFF7A85    // 3
#define BT_4     0xFF10EF    // Line 6: 4
#define BT_5     0xFF38C7    // 5
#define BT_6     0xFF5AA5    // 6
#define BT_7     0xFF42BD    // Line 7: 7
#define BT_8     0xFF4AB5    // 8
#define BT_9     0xFF52AD    // 9

long Previous;   // Stores previous code to handle NEC repeat codes
String str_btn;

//Create a receiver object to listen on pin 19
IRrecv My_Receiver(19);
 
//Create a decoder object
//IRdecode My_Decoder;
decode_results My_Decoder;

 
void setup()
{
  Serial.begin(115200);
  My_Receiver.enableIRIn(); // Start the receiver
  Serial.println("Setup");

}
 
void loop() {
//Continuously look for results. When you have them pass them to the decoder
  if (My_Receiver.decode(&My_Decoder)) {
    if(My_Decoder.decode_type==MY_PROTOCOL) {
    
    //Serial.println(My_Decoder.value, HEX);
    if(My_Decoder.value==0xFFFFFFFF) {
      My_Decoder.value=Previous;
    } else {
      Previous = My_Decoder.value;
    }
         
    Serial.println(My_Decoder.value, HEX);
/*    switch(My_Decoder.value) {
        case BT_OFF: { if(RED > 0) { RED = 0; GREEN = 0; BLUE = 0; }
                       else {RED = 99; GREEN = 99; BLUE = 99;} break;}
        case BT_0: { RED = 3; GREEN = 3; BLUE = 3; break;}
        case BT_1: { RED = 6; GREEN = 6; BLUE = 6; break;}
        case BT_2: { RED = 9; GREEN = 9; BLUE = 9; break;}
        case BT_3: { RED = 15; GREEN = 15; BLUE = 15; break;}
        case BT_4: { RED = 20; GREEN = 20; BLUE = 20; break;}
        case BT_5: { RED = 30; GREEN = 30; BLUE = 30; break;}
        case BT_6: { RED = 40; GREEN = 40; BLUE = 40; break;}
        case BT_7: { RED = 60; GREEN = 60; BLUE = 60; break;}
        case BT_8: { RED = 80; GREEN = 80; BLUE = 80; break;}
        case BT_9: { RED = 99; GREEN = 99; BLUE = 99; break;}
        case BT_PLS: { bt_plus(STEP); break;}
        case BT_MNS: { bt_minus(STEP); break;}
    }
*/    
    }
    My_Receiver.resume();     //Restart the receiver
  }
}
