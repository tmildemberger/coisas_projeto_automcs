#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
// criado a partir do .pio:
#include "stepper.pio.h"

// biblioteca do A*, usado para calcular o caminho da peça
#include "astar-lib/AStar.hpp"

#define _USE_MATH_DEFINES
#include <cmath>

// se for 0, se comunica normalmente com o computador pelo usb
#define ZERO_UART 1

#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

void loop();
void chegou_int();

PIO pio = pio0;
uint sm_step = 0;
uint sm_count = 0;
uint sm_outro = 0;
const uint in1_pin = 18; // outros são 19, 20 e 21

const uint led_pin = PICO_DEFAULT_LED_PIN;

// controle do ímã pelo pino 15
const uint magnet_pin = 15;
bool ima_ligado = false;

int main() {
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
    
    // inicializa uart para comunicação com o zero
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // inicializa porta USB para ser usada com o stdio
    stdio_init_all();
    
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

    while (1) {
        loop();
    }
}

////////////////////////////////////////////////////////////////////////////////

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

#define A (7.0f)
#define A2 (49.0f)
#define A22 (98.0f)

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
    if (x == 0 && y == 0) {
        // nesse caso haveriam infinitas soluções, em particular essas duas
        return (Solucoes) {
            135,
            -180,
            45,
            180
        };
    }
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
const int CASA_MMM = 24/2;
const int LARGURA = 12;
const int ALTURA = 8;
const float canto_inf_esquerdo_x = -(((float) LARGURA)*CASA/2);
const float canto_inf_esquerdo_y = -(((float) ALTURA)*CASA/2);
const int canto_inf_esquerdo_x_mmm = -(LARGURA*CASA_MMM/2);
const int canto_inf_esquerdo_y_mmm = -(ALTURA*CASA_MMM/2);

const float graus_evitar = 45;

int bresenham(int x1, int y1, int const x2, int const y2, int *vec, int max);

float transforma_x(int pos_x) {
    return canto_inf_esquerdo_x + CASA/2 + ((float) pos_x)*CASA;
}

float transforma_y(int pos_y) {
    return canto_inf_esquerdo_y + CASA/2 + ((float) pos_y)*CASA;
}

int transforma_x_mmm(int pos_x) {
    return canto_inf_esquerdo_x_mmm + CASA_MMM/2 + pos_x*CASA_MMM;
}

int transforma_y_mmm(int pos_y) {
    return canto_inf_esquerdo_y_mmm + CASA_MMM/2 + pos_y*CASA_MMM;
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
        // impossível
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

int x_atual = 70;
int y_atual = 0;

int posicao_motor_grande(float angulo) {
    float pos = 0.0f;
    float passos_por_volta = 200.0f * 16.0f;
    if (angulo > -180.0f && angulo <= -90 - graus_evitar) {
        pos = (angulo + 360.0f) * passos_por_volta / 360.0f;
    } else if (angulo >= -90 - graus_evitar && angulo <= -90 + graus_evitar) {
        // erro impossível
        while (1);
    } else {
        pos = angulo * passos_por_volta / 360.0f;
    }
    return (int) truncf(pos);
}

int angulo_para_passo_motor_pequeno(float angulo) {
    return (int) truncf(angulo * 2048.0f / 360.0f);
}

volatile bool chegou = false;

void chegou_int() {
    pio_interrupt_clear(pio, 0);
    chegou = true;
}

// dir = 1 -> anti-horário
// dir = 0 -> horário

int posicao_grande_atual = 0;

int le_numero() {
    const int size = 16;
    char str[size];
    int i = 0;
    while (i < (size-1) && (str[i] = getchar()) != '\r') {
        ++i;
    }
    str[i] = '\0';
    return atoi(str);
}

#define SZ 16384
class Caminho {
public:
    int vec[SZ];
    float angs[SZ];
    int i_pronto;
    int max;
    float t1;
    float t2;
    Caminho(int *v, int t, float theta1, float theta2) {
        if (t > SZ || (t & 1)) {
            while (1);
        }
        std::memcpy(vec, v, sizeof (int) * t);
        i_pronto = 2;
        max = t;
        angs[0] = t1 = theta1;
        angs[1] = t2 = theta2;
    }
    void calcula(int n) {
        if (n & 1) {
            while (1);
        }
        if (i_pronto + n > max) {
            n = max - i_pronto;
        }
        for (int i = 0; i < n; i += 2) {
            Solucoes sols = duas_solucoes((float)vec[i_pronto + i]/5.0f, (float)vec[i_pronto + i + 1]/5.0f);
            Solucao sol = escolhe_melhor(sols, t1, t2, 1);
            angs[i_pronto + i] = t1 = sol.theta1;
            angs[i_pronto + i + 1] = t2 = t2 + sol.estimated_theta2_diff;
        }
        i_pronto += n;
    }
    void le_angulo(int idx, float *th1, float *th2) {
        idx = 2 * idx;
        if (idx >= i_pronto) {
            while (1);
        }
        (*th1) = angs[idx];
        (*th2) = angs[idx + 1];
    }
    void le_pos(int idx, int *p_x, int *p_y) {
        idx = 2 * idx;
        if (idx >= i_pronto) {
            while (1);
        }
        (*p_x) = vec[idx];
        (*p_y) = vec[idx + 1];
    }
};

void manda_motores_rodarem(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno);
void roda_motores_e_espera(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno);
void anda_no_caminho_dado(Caminho& l, int tam);

void anda_em_linha_reta(int x, int y) {
    int vec[SZ];
    int tam = bresenham(x_atual, y_atual, x, y, vec, SZ);
    // printf("Numero de pontos pelo caminho: %d\r\n", tam);
    Caminho l {vec, tam, theta1, theta2};
    anda_no_caminho_dado(l, tam);
    
    // printf("Movimento terminou\r\n");
    // printf("Angulos atuais devem ser %f, %f;\r\n", theta1, theta2);
}


void anda_no_caminho_dado(Caminho& l, int tam) {
    l.calcula(tam + 20);

    int max = tam / 2;
    for (int i = 1; i < max; ++i) {
        float sol_theta1 = 0;
        float sol_theta2 = 0;

        l.le_angulo(i, &sol_theta1, &sol_theta2);

        int posicao_grande = posicao_motor_grande(sol_theta1);

        // vê se teve que desviar de zona proibida
        if (fabsf(sol_theta1 - theta1) > 180 || fabsf(sol_theta2 - theta2) > 180) {
            // se é o caso, para tentar evitar de bater nas bordas, leva
            // ima para o centro, gira motor grande, e depois posiciona motor pequeno

            // tudo isso com ima desligado, caso esteja ligado
            if (ima_ligado) {
                gpio_put(magnet_pin, 0);
                // espera um pouco para desligar
                sleep_ms(800);
            }

            float diff_centro_pequeno = 0;
            float aux_theta1 = entre_180_e_180(theta1);
            if (aux_theta1 >= 0) {
                diff_centro_pequeno = (theta2 <= 90 - aux_theta1) ? (-180 - theta2) : (180 - theta2);
            } else {
                diff_centro_pequeno = (theta2 <= -90 - aux_theta1) ? (-180 - theta2) : (180 - theta2);
            }

            int dir_centro_pequeno = (diff_centro_pequeno > 0) ? 1 : 0;
            uint32_t passos_centro_pequeno = (uint32_t) abs(diff_centro_pequeno);

            roda_motores_e_espera(1, 0, dir_centro_pequeno, passos_centro_pequeno);
            sleep_ms(20);

            // agora roda motor grande
            int dist_grande = posicao_grande - posicao_grande_atual;
            int dir_grande = (dist_grande > 0) ? 1 : 0;
            uint32_t n_passos_grande = (uint32_t) abs(dist_grande);

            roda_motores_e_espera(dir_grande, n_passos_grande, dir_centro_pequeno, 0);
            sleep_ms(20);

            // termina rodando motor menor
            float aux_theta2 = diff_centro_pequeno + theta2;
            int dist_pequeno = angulo_para_passo_motor_pequeno(sol_theta2 - aux_theta2);
            int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
            uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

            roda_motores_e_espera(dir_grande, 0, dir_pequeno, n_passos_pequeno);
            sleep_ms(20);

            if (ima_ligado) {
                gpio_put(magnet_pin, 1);
                // espera um pouco para ligar
                sleep_ms(1200);
            }
        } else {

            int dist_grande = posicao_grande - posicao_grande_atual;
            int dist_pequeno = angulo_para_passo_motor_pequeno(sol_theta2 - theta2);

            int dir_grande = (dist_grande > 0) ? 1 : 0;
            int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
            uint32_t n_passos_grande = (uint32_t) abs(dist_grande);
            uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

            manda_motores_rodarem(dir_grande, n_passos_grande, dir_pequeno, n_passos_pequeno);

            while (!chegou) {
                ;
            }
            sleep_ms(20);
        }


        theta1 = sol_theta1;
        theta2 = sol_theta2;

        l.le_pos(i, &x_atual, &y_atual);

        posicao_grande_atual = posicao_grande;
    }
}

void manda_motores_rodarem(int dir_grande, uint32_t n_passos_grande, int dir_pequeno, uint32_t n_passos_pequeno) {
    // prepara para rodarem
    uint8_t pc = pio_sm_get_pc(pio, sm_step);
    
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

    x_atual = transforma_x_mmm(pos_x);
    y_atual = transforma_y_mmm(pos_y);

    posicao_grande_atual = posicao_grande;
}

int vec[16384];

void loop() {

#if ZERO_UART == 0
    printf("Esperando comando:\r\n");
    int cmd = le_numero();
    printf("Cmd %d:\r\n", cmd);
    if (cmd == 1) {
        // ligar ima
        printf("Comando ligar ímã\r\n");
        gpio_put(magnet_pin, 1);
        ima_ligado = true;
    } else if (cmd == 2) {
        // desligar ima
        printf("Comando desligar ímã\r\n");
        gpio_put(magnet_pin, 0);
        ima_ligado = false;
    } else if (cmd == 3) {
        // comando ir para posição
        printf("Esperando entrada de posição:\r\n");
        int pos_x = le_numero();
        int pos_y = le_numero();
        pos_x = (pos_x >= LARGURA) ? LARGURA - 1 : pos_x;
        pos_y = (pos_y >= ALTURA) ? ALTURA - 1 : pos_y;

        ir_para_posicao(pos_x, pos_y);
    } else if (cmd == 4) {
        printf("Esperando posição final linha reta:\r\n");
        int pos_x = le_numero();
        int pos_y = le_numero();
        pos_x = (pos_x >= LARGURA) ? LARGURA - 1 : pos_x;
        pos_y = (pos_y >= ALTURA) ? ALTURA - 1 : pos_y;


        anda_em_linha_reta(transforma_x_mmm(pos_x), transforma_y_mmm(pos_y));
    } else if (cmd == 5) {
        printf("Esperando delta ângulos:\r\n");
        int d_1 = le_numero();
        int d_2 = le_numero();
        float diff_t1 = entre_180_e_180((float) d_1);
        float diff_t2 = entre_180_e_180((float) d_2);

        printf("Angulos finais %f, %f;\r\n", theta1 + diff_t1, theta2 + diff_t2);
    
        int posicao_grande = posicao_motor_grande(theta1 + diff_t1);
        printf("Posição final grande: %d\r\n", posicao_grande);

        int dist_grande = posicao_grande - posicao_grande_atual;
        printf("Passos motor grande: %d\r\n", dist_grande);
        int dist_pequeno = angulo_para_passo_motor_pequeno(diff_t2);

        int dir_grande = (dist_grande > 0) ? 1 : 0;
        int dir_pequeno = (dist_pequeno > 0) ? 1 : 0;
        uint32_t n_passos_grande = (uint32_t) abs(dist_grande);
        uint32_t n_passos_pequeno = (uint32_t) abs(dist_pequeno);

        roda_motores_e_espera(dir_grande, n_passos_grande, dir_pequeno, n_passos_pequeno);

        printf("Movimento terminou\r\n");

        theta1 += diff_t1;
        theta2 += diff_t2;

        // x_atual = transforma_x_mmm(pos_x);
        // y_atual = transforma_y_mmm(pos_y);

        posicao_grande_atual = posicao_grande;
    } else {
        printf("Comando não identificado;\r\n");
    }

    sleep_ms(4000);
#else
    uint8_t buf[512];
    // uma mensagem enviada pelo Raspberry Pi Zero tem tamanho 64+1+2+2+2+2+2+2 = 77
    // composto por:
    // 64 bytes '0' ou '1' indicando se a posição possui peça ou não
    // 1 separador '|'
    // 1 byte + separador '|' para coordenada x da peça a ser movida (entre 0 e 11)
    // 1 byte + separador '|' para coordenada y da peça a ser movida (entre 0 e 7)
    // 1 byte + separador '|' para coordenada x da peça a ser movida (em mm, entre 0 e 255)
    // 1 byte + separador '|' para coordenada y da peça a ser movida (em mm, entre 0 e 255)
    // 1 byte + separador '|' para coordenada x do destino da peça (entre 0 e 11)
    // 1 byte + separador '|' para coordenada y do destino da peça (entre 0 e 7)
    uart_read_blocking(UART_ID, buf, 77);
    // roque acontece em 3 momentos:
    // mandam tirar torre
    // mandam mover rei
    // mandam recolocar torre

    AStar::Generator generator;
    generator.setWorldSize({12, 10});
    generator.setHeuristic(AStar::Heuristic::euclidean);
    generator.setDiagonalMovement(true);

    // adiciona "paredes" onde há peças
    for (int i = 0; i < 64; ++i) {
        if (buf[i] == 1) {
            generator.addCollision({2+(i%8), 1+(i/8)});
        }
    }

    // adiciona também paredes padrão (casas fora de alcance ou inativas)
    for (int i = 0; i < 10; ++i) {
        generator.addCollision({0, i});
        generator.addCollision({11, i});
        if (i <= 1 || i >= 8) {
            generator.addCollision({1, i});
            generator.addCollision({10, i});
        }
        if (i == 0 || i == 9) {
            generator.addCollision({2, i});
            generator.addCollision({9, i});
        }
    }

    const uint8_t st_x = buf[65];
    const uint8_t st_y = buf[67] - 1;
    const uint8_t st_x_mmm = buf[69] / 2;
    const uint8_t st_y_mmm = buf[71] / 2;
    const uint8_t ed_x = buf[73];
    const uint8_t ed_y = buf[75] - 1;

    auto path = generator.findPath({st_x, st_y}, {ed_x, ed_y});

    if (path.size() == 0) {
        if (st_x != ed_x && st_y != ed_y) {
            // não há caminho, tem que tirar algum da frente
            printf("Não encontrou caminho;\r\n");
            while (1);
        } else {
            // movimento para a mesma coordenada
            // pode ajustar a peça, ou fazer nada
        }
    } else {
        // monta caminho:
        // utiliza o "int vec[16384];"

        // primeiro vai com ima desligado até inicio
        if (ima_ligado) {
            gpio_put(magnet_pin, 0);
            ima_ligado = false;
            sleep_ms(500);
        }

        int ix = 0;
        anda_em_linha_reta(st_x_mmm, st_y_mmm);

        // liga ima e calcula
        gpio_put(magnet_pin, 1);
        ima_ligado = true;

        int calc_x_atual = st_x_mmm;
        int calc_y_atual = st_y_mmm;
        for (auto& coordinate : path) {
            int proximo_x = transforma_x_mmm(coordinate.x);
            int proximo_y = transforma_y_mmm(coordinate.y);
            int tam = bresenham(calc_x_atual,
                                calc_y_atual,
                                proximo_x,
                                proximo_y,
                                vec + ix,
                                16384 - ix);
            if (tam + ix >= 16384 - 2) {
                // acabou espaço no vetor...
                printf("Sem espaço;\r\n");
                while (1);
            }
            ix += tam;
            calc_x_atual = proximo_x;
            calc_y_atual = proximo_y;
        }
        Caminho l {vec, ix, theta1, theta2};

        // dorme um pouco pro ima ligar
        sleep_ms(700);

        // anda e depois desliga ima
        anda_no_caminho_dado(l, ix);

        gpio_put(magnet_pin, 0);
        ima_ligado = false;
        sleep_ms(500);
    }

#endif
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
