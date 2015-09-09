// Codigo serial para o problema do KNN
// Vizinho mais próximo

#include <stdio.h>
#include <math.h>       /* distancia euclidiana */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include "timer.h"

#define MIN_PONTOS  5
#define MAX_VALUE   1000
#define DEBUG 0


//Estrutura para armazenar as coordenadas de um ponto.
typedef struct _ponto{
    float x;
    float y;
}Ponto;


int main(){
    int kClusters, nPontos; //Tamanho dos clusters, Número de pontos
    float dist, d_minima; //Distâncias euclidianas do ponto mais proximo.
    int i, j, k; // Iteradores

    // inicializa a semente da aleatoriedade
    srand (time(NULL));


    //Recebe a quantidade de pontos
    do {
        printf("Insira a quantidade de pontos: ");
        scanf("%d", &nPontos);
    } while (nPontos < MIN_PONTOS);

	//Estruturas para cada um dos pontos (p1...p5)
	//Ponto p[nPontos];
	Ponto* p = (Ponto* ) malloc (sizeof(Ponto) * nPontos);

    //Recebe o tamanho dos clusters
    do {
        printf("Insira o tamanho desejado do cluster: ");
        scanf("%d", &kClusters);
    } while (kClusters > nPontos || kClusters < 2);


    //Gera aloatoriamente nPontos pontos
    for (i = 0; i < nPontos; i++){
        p[i].x = rand() % MAX_VALUE + 1;
        p[i].y = rand() % MAX_VALUE + 1;
    }


    //Imprime os nPontos pontos
    if (DEBUG) {
        printf("DEBUG - Pontos gerados aleatoriamente:\n");
        for (i = 0; i < nPontos; i++){
            printf("P[%d] = (%.0f,%.0f)\n", i, p[i].x, p[i].y);
        }
        printf("\n\n");
    }


    // Armazena a distancia de cada ponto para todos os demais
    //float distancias[nPontos][nPontos];
    float** distancias = (float**) malloc (sizeof(float*) * nPontos);
    // Armazena os indices dos k vizinhos mais proximos
    //int clusters[nPontos][kClusters];
    int** clusters = (int**) malloc (sizeof(int*) * nPontos);
    // Armazena a distancia média para cada cluster
    //float medias[nPontos];
    float* medias = (float*) malloc (sizeof(float) * nPontos);
	// Indices temporarios
	int l, m, n;
    int melhorCluster = 0;

    double start, finish;

    //Registra o tempo ao iniciar o calculo
    GET_TIME(start);

    // Calcula distancia de cada ponto em relacao a todos os demais
    for (i = 0; i < nPontos; i++){
        for (j = i; j < nPontos; j++){

			// Aloca colunas das matrizes de distancias e de clusters
			if(i == 0) {
				distancias[j] = (float*) malloc (sizeof(float) * nPontos);
				clusters[j] = (int*) malloc (sizeof(int) * kClusters);
			}

            dist = sqrt(pow(p[i].x - p[j].x, 2.0) + pow(p[i].y - p[j].y, 2.0));
            distancias[i][j] = dist;
            distancias[j][i] = dist;
            l=j;
            n=i;

			// Inicializa clusters
            if (i < kClusters) {
				clusters[j][i] = i;
			}

            if (j < kClusters) {
                clusters[i][j] = j;

			// Ja com os clusters inicializados
            } else {
                for (k = 0; k < kClusters; k++){
                    // Atualiza o indice dos pontos mais proximos
                    if (distancias[i][l] < distancias[i][clusters[i][k]]) {
                        m = clusters[i][k];
                        clusters[i][k] = l;
                        l = m;
                    }

                    // Atualiza distancia media do cluster i
                    if (k == 0) {
                        medias[i] = distancias[i][clusters[i][k]] / kClusters;
                    } else {
                        medias[i] += (distancias[i][clusters[i][k]] / kClusters);
                    }

					if ((i >= kClusters) && (j != i)){
						// Atualiza o índice dos pontos mais proximos
						if (distancias[j][n] < distancias[j][clusters[j][k]]) {
							m = clusters[j][k];
							clusters[j][k] = n;
							n = m;
						}

						// Atualiza distancia media do cluster j
						if (k == 0) {
							medias[j] = distancias[j][clusters[j][k]] / kClusters;
						} else {
							medias[j] += (distancias[j][clusters[j][k]] / kClusters);
						}
					}
                }
            }
        }

        // Atualiza o indice do cluster mais compacto
        if (medias[i] < medias[melhorCluster]) {
            melhorCluster = i;
        }
    }
    //Registra o tempo ao finalizar o calculo
    GET_TIME(finish);

    // Imprime distancias
    if (DEBUG) {
        printf("DEBUG - Distancias:\n");
        for (i = 0; i < nPontos; i++){
            for (j = 0; j < nPontos; j++){
                printf("%7.2f ",distancias[i][j]);
            }
            printf("\n");
        }
        printf("\n\n");
    }

    // Imprime clusters encontrados
    if (DEBUG) {
        printf("DEBUG - Clusters formados:\n");
        for (i = 0; i < nPontos; i++){
            printf("DEBUG - Cluster P[%d]: media %.2f.\n", i, medias[i]);
            for (k = 0; k < kClusters; k++){
                printf("(%.0f,%.0f)\n",p[clusters[i][k]].x,p[clusters[i][k]].y);
            }

            printf("\n");
        }
		printf("\n");
    }

    printf("O melhor cluster encontrado, com media %.2f de distancia entre os elementos, foi:\n", medias[melhorCluster]);
    for (k = 0; k < kClusters; k++){
        printf("(%.0f,%.0f)\n",p[clusters[melhorCluster][k]].x,p[clusters[melhorCluster][k]].y);
    }

    printf("\nO tempo para calcular o melhor cluster, foi %e segundos\n", finish - start);

	free(p);
	free(distancias);
	free(clusters);
	free(medias);

    return 0;
}

