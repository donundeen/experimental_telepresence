// IMPORTANT
// before you upload this sketch to your arduino, change this to something unique to this device.
// Whenever the device can't connect to wifi, it will set up it's own access point, with this name,
// that you can connect to to enter the wifi credentials for the space you're in.
#define MY_DEVICE_NAME "jared"

/************************ Adafruit IO Config *******************************/

// visit io.adafruit.com if you need to create an account,
// or if you need your Adafruit IO key.

// Send messages TO this one. This should be YOUR account
#define PUB_IO_USERNAME ""
#define PUB_IO_KEY ""
#define PUB_IO_FEEDNAME "experimental_onoff"

// get messages from this one. This should be YOUR BUDDY'S
#define SUB_IO_USERNAME ""
#define SUB_IO_KEY ""
#define SUB_IO_FEEDNAME "tapping"

