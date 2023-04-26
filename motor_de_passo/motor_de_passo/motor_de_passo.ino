#include <AccelStepper.h>
#include <MultiStepper.h>

const int stepPin = 5;
const int dirPin = 4;
AccelStepper myStepper(AccelStepper::DRIVER, stepPin, dirPin);

const int resetPin = 6;
const int sleepPin = 7;

void setup() {
  pinMode(resetPin, OUTPUT);
  pinMode(sleepPin, OUTPUT);

  digitalWrite(resetPin, HIGH);
  digitalWrite(sleepPin, HIGH);
  // put your setup code here, to run once:
  myStepper.setMaxSpeed(40.0f);
  myStepper.setAcceleration(120.0);
  delay(3000);
}

// int multi = 1;

void loop() {
  // put your main code here, to run repeatedly:
  myStepper.move(100);
  //multi *= -1;
  myStepper.runToPosition();
  delay(2000);
}
