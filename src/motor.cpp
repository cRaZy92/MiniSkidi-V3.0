#include "motor.hpp"

#include <Arduino.h>

void Motor::Initialize()
{
  pinMode(pin1, OUTPUT);
  pinMode(pin2, OUTPUT);

  Stop();
};

void Motor::Forward()
{
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, LOW);
};

void Motor::Backward()
{
  digitalWrite(pin1, LOW);
  digitalWrite(pin2, HIGH);
};

void Motor::Stop()
{
  digitalWrite(pin1, LOW);
  digitalWrite(pin2, LOW);
};

void Motor::RemoveMomentum()
{
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, LOW);
  delay(10);
  digitalWrite(pin1, LOW);
  delay(5);
  digitalWrite(pin1, HIGH);
  delay(10);
};
