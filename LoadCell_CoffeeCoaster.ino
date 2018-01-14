#include "Arduino.h"
#include "HX711.h"

#define RST 4
#define DOUT  3
#define CLK  2
#define NoCofeeLED 6
#define DrinkUpLED 5
#define delayBetweenFlashes 60000
bool isDebug = false;

// Create our LoadCell object
HX711 scale(DOUT, CLK);

// Calibration magic number for Load Cell
float calibration_factor = -2680.00;
unsigned long time;

// Empty Coffee Cup Timer
unsigned long emptyCoffeeCup;
unsigned long noCoffeeDelayBetweenFlashesMs;

// forward declarations
void pulsateLED(int whichLED, int msDelay = 10, int times = 3);
void showClearEmptyCup(int show = 1);

// ----------------------------------------------------------------------------
// One size fits all Serial Monitor debugging messages
// ----------------------------------------------------------------------------
template<typename T>
void debugPrint(T printMe, bool newLine = false) {
	if (isDebug) {
		if (newLine) {
			Serial.println(printMe);
		}
		else {
			Serial.print(printMe);
		}
		Serial.flush();
	}
}

// --------------------------------------------------------------------------------------
// SET UP	SET UP   SET UP   SET UP   SET UP   SET UP   SET UP   SET UP   SET UP
// --------------------------------------------------------------------------------------
void setup() {
	Serial.begin(9600);
	debugPrint("Coffee Cup Load Cell v1.5.0 19 August 2016", true);

	scale.set_scale();
	scale.tare();  //Reset the scale to 0

	// Reset pin (to re-zero the weight not reset the board!)
	pinMode(RST, INPUT_PULLUP);

	// LED output pins (PWM), initially OFF (HIGH)
	pinMode(NoCofeeLED, OUTPUT);
	digitalWrite(NoCofeeLED, HIGH);

	pinMode(DrinkUpLED, OUTPUT);
	digitalWrite(DrinkUpLED, HIGH);

	// Acknowledge all OK, pulse output a few times
	pulsateLED(DrinkUpLED, 1, 5);

	// Seed the Randomise function otherwise same sequence each run
	randomSeed(analogRead(0));

	// Initialise empty coffee cup timer: [hrs x] mins x secs x ms
	emptyCoffeeCup = millis();
	noCoffeeDelayBetweenFlashesMs = (long) 15 * 60 * 1000;
}

// --------------------------------------------------------------------------------------
// LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP    LOOP     LOOP
// --------------------------------------------------------------------------------------
void loop() {

	// Call the weight measurement routine
	float weight = getWeight();

	debugPrint("Reading: ");
	debugPrint(weight);
	debugPrint(" g", true);

	// If weight above X grams (coffee in cup and on coaster) flash LED every X seconds
	if (weight >= 5.0) {
		debugPrint("Coffee in cup - LED flash?", true);

		// Clear down EmptyCup dim red and CoffeStillInCup dim yellow
		showClearEmptyCup(0);
		showClearCoffeeStillInCup(0);

		if (millis() - time > delayBetweenFlashes) {
			debugPrint("It's been a minute or so - flash LED", true);
			for (int cnt = 0; cnt < 20; cnt++) {
				digitalWrite(DrinkUpLED, !digitalRead(DrinkUpLED));
				delay(100);
			}

			// Update our snapshot time so we start the next minute
			time = millis();
		}
		else if (random(300) < 10) {
			debugPrint("Just a random - flash LED", true);
			for (int cnt = 0; cnt < 2; cnt++) {
				digitalWrite(DrinkUpLED, !digitalRead(DrinkUpLED));
				delay(50);
			}
		}

		// Permanent dim yellow light to show coffee getting cold
		showClearCoffeeStillInCup(1);
	}
	// Just pulsate the RED LED as long as cup still on coaster
	else if (!(weight < -5.0)) {
		if (millis() > emptyCoffeeCup + noCoffeeDelayBetweenFlashesMs) {
			debugPrint("Empty coffee cup, pulsate RED led", true);

			// Clear any permanent reminder levels
			showClearEmptyCup(0);
			showClearCoffeeStillInCup(0);

			pulsateLED(NoCofeeLED, 7, 5);
			emptyCoffeeCup = millis();
			showClearEmptyCup(1);
		}
		else
		// Empty coffee cup but not time for an empty coffee cup reminder yet
		{
			debugPrint("Empty coffee cup - reminder in ");
			debugPrint(((emptyCoffeeCup + noCoffeeDelayBetweenFlashesMs) - millis()) / 1000);
			debugPrint(" seconds", true);

			// Set Empty Cup dim level
			showClearEmptyCup(1);
			showClearCoffeeStillInCup(0);
		}
	}
	// if cup has been lifted off the coaster reset the 1 minute timer
	else {
		// Clear down Empty Cup dim levels
		showClearEmptyCup(0);
		showClearCoffeeStillInCup(0);

		debugPrint("Coffee cup not on coaster - resetting reminder timer", true);
		time = millis();
	}

	// Has user requested a Zero Reset?
	debugPrint("Checking RST request", true);
	int rstRequest = digitalRead(RST);
	if (rstRequest == LOW) {
		debugPrint("Resetting ZERO value - LED flash", true);

		// Clear any permanent reminder dim levels
		showClearEmptyCup(0);
		showClearCoffeeStillInCup(0);

		for (int cnt = 0; cnt < 10; cnt++) {
			digitalWrite(NoCofeeLED, !digitalRead(NoCofeeLED));
			delay(500);
		}
		scale.set_scale();
		scale.tare();
	}
}

// Common weight retrieval routine
float getWeight() {
	//Adjust to this calibration factor
	scale.set_scale(calibration_factor);

	// Read an average of X readings
	debugPrint("Reading weight", true);
	float weight = scale.get_units(5);

	// An intermediate weight value that we round according to some rules
	int netWeight = 0;

	// Make scale more sensitive at lower end
	// Weight > X then just round to nearest integer
	if (weight >= 5.0) {
		netWeight = (weight * 10.0);
		weight = (int) (0.5 + (netWeight / 10.0));
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

	return weight;
}

// LED pulser with slow-down towards end of fade (otherwise stops abruptly)
void pulsateLED(int whichLED, int msDelay, int times) {
// Acknowledge all OK, pulse output 3 times
// NB output is LOW for LED to be ON (Arduino SINKS current here via PWM)
	for (int cnt = 0; cnt < times; cnt++) {

		// Fade UP
		for (int pwm = 255; pwm > 100; pwm--) {
			analogWrite(whichLED, pwm);
			delay(msDelay);
		}

		// Fade DOWN
		for (int pwm = 100; pwm <= 255; pwm++) {
			analogWrite(whichLED, pwm);
			// Delay more at the end of the fade
			delay(msDelay + (pwm > 230 ? 10 : 0));
		}

		// Stop the flashing if the cup is now off the coaster or there is suddenly
		// coffee in the cup
		float currWeight = getWeight();
		if (currWeight > 5.0 || currWeight < -5.0) return;
	}
}

// Low level permanent red if cup is empty
void showClearEmptyCup(int show) {
	analogWrite(NoCofeeLED, show == 1 ? 250 : 255);
}

// Low level permanent yellow if coffee still in cup
void showClearCoffeeStillInCup(int show) {
	analogWrite(DrinkUpLED, show == 1 ? 250 : 255);
}
