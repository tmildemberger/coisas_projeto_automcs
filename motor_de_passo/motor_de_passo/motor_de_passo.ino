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

struct Solucoes {
  float theta1_sol1;
  float theta2_sol1;
  float theta1_sol2;
  float theta2_sol2;
};

struct Solucao {
  float theta1;
  float theta2;
};

#define A (12.5f)
#define A2 (156.25f)
#define A22 (312.5f)

float entre_180_e_180(float f) {
  while (f > 180.0f) {
    f -= 360.0f;
  }
  while (f < -180.0f) {
    f += 360.0f;
  }
  return f;
}

Solucoes duas_solucoes(float x, float y) {
  float alpha1 = 180.f*atan2f(y, x)/M_PI;
  float alpha2 = 180.f*acosf( sqrtf(x*x+y*y) / (2*A) )/M_PI;
  float alpha3 = 180.f*acosf( (A22 - x*x - y*y) / (A22) )/M_PI;
  return {
    entre_180_e_180(alpha1 + alpha2),
    entre_180_e_180(alpha3 - 180),
    entre_180_e_180(alpha1 - alpha2),
    entre_180_e_180(-alpha3 + 180)
  };
}

const float CASA = 3.0f;
const int LARGURA = 12;
const int ALTURA = 8;
const float canto_inf_esquerdo_x = -(static_cast<float>(LARGURA)*CASA/2);
const float canto_inf_esquerdo_y = -(static_cast<float>(ALTURA)*CASA/2);

const float graus_evitar = 45;

const int posicoes[8] = {
  3, 1,  // x = 3, y = 1  => (-7.5, -7.5) cm
  11, 7, // x = 11, y = 7 => (16.5, 10.5) cm
  1, 4,  // x = 1, y = 4  => (-13.5, 1.5) cm
  6, 2,  // x = 6, y = 0  => (1.5, -10.5) cm
};

float transforma_x(int pos_x) {
  return canto_inf_esquerdo_x + CASA/2 + static_cast<float>(pos_x)*CASA;
}

float transforma_y(int pos_y) {
  return canto_inf_esquerdo_y + CASA/2 + static_cast<float>(pos_y)*CASA;
}

// https://stackoverflow.com/questions/1878907/how-can-i-find-the-smallest-difference-between-two-angles-around-a-point
float diff_angulos(float a, float b) {
  float diff = a - b;
  diff += (diff>180) ? -360 : (diff<-180) ? 360 : 0;
  return diff;
}

Solucao escolhe_melhor(Solucoes sols, float theta1_atual, float theta2_atual) {
  bool nao_gosto_sol1 = sols.theta1_sol1 >= -90 - graus_evitar && sols.theta1_sol1 <= -90 + graus_evitar;
  bool nao_gosto_sol2 = sols.theta1_sol2 >= -90 - graus_evitar && sols.theta1_sol2 <= -90 + graus_evitar;
  if (nao_gosto_sol1 && nao_gosto_sol2) {
    // espero que seja impossível
    while (1);
  } else if (nao_gosto_sol1) {
    sols.theta1_sol1 = sols.theta1_sol2;
    sols.theta2_sol1 = sols.theta2_sol2;
  } else if (nao_gosto_sol2) {
    sols.theta1_sol2 = sols.theta1_sol1;
    sols.theta2_sol2 = sols.theta2_sol1;
  }

  if (diff_angulos(sols.theta2_sol1, theta2_atual) <= diff_angulos(sols.theta2_sol2, theta2_atual)) {
    return {
      sols.theta1_sol1,
      sols.theta2_sol1,
    };
  } else {
    return {
      sols.theta1_sol2,
      sols.theta2_sol2,
    };
  }
}

// angulo motor maior
float theta1 = 0.0;

// angulo motor pequeno
float theta2 = 0.0;

// posicao atual
int i = 0;

float posicao_motor_grande(float angulo) {
  float pos = 0.0f;
  if (angulo > -180.0f && angulo <= -90 - graus_evitar) {
    pos = (angulo + 360.0f) * 96.0f / 360.0f;
  } else if (angulo >= -90 - graus_evitar && angulo <= -90 + graus_evitar) {
    // erro impossível?
    while (1);
  } else {
    pos = angulo * 96.0f / 360.0f;
  }
  return pos;
}

float posicao_motor_pequeno(float angulo) {
  return angulo * 2048.0f / 360.0f;
}

void setup() {
  // motor_grande -> 96 passos por volta?
  // 6 seg por meia volta -> 48/6 passos por segundo
  // motor_grande.setMaxSpeed(8.33f);
  // motor_grande.setSpeed(8.33f);
  Serial.begin(115200);

  motor_grande.setMaxSpeed(2.0f);
  motor_grande.setSpeed(2.0f);
  motor_grande.setAcceleration(60.0f);
  
  // motor_pequeno -> 2048 passos por volta?
  // 6 seg por meia volta -> 1024/6 passos por segundo??
  motor_pequeno.setMaxSpeed(2*170.67f);
  motor_pequeno.setSpeed(2*170.67f);
  motor_pequeno.setAcceleration(4*800.0f);
  delay(8000);
  
  // motor_grande.moveTo(-25);
  // motor_pequeno.moveTo(512);
  // while (motor_grande.distanceToGo() != 0 || motor_pequeno.distanceToGo() != 0) {
  //   motor_pequeno.runSpeedToPosition();
  //   motor_grande.runSpeedToPosition();
  // }
  
  // motor_grande.moveTo(25);
  // motor_pequeno.moveTo(-1024);
  // while (motor_grande.distanceToGo() != 0 || motor_pequeno.distanceToGo() != 0) {
  //   motor_pequeno.runSpeedToPosition();
  //   motor_grande.runSpeedToPosition();
  // }

  // motor_grande.move(50);
  // motor_pequeno.move(1024);
  // motor_grande.runToPosition();
  // while (motor_pequeno.distanceToGo() != 0)
    // motor_pequeno.runSpeedToPosition();
  // motor_grande.runSpeedToPosition();
  // motor_pequeno.runSpeedToPosition();
}

void loop() {
  if (i >= 8) {
    i = 0;
  }
  
  int pos_x = posicoes[i];
  int pos_y = posicoes[i+1];
  Solucoes sols = duas_solucoes(transforma_x(pos_x), transforma_y(pos_y));
  Solucao sol = escolhe_melhor(sols, theta1, theta2);
  
  motor_grande.moveTo(posicao_motor_grande(sol.theta1));
  motor_pequeno.moveTo(-posicao_motor_pequeno(sol.theta2));
  Serial.println(posicao_motor_grande(sol.theta1));
  Serial.println(-posicao_motor_pequeno(sol.theta2));
  while (motor_grande.distanceToGo() != 0 || motor_pequeno.distanceToGo() != 0) {
    motor_pequeno.runSpeedToPosition();
    motor_grande.runSpeedToPosition();
  }

  theta1 = sol.theta1;
  theta2 = sol.theta2;

  i += 2;

  delay(4000);
}
