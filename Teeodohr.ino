#include "MelodyPlayer.h"
#include <Servo.h>
#include "MelodyPlayer.h"
#include "EEPROM.h"

// Used Pins
static const uint8_t LED1 = 3;
static const uint8_t LED2 = 5;
static const uint8_t LED3 = 6;
static const uint8_t PIEZO = 10;
static const uint8_t SERVO = 11;
static const uint8_t BUTTON_TIME = 9;
static const uint8_t BUTTON_START_STOP = 8;

// Servo SG90 calibration
static const int SERVO_MIN_PULSE = 670;
static const int SERVO_MAX_PULSE = 2420;

// Positioning
static const int EAR_UP_POSITION = 60;
static const int EAR_MIDDLE_POSITION = 80;
static const int EAR_DOWN_POSITION = 115;

// Durations
static const uint8_t DURATIONS[] = { 3, 5, 7, 8, 10, 12, 15 };
static const uint8_t DEFAULT_DURATION_IDX = 1;
static const uint8_t SHAKE_INTERVAL = 1;

// Melodies
Tone bootMelody[] = { { NOTE_E7, 12 }, { NOTE_E7, 12 }, { PAUSE, 12 }, { NOTE_E7, 12 }, { PAUSE, 12 }, { NOTE_C7, 12 }, { NOTE_E7, 12 }, { PAUSE, 12 }, {NOTE_G7, 12}, {PAUSE,  12}, {PAUSE, 6} };
Tone startMelody[] = { { NOTE_B7, 12 },{ NOTE_E8, 3 } };
Tone endMelody[] = { { NOTE_E7, 8 },{ NOTE_G7, 8 },{ NOTE_E8, 8 },{ NOTE_C8, 8 },{ NOTE_D8, 8 },{ NOTE_G8, 8 },{ PAUSE, 4 } };

// Persistence
static const int EEPROM_ADDRESS_DURATION_IDX = 0;

Servo Ear;
MelodyPlayer melodyPlayer(PIEZO);
uint8_t currentDurationIdx = DEFAULT_DURATION_IDX;

void setup()
{
	pinMode(LED1, OUTPUT);
	pinMode(LED2, OUTPUT);
	pinMode(LED3, OUTPUT);
	pinMode(PIEZO, OUTPUT);
	pinMode(SERVO, OUTPUT);
	pinMode(BUTTON_TIME, INPUT_PULLUP);
	pinMode(BUTTON_START_STOP, INPUT_PULLUP);

	readCurrentDurationFromEeprom();
	showCurrentDuration();
	melodyPlayer.play(bootMelody);

	Ear.attach(SERVO, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
	Ear.write(EAR_UP_POSITION);
}

void loop()
{
	if (isButtonPressed(BUTTON_TIME))
	{
		switchDuration();
		showCurrentDuration();
		waitForButtonRelease(BUTTON_TIME);
	}

	if (isButtonPressed(BUTTON_START_STOP))
	{
		melodyPlayer.play(startMelody);
		updateCurrentDurationInEeprom();
		makeTea();
		melodyPlayer.play(endMelody);
		showCurrentDuration();
	}
}

void readCurrentDurationFromEeprom()
{
	uint8_t readValue = EEPROM.read(EEPROM_ADDRESS_DURATION_IDX);

	const bool valueIsValid = readValue < sizeof(DURATIONS);
	currentDurationIdx = (valueIsValid) ? readValue : DEFAULT_DURATION_IDX;
}

void updateCurrentDurationInEeprom()
{
	EEPROM.update(EEPROM_ADDRESS_DURATION_IDX, currentDurationIdx);
}

bool isButtonPressed(uint8_t buttonPin)
{
	return !digitalRead(buttonPin);
}

void waitForButtonRelease(uint8_t buttonPin)
{
	static const int buttonBouncingTime_ms = 200;

	do
	{
		delay(buttonBouncingTime_ms);
	} while (!digitalRead(buttonPin));
}

void switchDuration()
{
	currentDurationIdx = (currentDurationIdx + 1) % sizeof(DURATIONS);
}

void showCurrentDuration()
{
	const uint8_t led1State = (currentDurationIdx == 0 || currentDurationIdx == 3 || currentDurationIdx == 4 || currentDurationIdx == 6) ? HIGH : LOW;
	const uint8_t led2State = (currentDurationIdx == 1 || currentDurationIdx == 3 || currentDurationIdx == 5 || currentDurationIdx == 6) ? HIGH : LOW;
	const uint8_t led3State = (currentDurationIdx == 2 || currentDurationIdx == 4 || currentDurationIdx == 5 || currentDurationIdx == 6) ? HIGH : LOW;

	digitalWrite(LED3, led3State);
	digitalWrite(LED2, led2State);
	digitalWrite(LED1, led1State);
}

void makeTea()
{
	moveEarTo(EAR_DOWN_POSITION);

	const unsigned long startTime = millis();
	const unsigned long stopTime = startTime + currentDuration_ms();
	unsigned long nextShakeTime = startTime + shakeInterval_ms();

	waitForButtonRelease(BUTTON_START_STOP);

	while (true)
	{
		if (isButtonPressed(BUTTON_START_STOP))
		{
			break;
		}

		const unsigned long currentTime = millis();

		if (currentTime >= stopTime)
		{
			break;
		}

		if (currentTime >= nextShakeTime)
		{
			shake();
			nextShakeTime += shakeInterval_ms();
		}

		const unsigned long elapsedTime = currentTime - startTime;
		showCurrentProgress(elapsedTime);
	}

	moveEarTo(EAR_UP_POSITION);
}

unsigned long currentDuration_ms()
{
	return static_cast<unsigned long>(DURATIONS[currentDurationIdx]) * 60 * 1000;
}

unsigned long shakeInterval_ms()
{
	return static_cast<unsigned long>(SHAKE_INTERVAL) * 60 * 1000;
}

void shake()
{
	// assume Ear is in DOWN_POSITION
	moveEarTo(EAR_MIDDLE_POSITION);
	delay(600);
	moveEarTo(EAR_DOWN_POSITION);
}

void moveEarTo(int newPosition)
{
	const uint8_t timeForStep = 20;
	const uint8_t degreesPerStep = 1; 

	int currentPosition = Ear.read();
	const int delta = newPosition - currentPosition;
	const int8_t increment = (delta > 0) ? degreesPerStep : -degreesPerStep;
	const uint8_t numFullSteps = delta / increment;

	for (uint8_t i = 0; i < numFullSteps; i++)
	{
		currentPosition += increment;
		Ear.write(currentPosition);
		delay(timeForStep);
	}

	Ear.write(newPosition);
	delay(timeForStep);
}

void showCurrentProgress(unsigned long elapsedTime)
{
	const uint8_t halfBrightness = 15;

	const float progressPercentage = static_cast<float>(elapsedTime) / currentDuration_ms();

	if (progressPercentage < 0.165)
	{
		analogWrite(LED1, halfBrightness);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, LOW);
	}
	else if (progressPercentage >= 0.165 && progressPercentage < 0.33)
	{
		digitalWrite(LED1, HIGH);
		digitalWrite(LED2, LOW);
		digitalWrite(LED3, LOW);
	}
	else if (progressPercentage >= 0.33 && progressPercentage < 0.495)
	{
		digitalWrite(LED1, HIGH);
		analogWrite(LED2, halfBrightness);
		digitalWrite(LED3, LOW);
	}
	else if (progressPercentage >= 0.495 && progressPercentage < 0.66)
	{
		digitalWrite(LED1, HIGH);
		digitalWrite(LED2, HIGH);
		digitalWrite(LED3, LOW);
	}
	else if (progressPercentage >= 0.66 && progressPercentage < 0.825)
	{
		digitalWrite(LED1, HIGH);
		digitalWrite(LED2, HIGH);
		analogWrite(LED3, halfBrightness);
	}
	else
	{
		digitalWrite(LED1, HIGH);
		digitalWrite(LED2, HIGH);
		digitalWrite(LED3, HIGH);
	}
}