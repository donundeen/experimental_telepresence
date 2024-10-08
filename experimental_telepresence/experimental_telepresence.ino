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
const int RATE_LIMIT_TIME = 60000; // within what time frame? 60000ms = 60 seconds = 1 minute (imperial)
bool firstDroppedCallMessageSent = false;
RateLimiter<RATE_LIMIT_TIME, RATE_LIMIT> limiter;


unsigned short hangout_timeout_id = 0; 



/*******************************************************
DEVICE SETUPS

Here's where you can change stuff
******************************************************/

// Another variable you might change:
// prevent holding a recieved HIGH value forever
int HANGOUT_HIGH_MS = 10000; // max amount of time after an ON message is received before we automatically send OFF
                             // 10000ms = 10 seconds




/*****************************************************
INPUT SETUP VARIABLES
******************************************************/
// we might use different types of devices,sensors,etc for input and output
// configure any variables you need for that here.
// just because you set them up doesn't mean you have to use them :)

// Piezo as an input device
const int PIEZO_PIN = A2; // Piezo input. A2 is labelled A2/34 on the arduino. 7 down from top on long side.                          // 
const int PIEZO_THRESHOLD = 50; // value higher than this triggers ON

// Flex as an input device:
const int FLEX_PIN = A2; // Piezo input. A2 is labelled A2/34 on the arduino. 7 down from top on long side.                          // 
const int FLEX_THRESHOLD = 1000; // value higher than this triggers ON

//  Capacitive Touch Sensor Code
const int TOUCH_PIN = T9;  // Pin T9 is labelled 32 on the arduino, 4 up from the bottom on the short side. 
                            // one wire going in here then attached to other stuff, is all you need!
const int TOUCH_THRESHOLD = 30; // value lower than this triggers ON. You might change this!

/*****************************************************
END INPUT SETUP VARIABLES
******************************************************/


/*****************************************************
OUTPUT SETUP VARIABLES
******************************************************/

// MIDI OUTPUT DEVICE SETUP VARIABLES (ONLY if you are using the MusicMaker Shield to play MIDI, not mp3 files)
// Solder closed jumper on bottom!
// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 31
#define VS1053_BANK_DEFAULT 0x00
#define VS1053_BANK_DRUMS1 0x78
#define VS1053_BANK_DRUMS2 0x7F
#define VS1053_BANK_MELODY 0x79

// change this number to other things for different sounds.
// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 32 for more!
#define VS1053_GM1_OCARINA 80 

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0
#if defined(ESP8266) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__)
  #define VS1053_MIDI Serial
#else
  // anything else? use the hardware serial1 port
  #define VS1053_MIDI Serial1
#endif
// END MIDI OUTPUT SETUP


// SERVO SETUP VARIABLES
#include <ESP32Servo.h>
Servo myservo;  // create servo object to control a servo
int SERVO_OUTPUT_PIN = A0; 
/* SERVO SETUP: 
- the brown or black wire goes into GND (first!), 
- the red or orange wire goes into USB
- the other wire goes into A0
*/

// END SERVO SETUP VARIABLES

// OUTPUT PIN for LEDS values
const int LED_OUTPUT_PIN = LED_BUILTIN; // this uses the builtin LED for easy testing. But it could be other stuff!
// if you use other LEDs, make sure to use a resistor along the LED circuit path.

// OUTPUT PIN FOR A RELAY
const int RELAY_OUTPUT_PIN = A0; // A0 pin is labeled A0 on the Ardiuno, 5 down from the top on the long side
/* RELAY SETUP: 
Just plug the RELAY shield into the top of your Arduino. 
The A0 pin is already set up to connect correctly
If this conflicts with inputs you want to use, we'll do some soldering.
*/


const int LATCHING_RELAY_SET_PIN = A0; // a different kind of relay that his different pins for SET (ON) and UNSET (OFF)
const int LATCHING_RELAY_UNSET_PIN = A1; // a different kind of relay that his different pins for SET (ON) and UNSET (OFF)
/* LATCHING RELAY SETUP: 
Just plug the LATCHING RELAY shield into the top of your Arduino. 
The A0 and A1 pins are already set up to connect correctly
If this conflicts with inputs you want to use, we'll do some soldering.
*/

/*****************************************************
END OUTPUT SETUP VARIABLES
******************************************************/



/*****************************************
SETUP DEVICES FUNCTION
*********************************************/

// setup code here for whatever you're using for inputs and outputs
// eg leds, buttons, piezos, etc
// NOTE: not all inputs/outputs NEED a setup
// Find the code segment for your input and output things, and uncomment them.
// make sure the code for other inputs and outputs is commentsed.
void setup_devices(){


  /*******************************
  INPUT DEVICE SETUP
  ********************************/
// not many input devices need setups here.
  /*******************************
  END INPUT DEVICE SETUP
  ********************************/

  /*******************************
  OUTPUT DEVICE SETUP
  ********************************/

  // initialize digital pin LED_BUILTIN as an output.
  // initialize the ledOutputPin as an output, and set to LOW
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, LOW);

  // if you're using an EXTERNAL LED
  ledOutputSetup();

  // if you're using the NON-LATCHING RELAY
  //relayOutputSetup();

  // if you're using the LATCHING RELAY
  //latchingRelayOutputSetup();

  // setup the MIDI player if you're using it
  //midiOutputSetup();

  // setup the SERVO if you're using it
  //servoOutputSetup();

  // send an on and off value to test it
  Serial.println("testing onoff");
  setOnOffOutput(HIGH);
  delay(500);
  setOnOffOutput(LOW);
  Serial.println("DONE testing onoff");

  /*******************************
  END OUTPUT DEVICE SETUP
  ********************************/
}

/*****************************************
SENSOR READ TIMING SETUP
****************************************/
// you need a function that gets called frequently to check on your sensor.
// set that here in setInterval
// probably you should just have one line running here
void setupReadTimers(){
 // t.setInterval(readFlex, 100); // every 100 ms, run the readPiezo function
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
/*****************************************
END SENSOR READ TIMING SETUP
****************************************/



// this function is like readTouch above, but it reads a analog value from a Piezo sensor
// if you set this up, change the name of the function in setupReadTimers
// to make this circuit, connect one wire to PIEZO_PIN, one wire to ground,
// and put a 1M-Ohm resistor between PIEZO_PIN and GROUND
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


// this function is exactly like readPiezo above, but it reads a analog value from a Flex sensor
// to set this up, put one wire coming from the flex sensor into FLEX_PIN,
// the other wire from flex sensor needs to go to power and GND, with a 10k resistor between them.
void readFlex(){
  uint32_t flexVal = analogRead(FLEX_PIN);  
  Serial.println("reading " );
  Serial.println(flexVal);
  int pubVal = 0;
  if(flexVal > FLEX_THRESHOLD){ // flexVal goes UP when you flex it.
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
  //relayOnOff(onOff); // use this if you have a relay attached
  //latchingRelayOnOff(onOff); // use this if you have a relay attached
   ledOnOff(onOff); // use this if you're turning an LED on and off // 
//  midiOnOff(onOff); // use this if you're playing MIDI notes     
//  servoOnOff(onOff);  // use this if you're controlling a SERVO
}


// turning an LED on and off
void ledOnOff(int onOff){
  digitalWrite(LED_OUTPUT_PIN, onOff); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
                                       // to start off it's just the builtin LED, 
                                       // but it could be a bunch of LEDs, relay, etc....
}


// turning a NON-LATCHING RELAY on and off
void relayOnOff(int onOff){
    digitalWrite(RELAY_OUTPUT_PIN, onOff); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
}

// turning a LATCHING RELAY on and off
void latchingRelayOnOff(int onOff){
  if(onOff == 1){
    Serial.println("set");
    digitalWrite(LATCHING_RELAY_SET_PIN, HIGH); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
    digitalWrite(LATCHING_RELAY_UNSET_PIN, LOW); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
    delay(20);
    digitalWrite(LATCHING_RELAY_SET_PIN, LOW); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
  }else{
    
    Serial.println("unset");
    digitalWrite(LATCHING_RELAY_UNSET_PIN, HIGH); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
    digitalWrite(LATCHING_RELAY_SET_PIN, LOW); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 
    delay(20);
    digitalWrite(LATCHING_RELAY_UNSET_PIN, LOW); // this writes 1/0 aka HIGH/LOW  to whatever pin we have here. 

  }
}


// triggering midi notes, if you have the featehr musicMaker attached in MIDI mode
// play with different note combintations to make melodies or random weirdness.
void midiOnOff(int onOff){
  if(onOff == HIGH){
    // when a button ON is sent
    for (uint8_t i=60; i<69; i++) {
      midiNoteOn(0, i, 127);  // midiNoteOn turns a note on. Here, i is the note number, 0 is the channel, and 127 is the volume (127 is max)
      delay(100); // wait some time (in milliseconds)
      midiNoteOff(0, i, 127); // midiNoteOff turns off a note with the same number i
    }
  }else{
    // when a button OFF is sent
    for (uint8_t i=69; i>60; i--) {
      midiNoteOn(0, i, 127);
      delay(100);
      midiNoteOff(0, i, 127);
    }
  }
}


// activating a SERVO, depending on if the sent value is ON(HIGH) or OFF(LOW)
void servoOnOff(int onOff){
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);  
  if(onOff == HIGH){
    myservo.write(0);  // sets the servo position according to the value
    delay(500); 
  }else{
    myservo.write(100); // sets the servo position according to the value
    delay(500);      
  }
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
  Serial.println("wifi connected");
  led_flash(LED_BUILTIN, 100,125,8); // flash a light 8 times fast to say we connected to wifi

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



/*************************************
DEVICE-SPECIFIC HELPER FUNCTIONS
shouldn't need to change anything here...
***************************************/


// MIDI STUFF
void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);  
  delay(10);
  VS1053_MIDI.write(inst);
  delay(10);
}


void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}

void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((uint8_t)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}

void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}

void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}
// END MIDI STUFF


// other device output setup stuff

void relayOutputSetup(){
  // if you're using the NON-LATCHING RELAY
  // initialize the RELAY_OUTPUT_PIN as an output and set to low
  pinMode(RELAY_OUTPUT_PIN, OUTPUT);  
  digitalWrite(RELAY_OUTPUT_PIN, LOW);
}

void latchingRelayOutputSetup(){
  // if you're using the LATCHING RELAY
  // initialize the LATCHING_RELAY_SET_PIN as an output and set to low
  // initialize the LATCHING_RELAY_UNSET_PIN as an output and set to high, then low
  pinMode(LATCHING_RELAY_SET_PIN, OUTPUT);  
  pinMode(LATCHING_RELAY_UNSET_PIN, OUTPUT);  
  digitalWrite(LATCHING_RELAY_SET_PIN, LOW);
  digitalWrite(LATCHING_RELAY_UNSET_PIN, HIGH);
  delay(20);
  digitalWrite(LATCHING_RELAY_UNSET_PIN, LOW);
}

void servoOutputSetup(){
  myservo.setPeriodHertz(50);// Standard 50hz servo
  myservo.attach(SERVO_OUTPUT_PIN, 500, 2400);   // attaches the servo on pin SERVO_OUTPUT_PIN to the servo object
                                         // using SG90 servo min/max of 500us and 2400us
                                         // for MG995 large servo, use 1000us and 2000us,
                                         // which are the defaults, so this line could be
                                         // "myservo.attach(servoPin);"
}

void midiOutputSetup(){
  VS1053_MIDI.begin(31250); // MIDI uses a 'strange baud rate'
  midiSetChannelBank(0, VS1053_BANK_MELODY);
  midiSetChannelVolume(0, 127);
  midiSetInstrument(0, VS1053_GM1_OCARINA); // this number coudl be different for different tones
}

void ledOutputSetup(){
    // initialize the LED_OUTPUT_PIN as an output, and set to LOW
  // it might be the same as LED_BUILTIN, but maybe not...
  pinMode(LED_OUTPUT_PIN, OUTPUT);  
  digitalWrite(LED_OUTPUT_PIN, LOW);
}
