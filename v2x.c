/*
 * Simulador V2X simples: carros a comunicar por um radio que perde mensagens.
 *
 * A ideia central: os carros de tras NAO tem sensores. Nao veem nada. A unica
 * forma de saberem que o carro da frente travou e' receberem a mensagem por
 * radio. E o radio nao e' perfeito: quanto mais longe, maior a hipotese de a
 * mensagem se perder, e acima de um certo alcance nao chega de todo.
 *
 * A experiencia: correr com alcance grande (o aviso chega, ninguem bate) e com
 * alcance pequeno (o aviso nao chega, batem).
 *
 * Uso:  ./v2x <alcance_em_metros>
 *       ./v2x 300   -> o aviso chega, os carros travam a tempo
 *       ./v2x 50    -> o aviso nao chega, ha colisao
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N_CARROS   3
#define DT         0.1      /* passo de tempo: 0.1 segundos */
#define TEMPO_TOTAL 20.0    /* simular 20 segundos */

#define VELOCIDADE  25.0    /* m/s (90 km/h) */
#define DISTANCIA   40.0    /* metros entre carros */
#define TRAVAGEM    5.0     /* desaceleracao em m/s^2 */
#define INSTANTE_TRAVAGEM 8.0  /* o carro da frente trava aos 8 segundos */

/* tipos de mensagem */
#define MSG_POSICAO 0   
#define MSG_TRAVAGEM 1  

// mensagem de radio
typedef struct {
    int    tipo;        
    int    emissor;     
    double posicao;     
} Mensagem;

// carro
typedef struct {
    double posicao;
    double velocidade;
    int    esta_a_travar;       
    int    sabe_do_perigo;      /
} Carro;

static Carro carros[N_CARROS];
static double alcance = 300.0;   /* alcance do radio, em metros */
static double tempo = 0.0;

/* estatisticas simples */
static long enviadas = 0, recebidas = 0, perdidas = 0;


/*
 * O CANAL (o radio). Decide se uma mensagem enviada por um carro chega a outro.
 * Regra:
 *   - se a distancia entre os dois for maior que o alcance -> perde-se sempre
 *   - senao -> chega, mas com uma hipotese de perda que cresce com a distancia
 */
static int mensagem_chega(double dist)
{
    double prob_perda;

    if (dist > alcance)
        return 0;                       /* longe demais: nao chega */

    prob_perda = dist / alcance;        /* 0 perto, quase 1 no limite do alcance */

    /* rand() da' um numero entre 0 e RAND_MAX; normalizamos para 0..1 */
    double sorteio = (double)rand() / RAND_MAX;

    return (sorteio > prob_perda);      /* chega se tivermos "sorte" */
}


/*
 * Enviar uma mensagem de um carro para todos os outros.
 * Cada carro que a receber processa-a.
 */
static void enviar(int emissor, int tipo)
{
    int i;
    Mensagem m;
    m.tipo    = tipo;
    m.emissor = emissor;
    m.posicao = carros[emissor].posicao;

    for (i = 0; i < N_CARROS; i++) {
        if (i == emissor) continue;     /* nao envia para si proprio */

        enviadas++;
        double dist = fabs(carros[i].posicao - carros[emissor].posicao);

        if (mensagem_chega(dist)) {
            recebidas++;
            /* o carro i recebeu a mensagem: so reage ao aviso de travagem,
               e so se o perigo estiver a' sua frente */
            if (m.tipo == MSG_TRAVAGEM && m.posicao > carros[i].posicao) {
                if (!carros[i].sabe_do_perigo) {
                    carros[i].sabe_do_perigo = 1;
                    printf("  t=%.1fs  carro %d recebeu o aviso e vai travar\n",
                           tempo, i);
                }
            }
        } else {
            perdidas++;
        }
    }
}


/* atualizar um carro: decidir a velocidade e mover */
static void mover_carro(int i)
{
    /* o carro da frente (0) trava no instante combinado */
    if (i == 0 && tempo >= INSTANTE_TRAVAGEM) {
        carros[i].velocidade -= TRAVAGEM * DT;
        if (!carros[i].esta_a_travar) {
            carros[i].esta_a_travar = 1;
            printf("  t=%.1fs  carro 0 TRAVOU e enviou aviso\n", tempo);
            enviar(0, MSG_TRAVAGEM);
        }
    }
    /* os carros de tras: so travam se souberem do perigo (receberam o aviso) */
    else if (i > 0 && carros[i].sabe_do_perigo) {
        carros[i].velocidade -= TRAVAGEM * DT;
    }

    if (carros[i].velocidade < 0)
        carros[i].velocidade = 0;

    carros[i].posicao += carros[i].velocidade * DT;
}


int main(int argc, char **argv)
{
    int i, houve_colisao = 0;
    double menor_distancia[N_CARROS];

    if (argc > 1)
        alcance = atof(argv[1]);

    /* colocar os carros: carro 0 a' frente, os outros atras, em fila */
    for (i = 0; i < N_CARROS; i++) {
        carros[i].posicao    = -(double)i * DISTANCIA;
        carros[i].velocidade = VELOCIDADE;
        carros[i].esta_a_travar  = 0;
        carros[i].sabe_do_perigo = 0;
        menor_distancia[i] = 1e9;
    }

    printf("=== Simulador V2X ===\n");
    printf("alcance do radio: %.0f m\n", alcance);
    printf("%d carros em fila, %.0f m de intervalo, a %.0f m/s\n\n",
           N_CARROS, DISTANCIA, VELOCIDADE);

    /* ciclo principal: avancar o tempo em passos de DT */
    for (tempo = 0.0; tempo < TEMPO_TOTAL; tempo += DT) {

        /* cada carro envia a sua posicao regularmente (uma vez por segundo) */
        for (i = 0; i < N_CARROS; i++) {
            if (fmod(tempo, 1.0) < DT)
                enviar(i, MSG_POSICAO);
        }

        /* mover todos os carros */
        for (i = 0; i < N_CARROS; i++)
            mover_carro(i);

        /* registar a menor distancia entre cada carro e o da frente */
        for (i = 1; i < N_CARROS; i++) {
            double d = carros[i-1].posicao - carros[i].posicao;
            if (d < menor_distancia[i])
                menor_distancia[i] = d;
        }
    }

    /* resultado */
    printf("\n=== Resultado ===\n");
    for (i = 1; i < N_CARROS; i++) {
        int bateu = (menor_distancia[i] <= 0);
        if (bateu) houve_colisao = 1;
        printf("carro %d: distancia minima ao da frente = %.1f m  %s\n",
               i, menor_distancia[i], bateu ? "*** COLISAO ***" : "ok");
    }

    printf("\nmensagens enviadas: %ld\n", enviadas);
    printf("recebidas: %ld  |  perdidas: %ld\n", recebidas, perdidas);

    return houve_colisao;
}