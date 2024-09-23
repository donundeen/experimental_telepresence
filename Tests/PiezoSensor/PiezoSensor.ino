const int PIEZO_PIN = A0; // Piezo output
// one pin to ground
// one pin to A0
// a 1M ohm resister btw the gnd and A0 

void setup() 
{
  Serial.begin(115200);
}

void loop() 
{
  // Read Piezo ADC value in, and convert it to a voltage
  int piezoADC = analogRead(PIEZO_PIN);
  float piezoV = piezoADC ;
  Serial.println(piezoADC); // Print the voltage.
  delay(100);
}