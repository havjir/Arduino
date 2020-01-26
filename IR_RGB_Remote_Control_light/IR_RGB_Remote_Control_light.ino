// IR Remote Control
#include <IRLib.h>

#define MY_PROTOCOL NEC

#define BT_UP     0xFF3AC5 // Light up
#define BT_DW     0xFFBA45 // Light down

#define BT_PLAY   0xFF827D // Play button
#define BT_OFF    0xFF02FD // Off

#define BT_R      0xFF1AE5 // R button
#define BT_G      0xFF9A65 // G button
#define BT_B      0xFFA25D // B buton
#define BT_W      0xFF22DD // W buton

#define BT_C11    0xFF2AD5 // RED (row 1/ col 1)
#define BT_C12    0xFFAA55 // LIGHT GREEN (row 1/ col 2)
#define BT_C13    0xFF926D // DARK BLUE (row 1/ col 3)
#define BT_C14    0xFF12ED // PINK (row 1/ col 4)
#define BT_C21    0xFF0AF5 // ORANGE (row 2/ col 1)
#define BT_C22    0xFF8A75 // LIGHT BLUE (row 2/ col 2)
#define BT_C23    0xFFB24D // BROWN (row 2/ col 3)
#define BT_C24    0xFF32CD // PINK (row 2/ col 4)
#define BT_C31    0xFF38C7 // BEIGE bezova (row 3/ col 1)
#define BT_C32    0xFFB847 // MEDIUM BLUE (row 3/ col 2)
#define BT_C33    0xFF7887 // MEDIUM RED  (row 3/ col 3)
#define BT_C34    0xFFF807 // LIGHT BLUE  (row 3/ col 4)
#define BT_C41    0xFF18E7 // YELLOW (row 4/ col 1)
#define BT_C42    0xFF9867 // DARK BLUE (row 4/ col 2)
#define BT_C43    0xFF58A7 // RED (row 4/ col 3)
#define BT_C44    0xFFD827 // LIGHT BLUE (row 4/ col 4)

#define BT_RU     0xFF28D7 // RED UP
#define BT_GU     0xFFA857 // GREEN UP
#define BT_BU     0xFF6897 // BLUE UP
#define BT_RD     0xFF08F7 // RED DOWN
#define BT_GD     0xFF8877 // GREEN DOWN
#define BT_BD     0xFF48B7 // BLUE DOWN
#define BT_DIY1   0xFF30CF // DIY1
#define BT_DIY2   0xFFB04F // DIY2
#define BT_DIY3   0xFF708F // DIY3
#define BT_DIY4   0xFF10EF // DIY4
#define BT_DIY5   0xFF906F // DIY5
#define BT_DIY6   0xFF50AF // DIY6

#define BT_QUIC   0xFFE817 // QUICK
#define BT_SLOW   0xFFC837 // SLOW

#define BT_AUTO   0xFFF00F // AUTO
#define BT_FLASH  0xFFD02F // FLASH
#define BT_FADE7  0xFFE01F // FADE7
#define BT_FADE3  0xFF609F // FADE3
#define BT_JUMP7  0xFFA05F // JUMP7
#define BT_JUMP3  0xFF20DF // JUMP3

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
       case BT_C11: str_btn = "BT_0";
       case BT_C12: str_btn = "BT_1";
       case BT_C13: str_btn = "BT_2";
    }
    }
    My_Receiver.resume();     //Restart the receiver
  }
}
