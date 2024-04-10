#ifndef MOTOR_H
#define MOTOR_H

class Motor
{
private:
    int pin1;
    int pin2;

public:
    Motor(int pin1, int pin2) : pin1{pin1}, pin2{pin2} {}

    void Initialize();
    void Forward();
    void Backward();
    void Stop();
    void RemoveMomentum();
};

#endif