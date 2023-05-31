#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
// criado a partir do .pio:
#include "stepper.pio.h"

#define _USE_MATH_DEFINES
#include <math.h>

void loop(void);
void chegou_int(void);

PIO pio = pio0;
uint sm_step = 0;
uint sm_count = 0;
uint sm_outro = 0;
const uint in1_pin = 18; // outros são 19, 20 e 21

const uint led_pin = PICO_DEFAULT_LED_PIN;

int main(void) {
    const uint step_pin = 6;
    const uint dir_pin = 7;
    
    // inicializar pino do LED
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 0);
    
    // inicializa porta serial selecionada no CMakeLists.txt
    stdio_init_all();

    // sleep_ms(1000);
    
    printf("Começa configuração\r\n");

    pio_add_program_at_offset(pio, &stepper_step_program, 0);
    sm_step = pio_claim_unused_sm(pio, true);
    stepper_step_program_preinit(pio, sm_step, 0, in1_pin);
    stepper_step_program_init(pio, sm_step, 0, in1_pin, 0);

    uint offset_count = pio_add_program(pio, &stepper_count_program);
    sm_count = pio_claim_unused_sm(pio, true);
    stepper_count_program_init(pio, sm_count, offset_count);

    uint offset_outro = pio_add_program(pio, &stepper_outro_program);
    sm_outro = pio_claim_unused_sm(pio, true);
    stepper_outro_program_init(pio, sm_outro, offset_outro, step_pin, dir_pin);

    // interrupção para quando chegarem no lugar
    pio_interrupt_clear(pio, 0);
    pio_set_irq0_source_enabled(pio, PIO_INTR_SM0_LSB, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, chegou_int);
    irq_set_enabled(PIO0_IRQ_0, true);

    printf("Fim da configuração\r\n");
    // espera ligar na tomada
    sleep_ms(8000);

    while (1) {
        loop();
    }
}

////////////////////////////////////////////////////////////////////////////////

// copiado do .ino

typedef struct solucoes {
    float theta1_sol1;
    float theta2_sol1;
    float theta1_sol2;
    float theta2_sol2;
} Solucoes;

typedef struct solucao {
    float theta1;
    float theta2;
    float estimated_theta2_diff;
} Solucao;

#define A (8.9f)
#define A2 (76.21f)
#define A22 (158.42f)

float entre_180_e_180(float f) {
    while (f > 180.0f) {
        f -= 360.0f;
    }
    while (f <= -180.0f) {
        f += 360.0f;
    }
    return f;
}

Solucoes duas_solucoes(float x, float y) {
    float alpha1 = 180.f*atan2f(y, x)/M_PI;
    float alpha2 = 180.f*acosf( sqrtf(x*x+y*y) / (2*A) )/M_PI;
    float alpha3 = 180.f*acosf( (A22 - x*x - y*y) / (A22) )/M_PI;
    return (Solucoes) {
        entre_180_e_180(alpha1 + alpha2),
        entre_180_e_180(alpha3 - 180),
        entre_180_e_180(alpha1 - alpha2),
        entre_180_e_180(-alpha3 + 180)
    };
}

const float CASA = 2.4f;
const int LARGURA = 12;
const int ALTURA = 8;
const float canto_inf_esquerdo_x = -(((float) LARGURA)*CASA/2);
const float canto_inf_esquerdo_y = -(((float) ALTURA)*CASA/2);

const float graus_evitar = 45;

const int posicoes[8] = {
    3, 1,  // x = 3, y = 1  => (-7.5, -7.5) cm
    11, 7, // x = 11, y = 7 => (16.5, 10.5) cm
    1, 4,  // x = 1, y = 4  => (-13.5, 1.5) cm
    6, 2,  // x = 6, y = 0  => (1.5, -10.5) cm
};

float transforma_x(int pos_x) {
    return canto_inf_esquerdo_x + CASA/2 + ((float) pos_x)*CASA;
}

float transforma_y(int pos_y) {
    return canto_inf_esquerdo_y + CASA/2 + ((float) pos_y)*CASA;
}

// https://stackoverflow.com/questions/1878907/how-can-i-find-the-smallest-difference-between-two-angles-around-a-point
float diff_angulos(float a, float b) {
    float diff = a - b;
    diff += (diff>180) ? -360 : (diff<-180) ? 360 : 0;
    return diff;
}

const float overlap_theta2 = 5;

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

    // para solução 1

    // theta2_atual pode estar fora de (-180, 180]
    float sol1_diff = diff_angulos(sols.theta2_sol1, entre_180_e_180(theta2_atual));
    if (theta2_atual + sol1_diff > 270 + overlap_theta2) {
        sol1_diff -= 360;
    } else if (theta2_atual + sol1_diff < -90 - overlap_theta2) {
        sol1_diff += 360;
    }
    
    // fim solução 1
    
    // para solução 2
    float sol2_diff = diff_angulos(sols.theta2_sol2, entre_180_e_180(theta2_atual));
    if (theta2_atual + sol2_diff > 180 + overlap_theta2) {
        sol2_diff -= 360;
    } else if (theta2_atual + sol2_diff < -180 - overlap_theta2) {
        sol2_diff += 360;
    }
    // fim solução 2

    if (fabsf(sol1_diff) <= fabsf(sol2_diff)) {
        return (Solucao) {
            sols.theta1_sol1,
            sols.theta2_sol1,
            sol1_diff,
        };
    } else {
        return (Solucao) {
            sols.theta1_sol2,
            sols.theta2_sol2,
            sol2_diff,
        };
    }
}

// angulo motor maior
float theta1 = 0.0;

// angulo motor pequeno
float theta2 = 0.0;

// posicao atual
int i = 0;

int posicao_motor_grande(float angulo) {
    float pos = 0.0f;
    float passos_por_volta = 96.0f * 2.0f;
    if (angulo > -180.0f && angulo <= -90 - graus_evitar) {
        pos = (angulo + 360.0f) * passos_por_volta / 360.0f;
    } else if (angulo >= -90 - graus_evitar && angulo <= -90 + graus_evitar) {
        // erro impossível?
        while (1);
    } else {
        pos = angulo * passos_por_volta / 360.0f;
    }
    return (int) roundf(pos);
}

int angulo_para_passo_motor_pequeno(float angulo) {
    return (int) roundf(angulo * 2048.0f / 360.0f);
}

volatile bool chegou = false;

void chegou_int(void) {
    pio_interrupt_clear(pio, 0);
    chegou = true;
}

// dir = 1 -> anti-horário
// dir = 0 -> horário

int posicao_grande_atual = 0;

int le_posicao(void) {
    const int size = 16;
    char str[size];
    int i = 0;
    while (i < (size-1) && (str[i] = getchar()) != '\r') {
        ++i;
    }
    str[i] = '\0';
    return atoi(str);
}

void loop(void) {
    if (i >= 8) {
        i = 0;
    }

    if (i % 4) {
        gpio_put(led_pin, 1);
    } else {
        gpio_put(led_pin, 0);
    }
    
    int pos_x = posicoes[i];
    int pos_y = posicoes[i+1];


    printf("Esperando entrada de posição:\r\n");
    pos_x = le_posicao();
    pos_y = le_posicao();
    pos_x = (pos_x >= LARGURA) ? LARGURA - 1 : pos_x;
    pos_y = (pos_y >= ALTURA) ? ALTURA - 1 : pos_y;
    printf("Começo do movimento para (%d, %d):\r\n", pos_x, pos_y);

    Solucoes sols = duas_solucoes(transforma_x(pos_x), transforma_y(pos_y));
    Solucao sol = escolhe_melhor(sols, theta1, theta2);
    
    printf("Angulos finais %f, %f; Diff %f\r\n", sol.theta1, sol.theta2, sol.estimated_theta2_diff);
    
    int posicao_grande = posicao_motor_grande(sol.theta1);

    int dist_grande = posicao_grande - posicao_grande_atual;
    int dist_pequeno = angulo_para_passo_motor_pequeno(sol.estimated_theta2_diff);

    int dir_grande = (dist_grande > 0) ? 1 : 0;
    int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
    uint32_t n_passos_grande = (uint32_t) abs(dist_grande);
    uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

    // prepara para rodarem
    uint8_t pc = pio_sm_get_pc(pio, sm_step);
    
    printf("PC e dir: %d, %d\r\n", pc, dir_pequeno);
    if (pc < 8 && dir_pequeno == 0) {
        uint novo_pc = 0;
        // vai trocar de dir = 1 para 0
        switch (pc) {
            case 0:
            case 7:
                novo_pc = 12;
                break;
            case 1:
            case 2:
                novo_pc = 10;
                break;
            case 3:
            case 4:
            default:
                novo_pc = 8;
                break;
            case 5:
            case 6:
                novo_pc = 14;
                break;
        }
        stepper_step_program_init(pio, sm_step, 0, in1_pin, novo_pc);
    } else if (pc >= 8 && dir_pequeno == 1) {
        uint novo_pc = 0;
        // vai trocar de dir = 0 para 1
        switch (pc) {
            case 15:
            case 8:
                novo_pc = 4;
                break;
            case 9:
            case 10:
                novo_pc = 2;
                break;
            case 11:
            case 12:
            default:
                novo_pc = 0;
                break;
            case 13:
            case 14:
                novo_pc = 6;
                break;
        }
        stepper_step_program_init(pio, sm_step, 0, in1_pin, novo_pc);
    }

    chegou = false;
    // faz começarem a rodar
    pio_sm_put_blocking(pio, sm_outro, (n_passos_grande << 1) | (1 & ((uint32_t) dir_grande)));
    pio_sm_put_blocking(pio, sm_count, n_passos_pequeno);

    // espera terminarem
    while (!chegou) {
        ;
    }

    printf("Movimento terminou\r\n");

    theta1 = sol.theta1;
    theta2 += sol.estimated_theta2_diff;

    posicao_grande_atual = posicao_grande;

    i += 2;

    sleep_ms(4000);
}