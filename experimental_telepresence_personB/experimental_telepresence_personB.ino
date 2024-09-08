/****************** EXPERIMENTAL TELEPRESENCE ********************************/
/****************** FRIENDLY ACTION AT A DISTANCE ****************************/


/************************** Configuration ***********************************/
// edit the config.h tab and enter your Adafruit IO credentials
#include "config.h"

// SETTING THE DEVICE NAME, WHICH IS USED FOR CREATING A WIFI ACCESS POINT
char* my_device_name = MY_DEVICE_NAME;


// TIMING INCLUDES
#include <elapsedMillis.h>
#include <AsyncTimer.h> //https://github.com/Aasim-A/AsyncTimer
AsyncTimer t;

/// set up WIFIManager
// this is the thing that creates a WIFI access point if it can't connect to the internet
#include <WiFiManager.h>  // library https://github.com/tzapu/WiFiManager WiFi Configuration Magic
WiFiManager wifiManager;
#include "WiFiClientSecure.h"
// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;


// DEFINE ADAFRUIT.IO STUFF
#include "AdafruitIO_WiFi.h" // the adafruit.io library, that makes connecting to the adafruit.io MQTT server easy.

// see config.h for setting the Values for PUB_IO_USERNAME, PUB_IO_KEY, SUB_IO_USERNAME, SUB_IO_KEY

// publish your signal to this one
AdafruitIO_WiFi io_pub(PUB_IO_USERNAME, PUB_IO_KEY,"","");         // donundeen@gmail // experimental_onoff
AdafruitIO_Feed *onOffPub = io_pub.feed(PUB_IO_FEEDNAME);  // publish your signal to this one

// listen for signals on this one
AdafruitIO_WiFi io_sub(SUB_IO_USERNAME, SUB_IO_KEY,"",""); // dundeenlcc // tapping -
AdafruitIO_Feed *onOffSub = io_sub.feed(SUB_IO_FEEDNAME);      // listen to this one

// RATE LIMIT STUFF
// https://io.adafruit.com/api/docs/mqtt.html#mqtt-api-rate-limiting
// rate limit is 30 per minute PER ACCOUNT, but we'll set to 25 to give some breathing room
#include "RateLimiter.h"
const int RATE_LIMIT = 25; // how many messages you're allowed
const int RATE_LIMIT_TIME = 60000; // within what time framae?
bool firstDroppedCallMessageSent = false;
RateLimiter<RATE_LIMIT_TIME, RATE_LIMIT> limiter;

// prevent holding a recieved HIGH value forever
int HANGOUT_HIGH_MS = 10000; // max amount of time after an ON message is received before we automatically send OFF
unsigned short hangout_timeout_id = 0; 

/*********************************
DEVICE SETUPS
*******************************/
// we might use different types of devices,sensors,etc for input and output
// configure any variables you need for that here.
// just because you set them up doesn't mean you have to use them :)
// Piezo as an input device
const int PIEZO_PIN = A2; // Piezo input
const int PIEZO_THRESHOLD = 50; // value higher than this triggers ON

//  Capacitive Touch Sensor Code
const int TOUCH_PIN = T9; 
const int TOUCH_THRESHOLD = 30; // value lower than this triggers ON

// OUTPUT PIN for ON/OFF values
const int onOffPin = LED_BUILTIN; // this uses the builtin LED for easy testing. 


/*****************************************
PLAYGROUND FOR READING AND SENDING SENSOR VALUES
- this is where you might play with things.
- what other sensors could you use?
use these functions as a template, 
copy and rename them, eg readFlex or readButton
and explore threshold values if you need them.
*****************************************/

// Depening on what sensors you want to use to read data
// and what functions do that reading,
// you might change the code in here:

// setup code here for whatever you're using for inputs and outputs
// eg leds, buttons, piezos, etc
// NOTE: not all inputs/outputs NEED a setup
void setup_devices(){

  // initialize the onOffPin as an output, and set to LOW
  pinMode(onOffPin, OUTPUT);  
  digitalWrite(onOffPin, LOW);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, LOW);
}

// you need a function that gets called frequently to check on your sensor.
// set that here in setInterval
// probably you should just have one line uncommented here
void setupReadTimers(){
 // t.setInterval(readPiezo, 100); // every 100 ms, run the readPiezo function
  t.setInterval(readTouch, 100); // every 100 ms, run the readTouch function

}

void readTouch(){
  // read the touchPint
  uint32_t touchVal = touchRead(TOUCH_PIN);
  // uncomment this to see the values being produced, and adjust the TOUCH_THRESHOLD accordingly
  //Serial.println(touchVal); 

  // we're just goign to send an ON or OFF, not in-between values.
  // so if the read touch value is less than TOUCH_THRESHOLD, we sent the send value to 1
  // More touch = lower value.
  int pubVal = 0;
  if(touchVal < TOUCH_THRESHOLD){
    pubVal = 1;
  }
  Serial.println(pubVal);
  sendOnOff(pubVal);
}

void readPiezo(){
  // Now we can publish stuff!
  uint32_t piezoVal = analogRead(PIEZO_PIN);  
  Serial.println("reading " );
  Serial.println(piezoVal);
  int pubVal = 0;
  if(piezoVal > PIEZO_THRESHOLD){
    pubVal = 1;
  }
  sendOnOff(pubVal);
}


/********************************************
END PLAYGROUND FOR READING DATA AND PUBLISHING IT
********************************************/

/*****************************************
PLAYGROUND FOR GETTING VALUES AND DOING STUFF WITH IT
- this is where you might play with the values you get.
- what other outputs could you use?
use these functions as a template, 
copy and rename them, eg readFlex or readButton
and explore threshold values if you need them.
*****************************************/

// this function is where you do stuff with the value recieved from Adafruit.io
// it will be either 1 - ON, or 0 - OFF
// you can change the contents here to whatever is cool.
void setOnOffOutput(int onOff){
  digitalWrite(onOffPin, onOff); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
                                  // this could be an LED, relay, etc....

}




/*******************************************************
You probably don't need to mess with things below here
********************************************************/

// The setup function runs once right when the arduino starts up.
// it sets things up. cuz that's its name
void setup() {

  // start the serial connection
  Serial.begin(9600);
  Serial.println("in setup");
  while(! Serial); // wait for Serial to be ready
  setup_devices();
  // wait for serial monitor to open
  setup_wifi();
  setup_aio();
  setup_subscribers();
  setup_rate_limiter();
  setup_timers();  
  led_flash(LED_BUILTIN, 250,125,5);
}




// some things happen on a timer, like "every 100 ms, do such and such." 
// set that up here.
void setup_timers(){
  Serial.println("setup_timers");
  setupReadTimers(); // setup timers for reading sensor values.
}

void setup_rate_limiter(){
  limiter.SetDroppedCallCallback([&](unsigned int dropped_calls){
    Serial.println("rate limited!");
    digitalWrite(LED_BUILTIN, HIGH);
    // if we're ratelimiting, we send just one last value, 0, so the recipient is not left hanging in the ON state
    if(!firstDroppedCallMessageSent){
      firstDroppedCallMessageSent = true;
      Serial.println("dropping calls, sending 0");
      onOffPub->save(0);
    }
  });
}


// setup the WIFI stuff.
void setup_wifi(){
  wifiManager.autoConnect(my_device_name);
}

// setup the Adafruit.io connections
void setup_aio(){
  Serial.print("Connecting to Adafruit IO");

  // start MQTT connection to io.adafruit.com
  io_pub.connect();
  io_sub.connect();
  // wait for an MQTT connection
  // NOTE: when blending the HTTP and MQTT API, always use the mqttStatus
  // method to check on MQTT connection status specifically
  while(io_pub.mqttStatus() < AIO_CONNECTED || io_sub.mqttStatus() < AIO_CONNECTED ) {
    Serial.print(".");
    delay(500);
  }
  // we are connected
  Serial.println();
  Serial.println(io_pub.statusText());
  Serial.println(io_sub.statusText());
}


// this function runs in a loop whenever the arduino is turned on right after the setup function runs.
void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io_pub.run();
  io_sub.run();
  t.handle();
}



// send an 1/0 ON/OFF value if it's changed.
int lastPubValSent = 0;
void sendOnOff(int onOff){
  if(lastPubValSent != onOff){
    limiter.CallOrDrop([&](){   
      // we track if this is the very first time we've send something. 
      firstDroppedCallMessageSent = false;
      Serial.print(F("\nSending val "));
      Serial.print(onOff);  
      onOffPub->save(onOff);
    });
    lastPubValSent = onOff;
  }
}




// setup the code that runs when you get a message on the thing you're subscribed to.
void setup_subscribers(){
  // you might change this code if you want to call a different function when you get a message.
  onOffSub->onMessage(handleOnOff);
  // Because Adafruit IO doesn't support the MQTT retain flag, we can use the
  // get() function to ask IO to resend the last value for this feed to just
  // this MQTT client after the io client is connected.
  onOffSub->get();
  Serial.println("sub connected");

}

// do something when you get the on/off 1/0 value
void handleOnOff(AdafruitIO_Data *data) {
  Serial.print("received <- ");
  Serial.println(data->value());
  int value = data->toInt(); // 1 or 0
  setOnOffOutput(value); // calling this function in the "playground", so users only have to worry about what to do with the on/off value
  // We don't want to be stuck in ON mode forever, so if this message here is ON/1/HIGH, 
  // we are going to set it back to OFF/0/LOW after some amount of time.
  t.cancel(hangout_timeout_id); // cancel existing timeout if there is one.
  hangout_timeout_id = t.setTimeout(autoOff, HANGOUT_HIGH_MS);
}

// set your device to OFF. This is used as a timeout so we don't get stuck on ON
void autoOff(){
  // 
  setOnOffOutput(0);
}

// a little helper pin that flashes the builin LED some number of times.
void led_flash(int pin, int onms, int offms, int times){
  // flash an led a certain number of times. Good for status things.
  for(int i = 0; i<times; i++){
    digitalWrite(pin, HIGH);
    delay(onms);
    digitalWrite(pin, LOW);
    delay(offms);
  }
}