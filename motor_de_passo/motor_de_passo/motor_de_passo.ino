#include <AccelStepper.h>
#include <MultiStepper.h>

const int stepPin = 7;
const int dirPin = 8;
AccelStepper motor_grande(AccelStepper::DRIVER, stepPin, dirPin);

const int in1 = 5;
const int in2 = 4;
const int in3 = 3;
const int in4 = 2;
AccelStepper motor_pequeno(AccelStepper::FULL4WIRE, in1, in3, in2, in4);

// posição inicial: 0°, 0°

struct Solucoes {
  float theta_1_sol1;
  float theta_2_sol1;
  float theta_1_sol2;
  float theta_2_sol2;
};

#define A (12.5f)
#define A2 (156.25f)
#define A22 (312.5f)

Solucoes duas_solucoes(float x, float y) {
  float alpha1 = atan2f(y, x);
  float alpha2 = acosf( sqrtf(x*x+y*y) / (2*A) );
  float alpha3 = acosf( (A22 - x*x - y*y) / (A22) );
  return {
    alpha1 + alpha2,
    alpha3 - 180,
    alpha1 - alpha2,
    -alpha3 + 180
  };
}

// https://stackoverflow.com/questions/28852574/how-to-print-result-of-a-compile-time-calculation-in-c
template <class T, T x, class F>
void transparent(F f) { f(); }


template <bool B>
constexpr void my_assert() { 
    static_assert(B, "oh no");
}

template <int X, int Y>
void f() {
    transparent<int, (int)(180*atan2f(Y,X)/3.14)>([]{
        transparent<long, X*X*X>([]{
            my_assert<X+10==-89>(); });});
}

void setup() {
    f<0, -1>();
  // motor_grande -> 96 passos por volta
  // 6 seg por meia volta -> 48/6 passos por segundo
  // motor_grande.setMaxSpeed(8.33f);
  // motor_grande.setSpeed(8.33f);
  motor_grande.setMaxSpeed(3.0f);
  motor_grande.setSpeed(3.0f);
  motor_grande.setAcceleration(120.0f);
  
  // motor_pequeno -> 2048 passos por volta
  // 6 seg por meia volta -> 1024/6 passos por segundo
  motor_pequeno.setMaxSpeed(2*170.67f);
  motor_pequeno.setSpeed(2*170.67f);
  motor_pequeno.setAcceleration(4*800.0f);
  delay(8000);
  
  motor_grande.moveTo(-25);
  motor_pequeno.moveTo(512);
  while (motor_grande.distanceToGo() != 0 || motor_pequeno.distanceToGo() != 0) {
    motor_pequeno.runSpeedToPosition();
    motor_grande.runSpeedToPosition();
  }
  
  motor_grande.moveTo(25);
  motor_pequeno.moveTo(-1024);
  while (motor_grande.distanceToGo() != 0 || motor_pequeno.distanceToGo() != 0) {
    motor_pequeno.runSpeedToPosition();
    motor_grande.runSpeedToPosition();
  }
  // motor_grande.move(50);
  // motor_pequeno.move(1024);
  // motor_grande.runToPosition();
  // while (motor_pequeno.distanceToGo() != 0)
    // motor_pequeno.runSpeedToPosition();
  // motor_grande.runSpeedToPosition();
  // motor_pequeno.runSpeedToPosition();
}

// int multi = 1;

void loop() {
  // multi *= -1;
  // motor_grande.runToPosition();
  delay(2000);
}
