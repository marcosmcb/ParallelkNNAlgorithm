#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "timer.h"

#define MASTER 0
#define MAX_VALUE 1000

typedef struct _point {
    float x;
    float y;
} Point;

/**
 * Gera total_points pontos aleatoriamente, armazenando em points.
 */
void generate_points(Point* points, int total_points) {
  int i;

  srand(time(NULL));

  for(i = 0; i < total_points; i++) {
    points[i].x = rand() % MAX_VALUE + 1;
    points[i].y = rand() % MAX_VALUE + 1;
  }
}

/**
 * Cria o MPI_Datatype para a struct Point.
 */
void create_mpi_point_type(MPI_Datatype *mpi_point_type) {
  int nitems = 2;
  int blocklengths[2] = {1, 1};
  MPI_Datatype types[2] = {MPI_FLOAT, MPI_FLOAT};
  MPI_Aint offsets[2] = {offsetof(Point, x), offsetof(Point, y)};

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, mpi_point_type);
  MPI_Type_commit(mpi_point_type);
}

/**
 * verify_is_in_cluster()
 * Função para verificar se um ponto pertence ao cluster.
 * Verifica se a distância do novo ponto é menor do que a maior distância do
 * cluster. Caso seja, a maior distância é substituída.
 */
int verify_is_in_cluster(float* distances, float dist, int k) {
  int i;
  int index = -1;
  float max = -1;

  // Caso o cluster ainda não esteja completo
  for(i = 1; i < k + 1; i++) {
    if(distances[i] == -1) {
      return i;
    }
  }

  // Procura o indice da maior distancia
  for(i = 1; i < k + 1; i++) {
    if(distances[i] > max) {
      index = i;
      max = distances[i];
    }
  }

  // Substitui pela nova distancia calculada, caso essa seja menor
  // que a maior distancia do cluster.
  if(dist >= max && max != 0) {
    index = 0;
  }

  return index;
}

/**
 * Constrói um cluster para o ponto de referência i.
 * Retorna a distância média entre os pontos deste cluster.
 */
float build_cluster(Point** cluster, Point* points, int i, int N, int k) {
  int j;
  int index;
  float dist;
  float distances[k + 1];
  float avg = 0;

  // Inicializa vetor de distâncias
  distances[0] = 0;
  for(j = 1; j < k + 1; j++)
    distances[j] = -1;

  // Ponto de referência
  (*cluster)[0] = points[i];

  // Calcula a distância do ponto de referência para todos os outros pontos.
  for(j = 0; j < N; j++) {
    if(j != i) { // Não conta a distancia do ponto de referencia pra ele mesmo.
      dist = sqrt(pow(points[i].x - points[j].x, 2.0) + pow(points[i].y - points[j].y, 2.0));
      index = verify_is_in_cluster(distances, dist, k);
      if(index > 0) { // Adiciona o ponto ao cluster se ele possui uma das menores distancias.
        (*cluster)[index] = points[j];
        distances[index] = dist;
      }
    }
  }

  for(j = 1; j < k + 1; j++)
    avg += distances[j]/k;

  return avg;
}

int main() {
  int rank;                     // Rank do processo.
  int total_processes;          // Número total de processos.

  int N;                        // Número total de pontos.
  int n;                        // Número de pontos por processo.
  int k;                        // Número de vizinhos.

  Point *points;                // Vetor de pontos.
  Point *my_best_cluster;       // Melhor cluster de um processo.
  Point *best_clusters;         // Melhores clusters reunidos dos processos.
  Point *cluster;               // Cada cluster calculado num processo.

  float avg;                    // Distância média de um cluster.
  float my_best_avg = -1;       // Melhor distância média de um processo.
  float best_avg = 0;           // Melhor distância média de todos os processos.
  float *best_avgs;             // Vetor de melhores distâncias médias.

  double start, finish;         // Para medição de tempo.

  int best_index;               // Índice do melhor cluster.
  int start_index;              // Índice inicial do melhor cluster no vetor de melhores clusters.

  int my_start_index;           // Índice inicial de um processo.

  int i, j;                     // Auxiliares para iterações.

  MPI_Datatype mpi_point_type;  // Tipo MPI para Point.

  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &total_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  create_mpi_point_type(&mpi_point_type);

  if(rank == MASTER) {
    printf("Digite o numero de pontos (deve ser divisivel por %d): ", total_processes);
    fflush(stdout);
    scanf("%d", &N);

    printf("Digite o tamanho dos clusters (deve ser menor que %d): ", N);
    fflush(stdout);
    scanf("%d", &k);
  }

  MPI_Bcast(&N, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
  MPI_Bcast(&k, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

  if(N % total_processes == 0 && k < N) {
    n = N / total_processes;
  }
  else {
    printf("Valores invalidos");
    MPI_Finalize();
    exit(1);
  }

  // Alocação dos vetores de pontos.
  points = (Point *) malloc(sizeof(Point) * N);
  my_best_cluster = (Point *) malloc(sizeof(Point) * (k + 1));
  cluster = (Point *) malloc(sizeof(Point) * (k + 1));

  if(rank == MASTER) {
    generate_points(points, N);
  }

  // Broadcast dos pontos para todos os processos.
  MPI_Bcast(points, N, mpi_point_type, MASTER, MPI_COMM_WORLD);

  my_start_index = rank * n;

  // Inicia a contagem de tempo
  if(rank == MASTER)
    GET_TIME(start);

  for(i = my_start_index; i < my_start_index + n; i++) {
    avg = build_cluster(&cluster, points, i, N, k);
    if(avg < my_best_avg || my_best_avg == -1) {
      my_best_avg = avg;
      for(j = 0; j < k + 1; j++) {
        my_best_cluster[j] = cluster[j];
      }
    }
  }

  // Termina a contagem de tempo
  if(rank == MASTER)
    GET_TIME(finish);

  if(rank == MASTER) {
    best_clusters = (Point *) malloc(sizeof(Point) * total_processes * (k + 1));
    best_avgs = (float *) malloc(sizeof(float) * total_processes);
  }

  MPI_Gather(my_best_cluster, k + 1, mpi_point_type,
             best_clusters, k + 1, mpi_point_type,
             MASTER, MPI_COMM_WORLD);
  MPI_Gather(&my_best_avg, 1, MPI_FLOAT, best_avgs, 1, MPI_FLOAT,
             MASTER, MPI_COMM_WORLD);

  if(rank == MASTER) {
    for(i = 0; i < total_processes; i++) {
      if(best_avg == 0 || best_avgs[i] < best_avg) {
        best_avg = best_avgs[i];
        best_index = i;
      }
    }

    printf("\nA menor distancia media foi: %f\n", best_avg);
    start_index = best_index * (k + 1);
    printf("O ponto de referencia é: ");
    printf("(%.0f, %0.f)\n", best_clusters[start_index].x, best_clusters[i].y);
    printf("Os K vizinhos são:\n");
    for(i = start_index + 1; i < start_index + k + 1; i++) {
      printf("(%.0f, %0.f) ", best_clusters[i].x, best_clusters[i].y);
    }
    printf("\n");
    printf("O tempo gasto foi: %f\n", finish - start);
  }

  MPI_Finalize();

  return 0;
}
