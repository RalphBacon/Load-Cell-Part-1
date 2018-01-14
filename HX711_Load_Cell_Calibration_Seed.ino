#include "HX711.h"
#define DOUT  3
#define CLK  2
float seed = -2712.95;

HX711 scale(DOUT, CLK);

void setup() {
  Serial.begin(9600);

  scale.set_scale(seed);

  scale.tare();
}

void loop() {
  scale.set_scale(seed);

  if (Serial.available()) {
    char z = Serial.read();

    if (z == '-') seed -= 5;
    if (z == '+') seed += 5;
  }

  Serial.print("Seed value: ");
  Serial.print(seed);
  Serial.print("\tWeight:\t");
  Serial.println(scale.get_units(10), 1);
}
