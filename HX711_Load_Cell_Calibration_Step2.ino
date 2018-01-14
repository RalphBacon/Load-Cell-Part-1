/*
  Example using the SparkFun HX711 breakout board with a scale
  By: Nathan Seidle
  SparkFun Electronics
  Date: November 19th, 2014
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  This is the calibration sketch. Use it to determine the calibration_factor that the main example uses. It also
  outputs the zero_factor useful for projects that have a permanent mass on the scale in between power cycles.

  Setup your scale and start the sketch WITHOUT a weight on the scale
  Once readings are displayed place the weight on the scale
  Press +/- or a/z to adjust the calibration_factor until the output readings match the known weight
  Use this calibration_factor on the example sketch

  This example assumes pounds (lbs). If you prefer kilograms, change the Serial.print(" lbs"); line to kg. The
  calibration factor will be significantly different but it will be linearly related to lbs (1 lbs = 0.453592 kg).

  Your calibration factor may be very positive or very negative. It all depends on the setup of your scale system
  and the direction the sensors deflect from zero state
  This example code uses bogde's excellent library: https://github.com/bogde/HX711
  bogde's library is released under a GNU GENERAL PUBLIC LICENSE
  Arduino pin 2 -> HX711 CLK
  3 -> DOUT
  5V -> VCC
  GND -> GND

  Most any pin on the Arduino Uno will be compatible with DOUT/CLK.

  The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.

*/

#include "HX711.h"
#define RST 4
#define DOUT  3
#define CLK  2

HX711 scale(DOUT, CLK);

float calibration_factor = -2680.00; //-7050 worked for my 440lb max scale setup
float oldCal_factor;
float oldWeight;

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");

  scale.set_scale();
  scale.tare();  //Reset the scale to 0

  //Get a baseline reading
  long zero_factor = scale.read_average();

  //This can be used to remove the need to tare the scale.
  // Useful in permanent scale projects.
  Serial.print("Zero factor: ");
  Serial.println(zero_factor);

  // Reset pin (to rezero the weight not reset the board!)
  pinMode(RST, INPUT_PULLUP);
}

void loop() {

  //Adjust to this calibration factor
  scale.set_scale(calibration_factor);

  // Read an average of X readings
  float weight = scale.get_units(10);

  // An intermediate weigth value that we round according to some rules
  int netWeight = 0 ;

  // Make scale more sensitive at lower end
  // Weight > X then just round to nearest integer
  if (weight >= 5.0) {
    netWeight = (weight * 10.0);
    weight = (int)(0.5 + (netWeight / 10.0));
  }
  // Weight < Y then call it zero
  else if (weight > -0.01 && weight <= 0.01) {
    weight = 0;
  }
  // Any other weight will have 1 dec. place of precision
  else {
    netWeight = (weight * 10.0);
    weight = (netWeight / 10.0);
  }

  // Only print something out if it has changed (weight or cal. factor)
  if (calibration_factor != oldCal_factor || weight != oldWeight) {
    oldCal_factor = calibration_factor;
    oldWeight = weight;

    Serial.print("Reading: ");
    Serial.print(weight, 2);
    Serial.print(" g");
    Serial.print(" calibration_factor: ");
    Serial.print(calibration_factor);
    Serial.println();
  }

  // User entered +, -, a or z in the Serial Monitor window? Adjust cal. factor
  if (Serial.available())
  {
    char temp = Serial.read();
    if (temp == '+' || temp == 'a')
      calibration_factor += 2;
    else if (temp == '-' || temp == 'z')
      calibration_factor -= 2;
  }

  // Has user requested a Zero Reset?
  int rstRequest = digitalRead(RST);
  if (rstRequest == LOW) {
    Serial.println("Resetting ZERO value");
    scale.set_scale();
    scale.tare();  //Reset the scale to 0
  }
}
