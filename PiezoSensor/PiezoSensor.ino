const int PIEZO_PIN = A0; // Piezo output
// one pin to ground
// one pin to A0
// a 1M ohm resister btw the gnd and A0 

void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  // Read Piezo ADC value in, and convert it to a voltage
  int piezoADC = analogRead(PIEZO_PIN);
  float piezoV = piezoADC / 1023.0 * 5.0;
  Serial.println(piezoV); // Print the voltage.
  delay(1000);
}