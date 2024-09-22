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


unsigned short hangout_timeout_id = 0; 



/*********************************
DEVICE SETUPS
*******************************/

// Another variable you might change:
// prevent holding a recieved HIGH value forever
int HANGOUT_HIGH_MS = 10000; // max amount of time after an ON message is received before we automatically send OFF
                             // 10000ms = 10 seconds


// we might use different types of devices,sensors,etc for input and output
// configure any variables you need for that here.
// just because you set them up doesn't mean you have to use them :)
// Piezo as an input device
const int PIEZO_PIN = A2; // Piezo input. A2 is labelled A2/34 on the arduino. 7 down from top on long side.
                          // 
const int PIEZO_THRESHOLD = 50; // value higher than this triggers ON

//  Capacitive Touch Sensor Code
const int TOUCH_PIN = T9;  // Pin T9 is labelled 32 on the arduino, 4 up from the bottom on the short side. 
                            // one wire going in here then attached to other stuff, is all you need!
const int TOUCH_THRESHOLD = 30; // value lower than this triggers ON

// OUTPUT PIN for ON/OFF values
const int onOffOutputPin = LED_BUILTIN; // this uses the builtin LED for easy testing. But it could be other stuff!


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

  // initialize the onOffOutputPin as an output, and set to LOW
  pinMode(onOffOutputPin, OUTPUT);  
  digitalWrite(onOffOutputPin, LOW);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, LOW);
}

// you need a function that gets called frequently to check on your sensor.
// set that here in setInterval
// probably you should just have one line running here
void setupReadTimers(){
 // t.setInterval(readPiezo, 100); // every 100 ms, run the readPiezo function
  t.setInterval(readTouch, 100); // every 100 ms, run the readTouch function
  //               |        |
} //               |________|
  //               |     // these two names are the same, 
  //      _________|     // so readTouch gets called every 100 milliseconds
  //     |
void readTouch() {// this function readTouch matches the name you set up here
  // read the touchPin, which is the capacitive wire.
  uint32_t touchVal = touchRead(TOUCH_PIN);
  // uncomment this to see the values being produced, and adjust the TOUCH_THRESHOLD accordingly
  Serial.println(touchVal); 

  // we're just going to send an ON or OFF, not in-between values.
  // so if the read touch value is less than TOUCH_THRESHOLD, we sent the send value to 1
  // More touch = lower value.
  int pubVal = 0;
  if(touchVal < TOUCH_THRESHOLD){
    pubVal = 1;
  }
  Serial.println(pubVal);
  // we've got a value, so send it!
  // sendOfOff will make sure it's only sending CHANGED values,
  // and it will enforce a rate limit so you don't maek adafruit.io angry
  sendOnOff(pubVal);
}


// this function is like readTouch above, but it reads a analog value from a Piezo sensor
// if you set this up, change the name of the function in setupReadTimers
void readPiezo(){
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
// I recommend making another function for whatever you want the output to be, 
// then changing the function call in this function from digitalPinOnOff to whatever you make
void setOnOffOutput(int onOff){
  digitalPinOnOff(onOff);
            
}

void digitalPinOnOff(int onOff){
  digitalWrite(onOffOutputPin, onOff); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
                                       // to start off it's just the builtin LED, 
                                       // but it could be a bunch of LEDs, relay, etc....
}


/*******************************************************
You probably don't need to mess with things below here,
but feel free to see how it all works
********************************************************/

// The setup function runs once right when the arduino starts up.
// it sets things up. cuz that's its name
// I like to make a bunch of other setup functions and call them all from here.
void setup() {

  // start the serial connection
  Serial.begin(9600);
  Serial.println("in setup");
  while(! Serial); // wait for Serial to be ready
  setup_devices(); // calls the setup functions for your specific input/outputs devices/sensors
  setup_wifi();    // sets up the WIFI, creates the AP if necessary
  setup_aio();     // sets up connections to adafruit.io
  setup_subscribers(); // sets up connections to the adafruit feeds
  setup_rate_limiter();  // set up the rate limiter that keeps adafruit happy if we get too wild with the sendings
  setup_timers();      // set up the code that makes setInterval work. 
                        // SetInterval is how we can say "call this function every 100 milliseconds" without making the code wait.
  led_flash(LED_BUILTIN, 250,125,5); // flash a light 5 times to say we're all set up!
}




// some things happen on a timer, like "every 100 ms, do such and such." 
// set that up here.
void setup_timers(){
  Serial.println("setup_timers");
  setupReadTimers(); // setup timers for reading sensor values.
}


// set up the rate limiter that keeps adafruit happy if we get too wild with the sendings
// NOTE: I'm not 1000% sure the rate limiter works jsut as it should,
//  so try design your device so it doesn't encourage intense sendingzees
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
// If it doesn't find a wifi it knows how to connect to, 
// it creates its own wifi where you can connect to it and give it new wifi credentials
void setup_wifi(){
  wifiManager.autoConnect(my_device_name);
}

// setup the Adafruit.io connections
// there's two AIO connections:
// io_pub is where you SEND your on-off values
// io_sub is where you LISTEN for your buddy's on-off values
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
  // io_pub/sub.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io_pub.run();
  io_sub.run();
  // t.handle is used for the timer that makes setInterval work.
  t.handle();
}



// send an 1/0 ON/OFF value if it's changed.
// use a rate limiter so you don't anger the adafruit.io server.
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
// this function runs once when the arduino starts up, and is called from the startup function
void setup_subscribers(){
  // handleOnOff is the code that gets called whenever there's a message to the subscriber 
  // IE, when your buddy sends a message, this gets called.
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

  // We don't want to be stuck in ON mode forever, 
  // (imagine if your buddy sent an ON value, but then dropped their device in the bathtub and it broke)
  // so if this message here is ON/1/HIGH, 
  // we are going to set it back to OFF/0/LOW after some amount of time.
  t.cancel(hangout_timeout_id); // cancel existing timeout if there is one.
  hangout_timeout_id = t.setTimeout(autoOff, HANGOUT_HIGH_MS); // you can change HANGOUT_HIGH_MS at the top of the code if you want.
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