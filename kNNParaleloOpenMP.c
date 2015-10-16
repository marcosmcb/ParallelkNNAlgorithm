//-----------------------------------------------------------------------------
//  Algoritmo kNN (k Nearest Neighbours) implementado de maneira paralela com o auxílio da biblioteca 
//  Intel OpenMP para a disciplina de Computação Paralela da UFSCar Sorocaba.
//      Grupo:
//          Alexandre Braga Saldanha    408484
//          André Bonfatti              408182
//          Marcos Cavalcante           408336
//    
//      Compilação e Execução do programa:
//
//          compilar: make
//          executar: ./kNNParaleloOpenMP
//
//      Remover executáveis:
//          remover: make clean
//
//      Existe uma pasta com os arquivos de teste usados para rodar os experimentos
//      O nome dessa pasta é: arquivosDeTeste.
//      
//      Caso você queira rodar o programa sem usar os arquivos de teste, a entrada pedida é:
//          T - Número de Threads [valor inteiro];
//          N - Número de Pontos gerados aleatoriamente [valor inteiro];
//          K - Número de Vizinhos [valor inteiro];
//-----------------------------------------------------------------------------
#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "timer.h"

#define MAX_VALUE 1000

typedef struct _point {
    float x;
    float y;
} Point;

/**
 * verify_is_in_cluster()
 * Função para verificar se um ponto pertence ao cluster.
 * Verifica se a distancia do novo ponto é menor do que a maior distancia do
 * cluster. Caso seja, a maior distancia é substituida.
 */
int verify_is_in_cluster(float*** distances, float dist, int i, int k) {
    int j;
    int index = 0;
    int max = -1;
    
    // Caso o cluster ainda não esteja completo
    for(j = 1; j < (k + 1); j++) {
        if((*distances)[i][j] == 0) {
            (*distances)[i][j] = dist;
            return j;
        }
    }
    
    // Procura o indice da maior distancia
    for(j = 1; j < (k + 1); j++) {
        if((*distances)[i][j] > max) {
            index = j;
            max = (*distances)[i][j];
        }
    }
    
    // Substitui pelo nova distancia calculada, caso essa seja menor
    // que a maior distancia do cluster.
    if((dist > max) && (max != 0)) {
        index = 0;
    }
    else {
        (*distances)[i][index] = dist;
    }
    
    return index;    
}

/**
 * build_clusters()
 * Função para montar todos os possiveis clusters com os pontos gerados.
 * Calcula a distancia euclidiana do ponto de referencia (clusters[i][0])
 * para todos os outros pontos e monta os clusters com o k vizinhos mais próximos.
 */
void build_clusters(Point** points, Point*** clusters, float*** distances, int i, int n, int k) {
    int j;
    int index;
    float dist;
    
    // Ponto de referencia
    (*clusters)[i][0] = (*points)[i];    
    
    // Calcula a distancia do ponto de referencia para todos os outros pontos.
    for(j = 0; j < n; j++) {
        if(j != i) { // Não conta a distancia do ponto de referencia pra ele mesmo.
            dist = sqrt(pow((*points)[i].x - (*points)[j].x, 2.0) + pow((*points)[i].y - (*points)[j].y, 2.0));
            index = verify_is_in_cluster(&(*distances), dist, i, k);
            if(index) { // Adiciona o ponto ao cluster se ele possui uma das menores distancias.
                (*clusters)[i][index] = (*points)[j];
            }
        }
    }
}

/**
 * calc_avg()
 * Calcula a média das distancias do cluster i.
 */
void calc_avg(float** avgs, float*** distances, int i, int k) {
    int j;
    float sum = 0;
    
    for(j = 1; j < (k + 1); j++) {
        sum += (*distances)[i][j];
    }
    
    (*avgs)[i] = sum / k;
}

/**
 * best_avg()
 * Seleciona o cluster i com a menor distancia média.
 */
int best_avg(float* avgs, int n) {
    int i;
    int index = 1;
    int min = avgs[0];
    
    for(i = 1; i < n; i++) {
        if(avgs[i] < min) {
            index = i;
            min = avgs[i];
        }
    }
    
    return index;
}

/**
 * MAIN
 */
int main() {
    int n;  // variavel para o número de pontos a serem gerados
    int k;  // variavel para o tamanho dos clusters (k vizinhos).
    int i;  // variavel auxiliar para indices
    int j;  // variavel auxiliar para indices
    int numThread; //variavel para o numero de threads informado pelo usuario
    Point* points;  // variavel para o vetor de pontos gerados.
    float* avgs;    // variavel para a média das distancias dos clusters.
    Point** clusters;   // variavel para armazenar todos os clusters.
    float** distances;  // variavel para armazenar as distancias dos clusters.
    double start, finish;   //variaveis para medição do tempo.
    
    // Leitura dos valores numThreads, n e k
    printf("T - Informe o número de threads\n");
    scanf("%d", &numThread);
    printf("N - Informe a quantidade de pontos a ser gerada (aleatoriamente)\n");
    scanf("%d", &n);
    printf("K - Informe o número de Vizinhos\n");
    scanf("%d", &k);
    
    srand(time(NULL));
    
    // Alocação do vetor de pontos e geração dos mesmos.
    points = (Point*) malloc (sizeof(Point) * n);
    
    for(i = 0; i < n; i++) {
        points[i].x = rand() % MAX_VALUE + 1;
        points[i].y = rand() % MAX_VALUE + 1;
    }
    
    // Alocação dos vetores e matrizes.
    avgs = (float*) malloc (sizeof(float) * n);
    
    clusters = (Point**) malloc (sizeof(Point*) * n);
    for(i = 0; i < n; i++)
        clusters[i] = (Point*) malloc (sizeof(Point) * (k + 1));
        
    distances = (float**) malloc (sizeof(float*) * n);
    for(i = 0; i < n; i++)
        distances[i] = (float*) malloc (sizeof(float) * (k + 1));
    
    for(i = 0; i < n; i++) {
        for(j = 0; j < (k + 1); j++) {
            distances[i][j] = 0;
        }
    }
    /* Inicia a contagem do tempo para avaliacao posterior */
    GET_TIME(start);
    // Computação paralela dos clusters, bem como suas médias
    /* Setando a criação de threads de maneira dinâmicas para 0, Nós evitamos que o OpenMP decida sozinho a quantidade de threads*/
    omp_set_dynamic(0);
    /* A quantidade de threads informada pelo usuário será utilizada na verdade */
    omp_set_num_threads(numThread);

    #pragma omp parallel
    {
        #pragma omp for
            for(i = 0; i < n; i++) {
                build_clusters(&points, &clusters, &distances, i, n, k);
                calc_avg(&avgs, &distances, i, k);
            }
    }

    /*Contagem do tempo é terminada aqui*/
    GET_TIME(finish);
    
    // Busca o cluster com a melhor distancia média.
    i = best_avg(avgs, n);

    printf("The best cluster has an average distance of: %f\n", avgs[i]);
    printf("The cluster has the points:\n\n");
    for(j = 0; j < k+1; j++) {
        printf("%f, %f\n", clusters[i][j].x, clusters[i][j].y);
    }
    
    printf("\n");
    
    printf("The time calculated is: %f\n", finish - start);
    
    // Desaloca a memória
    for(i = 0; i < n; i++) {
        free(distances[i]);
        free(clusters[i]);
    }
    
    free(distances);
    free(clusters);
    free(points);
    free(avgs);
    
    return 0;
}