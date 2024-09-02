// Adafruit IO Subscription Example
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

char* my_device_name = "amber";

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

// TIMING INCLUDES
#include <elapsedMillis.h>
#include <AsyncTimer.h> //https://github.com/Aasim-A/AsyncTimer
AsyncTimer t;

// Rate Limiter
// Create a limiter limiting call rate to 5 per 1000ms.
#include "RateLimiter.h"
//static RateLimiter<1000, 25> limiter;
static RateLimiter<60000, 25> limiter;

/// set up WIFIManager
#include <WiFiManager.h>          // install https://github.com/tzapu/WiFiManager WiFi Configuration Magic
WiFiManager wifiManager;
#include "WiFiClientSecure.h"
// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

#include "AdafruitIO_WiFi.h"


AdafruitIO_WiFi io(IO_USERNAME, IO_KEY,"","");

// set up the 'tapping' feed
AdafruitIO_Feed *tappingSub = io.feed("tappingB");
AdafruitIO_Feed *tappingPub = io.feed("tappingA");


// RATE LIMIT STUFF
int RATE_LIMIT = 30;
int RATE_LIMIT_TIME = 60000;
int rate_limit_count = 0;
bool firstDroppedCallMessageSent = false;
elapsedMillis sendTracker;
int  sendCount = 0;
bool firstSend = true;


/*********************************
DEVICE SETUPS
*******************************/
const int PIEZO_PIN = A2; // Piezo input
const int BUZZER_PIN = 16; //Pieazo buzzer on arduino pin 16

//  Capacitive Touch Sensor Code
const int TOUCH_PIN = T6;
const int TOUCH_THRESHOLD = 10; // value lower than this triggers ON
int touchVal = 0;
int lastTouchSent = 0;

// PIEZO CODE
uint32_t maxPiezoThisCycle;
uint32_t lastPiezoSent;




void setup() {

  // start the serial connection
  Serial.begin(9600);
  Serial.println("in setup");
  sendTracker = 0;
  while(! Serial); // wait for Serial to be ready
  setup_devices();
  // wait for serial monitor to open
  setup_wifi();
  setup_aio();
  setup_subscribers();
  setup_timers();  
  led_flash(LED_BUILTIN, 250,125,5);
}

void setup_devices(){
  pinMode(BUZZER_PIN, OUTPUT); // Set buzzer - pin 9 as an output
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, LOW);
}

void setup_wifi(){
  wifiManager.autoConnect(my_device_name);
}

void setup_aio(){
  Serial.print("Connecting to Adafruit IO");

  // start MQTT connection to io.adafruit.com
  io.connect();
  // wait for an MQTT connection
  // NOTE: when blending the HTTP and MQTT API, always use the mqttStatus
  // method to check on MQTT connection status specifically
  while(io.mqttStatus() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void setup_subscribers(){
 // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  tappingSub->onMessage(handleTapping);
  // Because Adafruit IO doesn't support the MQTT retain flag, we can use the
  // get() function to ask IO to resend the last value for this feed to just
  // this MQTT client after the io client is connected.
  tappingSub->get();
  Serial.println("connected");

}

void setup_timers(){
  Serial.println("setup_timers");
 // t.setInterval(readPiezo, 100);
  t.setInterval(readTouch, 100);
  limiter.SetDroppedCallCallback([&](unsigned int dropped_calls){
    Serial.println("rate limited!");
    firstSend = true;
    digitalWrite(LED_BUILTIN, HIGH);
    if(!firstDroppedCallMessageSent){
      firstDroppedCallMessageSent = true;
      Serial.println("dropping calls, sending 0");
      tappingPub->save(0);
    }
  });
}

void readTouch(){
  // Now we can publish stuff!
  uint32_t piezoADC = touchRead(TOUCH_PIN);
  //Serial.println(piezoADC); // uncomment this to see the values being produced, and adjust the TOUCH_THRESHOLD accordingly
  // we're just goign to send an ON or OFF, not in-between values
  if(piezoADC < TOUCH_THRESHOLD){
    touchVal = 1;
  }else{
    touchVal = 0;
  }
  //  Serial.println(touchVal);
  sendTouch();
}

void sendTouch(){
  if(lastTouchSent != touchVal){
    limiter.CallOrDrop([&](){    
      if(firstSend){
        sendTracker =0;
        firstSend = false;
        sendCount = 0;
      }
      sendCount++;
      digitalWrite(LED_BUILTIN, LOW);
      firstDroppedCallMessageSent = false;
      Serial.print(F("\nSending val "));
      Serial.print(touchVal);  
      tappingPub->save(touchVal);
    });
    lastTouchSent = touchVal;
  }

}

void readPiezo(){
  // Now we can publish stuff!
  uint32_t piezoADC = analogRead(PIEZO_PIN);  
  Serial.println("reading " );
  Serial.println(piezoADC);
//  float piezoV = piezoADC / 1023.0 * 5.0;
  if(piezoADC > maxPiezoThisCycle){
    maxPiezoThisCycle = piezoADC;
  }
  sendPiezo();
}

void sendPiezo(){
  limiter.CallOrDrop([&](){
    Serial.print(maxPiezoThisCycle);
    firstDroppedCallMessageSent = false;
    if(lastPiezoSent != maxPiezoThisCycle){
      Serial.print(F("\nSending val "));
      Serial.print(maxPiezoThisCycle);
      Serial.print(F(" to test feed..."));
      tappingPub->save(maxPiezoThisCycle);
      lastPiezoSent = maxPiezoThisCycle;
      maxPiezoThisCycle = 0;  
    }
  });
}


void loop() {

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();
  t.handle();

  // Because this sketch isn't publishing, we don't need
  // a delay() in the main program loop.

}

// this function is called whenever a 'counter' message
// is received from Adafruit IO. it was attached to
// the counter feed in the setup() function above.
void handleTapping(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());
  int pitch = data->toInt();
  if(pitch > 0){
    tone(BUZZER_PIN, pitch); // Send 1KHz sound signal...
    delay(1000);        // ...for 1 sec
  }
  noTone(BUZZER_PIN);     // Stop sound...
  delay(1000);        // ...for 1sec

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