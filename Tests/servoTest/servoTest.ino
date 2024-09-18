
#include <ESP32Servo.h>

Servo myservo;  // create servo object to control a servo

int servoPin = 18;

void setup() {
  // put your setup code here, to run once:
  myservo.setPeriodHertz(50);// Standard 50hz servo
  myservo.attach(servoPin, 500, 2400);   // attaches the servo on pin 18 to the servo object
                                         // using SG90 servo min/max of 500us and 2400us
                                         // for MG995 large servo, use 1000us and 2000us,
                                         // which are the defaults, so this line could be
                                         // "myservo.attach(servoPin);"
}

void loop() {
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);  
  // put your main code here, to run repeatedly:
  myservo.write(0);                  // sets the servo position according to the scaled value
  delay(500);   
  myservo.write(100);                  // sets the servo position according to the scaled value
  delay(500);   
}
