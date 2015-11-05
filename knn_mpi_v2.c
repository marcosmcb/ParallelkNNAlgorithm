#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
​
#define MASTER 0
#define MAX_VALUE 100
​
typedef struct _point {
    float x;
    float y;
} Point;
​
/**
 * Gera total_points pontos aleatoriamente, armazenando em points.
 */
void generate_points(Point* points, int total_points) {
  int i;
​
  srand(time(NULL));
​
  for(i = 0; i < total_points; i++) {
    points[i].x = rand() % MAX_VALUE + 1;
    points[i].y = rand() % MAX_VALUE + 1;
  }
}
​
/**
 * Cria o MPI_Datatype para a struct Point.
 */
void create_mpi_point_type(MPI_Datatype *mpi_point_type) {
  int nitems = 2;
  int blocklengths[2] = {1, 1};
  MPI_Datatype types[2] = {MPI_FLOAT, MPI_FLOAT};
  MPI_Aint offsets[2] = {offsetof(Point, x), offsetof(Point, y)};
​
  MPI_Type_create_struct(nitems, blocklengths, offsets, types, mpi_point_type);
  MPI_Type_commit(mpi_point_type);
}
​
/**
 * verify_is_in_cluster()
 * Função para verificar se um ponto pertence ao cluster.
 * Verifica se a distância do novo ponto é menor do que a maior distância do
 * cluster. Caso seja, a maior distância é substituída.
 */
int verify_is_in_cluster(float* distances, float dist, int k) {
  int i;
  int index = -1;
  int max = -1;
​
  // Caso o cluster ainda não esteja completo
  for(i = 1; i < k + 1; i++) {
    if(distances[i] == -1) {
      return i;
    }
  }
​
  // Procura o indice da maior distancia
  for(i = 1; i < k + 1; i++) {
    if(distances[i] > max) {
      index = i;
      max = distances[i];
    }
  }
​
  // Substitui pela nova distancia calculada, caso essa seja menor
  // que a maior distancia do cluster.
  if(dist >= max && max != 0) {
    index = 0;
  }
​
  return index;
}
​
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
​
  // Inicializa vetor de distâncias
  distances[0] = 0;
  for(j = 1; j < k + 1; j++)
    distances[j] = -1;
​
  // Ponto de referência
  (*cluster)[0] = points[i];
​
  // Calcula a distância do ponto de referência para todos os outros pontos.
  for(j = 0; j < N; j++) {
    printf("j: %d\n", j);
    if(j != i) { // Não conta a distancia do ponto de referencia pra ele mesmo.
      dist = sqrt(pow(points[i].x - points[j].x, 2.0) + pow(points[i].y - points[j].y, 2.0));
      printf("dist: %f\n", dist);
      index = verify_is_in_cluster(distances, dist, k);
      printf("index: %d\n", index);
      if(index > 0) { // Adiciona o ponto ao cluster se ele possui uma das menores distancias.
        (*cluster)[index] = points[j];
        distances[index] = dist;
        avg += dist/k;
      }
    }
  }
​
  return avg;
}
​
int main() {
  int rank;                     // Rank do processo.
  int total_processes;          // Número total de processos.
​
  int N = 15;                   // Número total de pontos.
  int n = 5;                    // Número de pontos por processo.
  int k = 3;                    // Número de vizinhos.
​
  Point *points;                // Vetor de pontos.
  Point *my_best_cluster;       // Melhor cluster de um processo.
  Point *best_clusters;         // Melhores clusters reunidos dos processos.
  Point *cluster;               // Cada cluster calculado num processo.
  Point point;
​
  float avg;                    // Distância média de um cluster.
  float my_best_avg = -1;       // Melhor distância média de um processo.
​
  int my_start_index;           // Índice inicial de um processo.
​
  int i, j;                     // Auxiliares para iterações.
​
  MPI_Datatype mpi_point_type;  // Tipo MPI para Point.
​
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &total_processes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
​
  create_mpi_point_type(&mpi_point_type);
​
  // Alocação dos vetores de pontos.
  points = (Point *) malloc(sizeof(Point) * N);
  my_best_cluster = (Point *) malloc(sizeof(Point) * (k + 1));
  cluster = (Point *) malloc(sizeof(Point) * (k + 1));
​
  if(rank == MASTER) {
    generate_points(points, N);
  }
​
  // Broadcast dos pontos para todos os processos.
  MPI_Bcast(points, N, mpi_point_type, MASTER, MPI_COMM_WORLD);
​
  my_start_index = rank * n;
​
  for(i = my_start_index; i < my_start_index + n; i++) {
    avg = build_cluster(&cluster, points, i, N, k);
    if(avg < my_best_avg || my_best_avg == -1) {
      my_best_avg = avg;
      for(j = 0; j < k + 1; j++) {
        my_best_cluster[j] = cluster[j];
      }
    }
    for(j = 0; j < k+1; j++) {
      cluster[j].x = 0;
      cluster[j].y = 0;
    }
  }
​
  if(rank == MASTER) {
    best_clusters = (Point *) malloc(sizeof(Point) * total_processes * (k + 1));
  }
  MPI_Gather(my_best_cluster, k + 1, mpi_point_type,
             best_clusters, k + 1, mpi_point_type,
             MASTER, MPI_COMM_WORLD);
​
  if(rank == MASTER) {
    printf("best clusters: \n");
    for(i = 0; i < total_processes; i++) {
      printf("i: %d\n", i);
      for(j = 0; j < k + 1; j++) {
        point = best_clusters[i * total_processes + j];
        printf("(%.0f, %.0f) ", point.x, point.y);
      }
      printf("\n");
    }
  }
​
  MPI_Finalize();
​
  return 0;
}
