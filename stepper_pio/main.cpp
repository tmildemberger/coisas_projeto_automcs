#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/pio.h"
// criado a partir do .pio:
#include "stepper.pio.h"

#define _USE_MATH_DEFINES
#include <cmath>

void loop(void);
void chegou_int(void);

PIO pio = pio0;
uint sm_step = 0;
uint sm_count = 0;
uint sm_outro = 0;
const uint in1_pin = 18; // outros são 19, 20 e 21

const uint led_pin = PICO_DEFAULT_LED_PIN;

// controle do ímã pelo pino 15
const uint magnet_pin = 15;

int main(void) {
    const uint step_pin = 6;
    const uint dir_pin = 7;
    
    // inicializar pino do LED
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);
    gpio_put(led_pin, 0);

    // inicializar pino do ímã
    gpio_init(magnet_pin);
    gpio_set_dir(magnet_pin, GPIO_OUT);
    gpio_put(magnet_pin, 0);
    
    // inicializa porta serial selecionada no CMakeLists.txt
    stdio_init_all();

    sleep_ms(1000);
    
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
    pio_set_irq0_source_enabled(pio, pio_interrupt_source::pis_interrupt0, true);
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
    // printf("nao\r\n");
    while (f > 180.0f) {
        // printf("pode\r\n");
        f -= 360.0f;
    }
    while (f <= -180.0f) {
        // printf("ser\r\n");
        f += 360.0f;
    }
    // printf("cara\r\n");
    return f;
}

Solucoes duas_solucoes(float x, float y) {
    // printf("teste_eoq\r\n");
    float alpha1 = 180.f*atan2f(y, x)/M_PI;
    float alpha2 = 180.f*acosf( sqrtf(x*x+y*y) / (2*A) )/M_PI;
    float alpha3 = 180.f*acosf( (A22 - x*x - y*y) / (A22) )/M_PI;
    // printf("teste_claro\r\n");
    // printf("x, y = %d, %d\r\n", x, y);
    // printf("a1 = %f\r\n", (double)alpha1);
    // printf("a2 = %f\r\n", (double)alpha2);
    // printf("a3 = %f\r\n", (double)alpha3);
    return (Solucoes) {
        entre_180_e_180(alpha1 + alpha2),
        entre_180_e_180(alpha3 - 180),
        entre_180_e_180(alpha1 - alpha2),
        entre_180_e_180(-alpha3 + 180)
    };
}

const float CASA = 2.4f;
const int CASA_MM = 24;
const int LARGURA = 12;
const int ALTURA = 8;
const float canto_inf_esquerdo_x = -(((float) LARGURA)*CASA/2);
const float canto_inf_esquerdo_y = -(((float) ALTURA)*CASA/2);
const int canto_inf_esquerdo_x_mm = -(LARGURA*CASA_MM/2);
const int canto_inf_esquerdo_y_mm = -(ALTURA*CASA_MM/2);

const float graus_evitar = 45;

const int posicoes[8] = {
    3, 1,  // x = 3, y = 1  => (-7.5, -7.5) cm
    11, 7, // x = 11, y = 7 => (16.5, 10.5) cm
    1, 4,  // x = 1, y = 4  => (-13.5, 1.5) cm
    6, 2,  // x = 6, y = 0  => (1.5, -10.5) cm
};

int bresenham(int x1, int y1, int const x2, int const y2, int *vec, int max);

float transforma_x(int pos_x) {
    return canto_inf_esquerdo_x + CASA/2 + ((float) pos_x)*CASA;
}

float transforma_y(int pos_y) {
    return canto_inf_esquerdo_y + CASA/2 + ((float) pos_y)*CASA;
}

int transforma_x_mm(int pos_x) {
    return canto_inf_esquerdo_x_mm + CASA_MM/2 + pos_x*CASA_MM;
}

int transforma_y_mm(int pos_y) {
    return canto_inf_esquerdo_y_mm + CASA_MM/2 + pos_y*CASA_MM;
}

// https://stackoverflow.com/questions/1878907/how-can-i-find-the-smallest-difference-between-two-angles-around-a-point
float diff_angulos(float a, float b) {
    float diff = a - b;
    diff += (diff>180) ? -360 : (diff<-180) ? 360 : 0;
    return diff;
}

const float overlap_theta2 = 5;

Solucao escolhe_melhor(Solucoes sols, float theta1_atual, float theta2_atual, int tipo_escolha) {
    bool nao_gosto_sol1 = sols.theta1_sol1 >= -90 - graus_evitar && sols.theta1_sol1 <= -90 + graus_evitar;
    bool nao_gosto_sol2 = sols.theta1_sol2 >= -90 - graus_evitar && sols.theta1_sol2 <= -90 + graus_evitar;
    if (nao_gosto_sol1 && nao_gosto_sol2) {
        // espero que seja impossível
        printf("teste_aaaa\r\n");
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
    if (theta2_atual + sol1_diff > 180 + overlap_theta2) {
        sol1_diff -= 360;
    } else if (theta2_atual + sol1_diff < -180 - overlap_theta2) {
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

    if (tipo_escolha == 0) { // escolha pelo menor diff theta2
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
    } else { // escolhe pelo menor diff somado
        printf("teste5\r\n");
        float sol1_t1_diff = diff_angulos(sols.theta1_sol1, entre_180_e_180(theta1_atual));
        float sol2_t1_diff = diff_angulos(sols.theta1_sol2, entre_180_e_180(theta1_atual));
        if ((fabsf(sol1_diff) + fabsf(sol1_t1_diff)) <= (fabsf(sol2_diff) + fabsf(sol2_t1_diff))) {
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
}

// angulo motor maior
float theta1 = 0.0;

// angulo motor pequeno
float theta2 = 0.0;

int x_atual = 89*2;
int y_atual = 0;

// posicao atual
int i = 0;

int posicao_motor_grande(float angulo) {
    float pos = 0.0f;
    float passos_por_volta = 96.0f * 16.0f;
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

int le_numero(void) {
    const int size = 16;
    char str[size];
    int i = 0;
    while (i < (size-1) && (str[i] = getchar()) != '\r') {
        ++i;
    }
    str[i] = '\0';
    return atoi(str);
}

// void anda_em_linha_reta(float diff_angulo_grande, float diff_angulo_pequeno) {
//     if ((diff_angulo_grande >= 0 && diff_angulo_pequeno >= 0) ||
//         (diff_angulo_grande <= 0 && diff_angulo_pequeno <= 0)) {
//         // impossível andar em linha reta com essa situação
//         // (motores têm que girar em sentidos opostos)
//         printf("Erro lógico sentido");
//         while (1) {
//             ;
//         }
//     }
//     if (fabsf(diff_angulo_grande) >= 180 || fabsf(diff_angulo_pequeno) >= 180) {
//         // impossível que os arcos se cancelem se forem maiores que 180
//         printf("Erro lógico tamanho");
//         while (1) {
//             ;
//         }
//     }
    
// }
#define SZ 1024
class Linha {
public:
    int vec[SZ];
    float angs[SZ];
    int i_pronto;
    int max;
    float t1;
    float t2;
    Linha(int *v, int t, float theta1, float theta2) {
        if (t > SZ || (t & 1)) {
            printf("algo diferente pode acontecer");
            while (1) {
                ;
            }
        }
        std::memcpy(vec, v, sizeof (int) * t);
        i_pronto = 2;
        max = t;
        angs[0] = t1 = theta1;
        angs[1] = t2 = theta2;
    }
    void calcula(int n) {
        if (n & 1) {
            printf("eu nao sei usar minha propria função\r\n");
        }
        if (i_pronto + n > max) {
            n = max - i_pronto;
        }
        // printf("inicio_debug\r\n");
        // for (int i = 0; i < 20; ++i) {
        //     printf("vec[%d] = %d;\r\n", i, vec[i]);
        // }
        // printf("fim_debug\r\n");
        for (int i = 0; i < n; i += 2) {
            Solucoes sols = duas_solucoes((float)vec[i_pronto + i]/10.0f, (float)vec[i_pronto + i + 1]/10.0f);
            // printf("teste3\r\n");fflush(stdout);
            Solucao sol = escolhe_melhor(sols, t1, t2, 1);
            printf("teste6\r\n");
            angs[i_pronto + i] = t1 = sol.theta1;
            angs[i_pronto + i + 1] = t2 = sol.theta2;
        }
        i_pronto += n;
    }
    void le_angulo(int idx, float *th1, float *th2) {
        idx = 2 * idx;
        if (idx >= i_pronto) {
            printf("hmm\r\n");
            while (1);
        }
        (*th1) = angs[idx];
        (*th2) = angs[idx + 1];
    }
    void le_pos(int idx, int *p_x, int *p_y) {
        idx = 2 * idx;
        if (idx >= i_pronto) {
            printf("hmm\r\n");
            while (1);
        }
        (*p_x) = vec[idx];
        (*p_y) = vec[idx + 1];
    }
};

void manda_motores_rodarem(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno);

void anda_em_linha_reta(int x, int y) {
    // const int sz = 1024;
    int vec[SZ];
    int tam = bresenham(x_atual, y_atual, x, y, vec, SZ);
    printf("Numero de pontos pelo caminho: %d\r\n", tam);
    Linha l {vec, tam, theta1, theta2};
    printf("teste1\r\n");
    l.calcula(8);
    printf("teste2\r\n");

    int max = tam / 2;
    for (int i = 1; i < max; ++i) {
        // if (i == 2) printf("teste7\r\n");
        float sol_theta1 = 0;
        float sol_theta2 = 0;

        l.le_angulo(i, &sol_theta1, &sol_theta2);

        int posicao_grande = posicao_motor_grande(sol_theta1);

        int dist_grande = posicao_grande - posicao_grande_atual;
        int dist_pequeno = angulo_para_passo_motor_pequeno(sol_theta2 - theta2);

        int dir_grande = (dist_grande > 0) ? 1 : 0;
        int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
        uint32_t n_passos_grande = (uint32_t) abs(dist_grande);
        uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

        manda_motores_rodarem(dir_grande, n_passos_grande, dir_pequeno, n_passos_pequeno);

        int vezes = 2;
        while (!chegou) {
            if (vezes > 0) {
                --vezes;
                l.calcula(2);
            }
        }
        // printf("Movimento terminou\r\n");

        theta1 = sol_theta1;
        theta2 = sol_theta2;

        l.le_pos(i, &x_atual, &y_atual);

        posicao_grande_atual = posicao_grande;
        sleep_ms(1);
    }
    printf("Movimento terminou\r\n");
}

void manda_motores_rodarem(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno) {
    // prepara para rodarem
    uint8_t pc = pio_sm_get_pc(pio, sm_step);
    
    // printf("PC e dir: %d, %d\r\n", pc, dir_pequeno);
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
}

void roda_motores_e_espera(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno) {
    manda_motores_rodarem(dir_grande, n_passos_grande, dir_pequeno, n_passos_pequeno);

    // espera terminarem
    while (!chegou) {
        ;
    }
}

void ir_para_posicao(int pos_x, int pos_y) {
    printf("Começo do movimento para (%d, %d):\r\n", pos_x, pos_y);

    Solucoes sols = duas_solucoes(transforma_x(pos_x), transforma_y(pos_y));
    Solucao sol = escolhe_melhor(sols, theta1, theta2, 0);
    
    printf("Angulos finais %f, %f; Diff %f\r\n", sol.theta1, sol.theta2, sol.estimated_theta2_diff);
    
    int posicao_grande = posicao_motor_grande(sol.theta1);

    int dist_grande = posicao_grande - posicao_grande_atual;
    int dist_pequeno = angulo_para_passo_motor_pequeno(sol.estimated_theta2_diff);

    int dir_grande = (dist_grande > 0) ? 1 : 0;
    int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
    uint32_t n_passos_grande = (uint32_t) abs(dist_grande);
    uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

    roda_motores_e_espera(dir_grande, n_passos_grande, dir_pequeno, n_passos_pequeno);

    printf("Movimento terminou\r\n");

    theta1 = sol.theta1;
    theta2 += sol.estimated_theta2_diff;

    x_atual = transforma_x_mm(pos_x);
    y_atual = transforma_y_mm(pos_y);

    posicao_grande_atual = posicao_grande;
}

void loop(void) {
    if (i % 2) {
        gpio_put(led_pin, 1);
    } else {
        gpio_put(led_pin, 0);
    }

    printf("Esperando comando:\r\n");
    int cmd = le_numero();
    if (cmd == 1) {
        // ligar ima
        printf("Comando ligar ímã\r\n");
        gpio_put(magnet_pin, 1);
    } else if (cmd == 2) {
        // desligar ima
        printf("Comando desligar ímã\r\n");
        gpio_put(magnet_pin, 0);
    } else if (cmd == 3) {
        // comando ir para posição
        printf("Esperando entrada de posição:\r\n");
        int pos_x = le_numero();
        int pos_y = le_numero();
        pos_x = (pos_x >= LARGURA) ? LARGURA - 1 : pos_x;
        pos_y = (pos_y >= ALTURA) ? ALTURA - 1 : pos_y;

        ir_para_posicao(pos_x, pos_y);
    } else {
        // com cmd == 0 ou qualquer outro
        printf("Esperando posição final linha reta:\r\n");
        int pos_x = le_numero();
        int pos_y = le_numero();
        pos_x = (pos_x >= LARGURA) ? LARGURA - 1 : pos_x;
        pos_y = (pos_y >= ALTURA) ? ALTURA - 1 : pos_y;


        anda_em_linha_reta(transforma_x_mm(pos_x), transforma_y_mm(pos_y));
    }

    ++i;

    sleep_ms(4000);
}

int bresenham(int x1, int y1, int const x2, int const y2, int *vec, int max) {
    int delta_x = (x2 - x1);

    int ix = (delta_x > 0) - (delta_x < 0);
    delta_x = abs(delta_x) << 1;

    int delta_y = (y2 - y1);

    int iy = (delta_y > 0) - (delta_y < 0);

    delta_y = abs(delta_y) << 1;

    int i = 0;
    vec[i++] = x1;
    vec[i++] = y1;
    if (i >= max - 2) return i;

    if (delta_x >= delta_y) {
        // error may go below zero
        int error = (delta_y - (delta_x >> 1));

        while (x1 != x2) {
            // reduce error, while taking into account the corner case of error == 0
            if ((error > 0) || (!error && (ix > 0))) {
                error -= delta_x;
                y1 += iy;
            }
            // else do nothing

            error += delta_y;
            x1 += ix;

            vec[i++] = x1;
            vec[i++] = y1;
            if (i >= max - 2) return i;

        }
    }
    else {
        // error may go below zero
        int error = (delta_x - (delta_y >> 1));

        while (y1 != y2) {
            // reduce error, while taking into account the corner case of error == 0
            if ((error > 0) || (!error && (iy > 0))) {
                error -= delta_y;
                x1 += ix;
            }
            // else do nothing

            error += delta_x;
            y1 += iy;

            vec[i++] = x1;
            vec[i++] = y1;
            if (i >= max - 2) return i;
        }
    }
    return i;
}
