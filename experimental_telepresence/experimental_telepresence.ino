/******************
CONFIGS HERE
********************/
char* my_device_name = "lester";




/// set up WIFIManager
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
WiFiManager wifiManager;







void setup() 
{
  Serial.begin(9600); // initialize serial communications at 9600 bps
  Serial.println("connecting?");
  wifiManager.autoConnect(my_device_name);
  Serial.println("connected I think");
}

void loop() 
{ 
  delay(100);//delay 200ms
}


void led_flash(int pin, int onms, int offms, int times){
  // flash an led a certain number of times. Good for status things.
  for(int i = 0; i<times; i++){
    digitalWrite(pin, HIGH);
    delay(onms);
    digitalWrite(pin, LOW);
    delay(offms);
  }
}