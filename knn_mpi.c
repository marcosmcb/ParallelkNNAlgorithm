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
//          executar: ./kNNParaleloMPI
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include <stddef.h>
#include "timer.h"

#define MAX_VALUE 1000
#define MASTER 0

typedef struct _point {
    float x;
    float y;
} Point;

typedef struct _cluster {
  float mean;
  Point *points;
} Cluster;

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

    int my_rank; // variavel para o rank do processo
    int comm_sz; // variavel para o numero de processos
    int clusters_per_proc; // variavel para o numero de clusters por processo
    Point* points;  // variavel para o vetor de pontos gerados.
    float* avgs;    // variavel para a média das distancias dos clusters.
    float* sub_avgs;
    float best_sub_avg;
    float* best_sub_avgs;
    Point** clusters;   // variavel para armazenar todos os clusters.
    Point** sub_clusters;
    float** distances;  // variavel para armazenar as distancias dos clusters.
    float** sub_distances;
    double start, finish;   //variaveis para medição do tempo.

    const int nitems = 2;
    int blocklengths[2] = {1, 1};
    MPI_Datatype types[2] = {MPI_FLOAT, MPI_FLOAT};
    MPI_Datatype mpi_points_type;
    MPI_Aint offsets[2];

    offsets[0] = offsetof(Point, x);
    offsets[1] = offsetof(Point, y);

    // Tarefas iniciais do MPI
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    /* Cria um tipo para Points */
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_points_type);
    MPI_Type_commit(&mpi_points_type);

    // Leitura dos valores numThreads, n e k
    if (my_rank == MASTER) {
      printf("N - Informe a quantidade de pontos a ser gerada (aleatoriamente)\n");
      scanf("%d", &n);
      printf("K - Informe o número de Vizinhos\n");
      scanf("%d", &k);

      // Alocação dos vetores e matrizes.
      avgs = (float*) malloc (sizeof(float) * n);

      clusters = (Point**) malloc (sizeof(Point*) * n);
      for(i = 0; i < n; i++)
          clusters[i] = (Point*) malloc (sizeof(Point) * (k + 1));

      distances = (float**) malloc (sizeof(float*) * n);
      for(i = 0; i < n; i++)
          distances[i] = (float*) malloc (sizeof(float) * (k + 1));
    }

    // Calcula o número de clusters por processo
    if (n % comm_sz == 0)
      clusters_per_proc = n / comm_sz;
    else
      exit(1); // tem que ser divisivel

    srand(time(NULL));

    // Alocação do vetor de pontos e geração dos mesmos.
    points = (Point*) malloc (sizeof(Point) * n);

    for(i = 0; i < n; i++) {
        points[i].x = rand() % MAX_VALUE + 1;
        points[i].y = rand() % MAX_VALUE + 1;
    }

    // Aloca as variaveis para cada processo
    sub_clusters = (Point**) malloc (sizeof(Point*) * clusters_per_proc);
    for (i = 0; i < clusters_per_proc; i++)
      sub_clusters[i] = (Point*) malloc (sizeof(Point) * (k + 1));

    sub_avgs = (float*) malloc (sizeof(float) * clusters_per_proc);

    sub_distances = (float**) malloc (sizeof(float*) * clusters_per_proc);
    for (i = 0; i < clusters_per_proc; i++)
      sub_distances[i] = (float*) malloc (sizeof(float) * (k + 1));

    // Envia o valor de n e k para todos os processos
    MPI_Bcast(&n, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
    MPI_Bcast(points, n, mpi_points_type, MASTER, MPI_COMM_WORLD);

    if(my_rank != 0) {
      for(i = 0; i < clusters_per_proc; i++) {
          build_clusters(&points, &sub_clusters, &sub_distances, i, n, k);
          calc_avg(&sub_avgs, &sub_distances, i, k);
      }
    }

    if(my_rank != 0) {
      printf("p %d\n", my_rank);
      printf("ponto cluster 1: %f, %f\n", sub_clusters[1][1].x, sub_clusters[1][1].y);
      // printf("ponto 2: %f, %f\n", points[5].x, points[5].y);
      printf("\n");
    }

    // if (my_rank == MASTER) printf("OI\n");
    //
    // // Busca o cluster com a melhor distancia média.
    // if (my_rank == MASTER) {
    //   i = best_avg(avgs, n);
    //
    //   printf("The best cluster has an average distance of: %f\n", avgs[i]);
    //   printf("The cluster has the points:\n\n");
    //   for(j = 0; j < k+1; j++) {
    //       printf("%f, %f\n", clusters[i][j].x, clusters[i][j].y);
    //   }
    //
    //   printf("\n");
    // }
    //
    // // Desaloca a memória
    // // for(i = 0; i < n; i++) {
    // //     free(distances[i]);
    // //     free(clusters[i]);
    // // }
    // //
    // // free(distances);
    // // free(clusters);
    // // free(points);
    // // free(avgs);
    MPI_Finalize();
    return 0;
}
