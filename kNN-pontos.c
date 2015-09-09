// Vizinho mais próximo

#include <stdio.h>
#include <math.h>		/* distancia euclidiana */
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include "timer.h"       /* time */


#define MIN_POINTS	5
#define MAX_POINTS	100


//Estrutura para armazenar as coordenadas de um ponto.
typedef struct _point{
	float x;
	float y;
	float k_avg;
}Point;

int compare(const void*, const void*);

Point p[MAX_POINTS];

int main(){
	Point pr; //Estruturas para cada um dos pontos (p1...p5) e para o ponto de referência (pr).
	int points_n, i, j; //Número de pontos, iterador e indice do ponto mais proximo
	float avg_sum; //Distâncias euclidianas do ponto mais proximo.
	int k_nb; // number of nb
	int *v_nb;

	double start, finish;

	float v_aux[points_n];

	// inicializa a semente da aleatoriedade
  srand (time(NULL));

	//Recebe a quantidade de pontos
	do {
		scanf("%d", &points_n);
	} while (points_n < MIN_POINTS || points_n > MAX_POINTS);

	//Gera aloatoriamente points_n pontos
	for (i = 0; i < points_n; i++){
		p[i].x = rand() % MAX_POINTS + 1;
		p[i].y = rand() % MAX_POINTS + 1;
	}

	//Imprime os points_n pontos
	for (i = 0; i < points_n; i++){
		printf("P(%.0f,%.0f) ", p[i].x, p[i].y);
	}
	printf("\n\n");

	scanf("%d", &k_nb);

	GET_TIME(start);

	for (i = 0; i < points_n; i++) {
		avg_sum = 0;
		for (j = 0; j < points_n; j++){
			//Calcula as distâncias euclidianas para cada um dos pontos.
			if (p[i].x != p[j].x || p[i].y != p[j].y)
				v_aux[j] = sqrt(pow(p[i].x - p[j].x, 2.0) + pow(p[i].y - p[j].y, 2.0));
		}
		qsort(v_aux, points_n - 1, sizeof(float), compare); //excluding the point itself
		for (j = 0; j < k_nb; j++) {
			avg_sum += v_aux[j];
		}
		p[i].k_avg = avg_sum / k_nb;
	}
	qsort(p, points_n, sizeof(Point), compare);

	GET_TIME(finish);

	for (i = 0; i < points_n; i++) {
		printf("[%d] Média entre os %d elementos em P(%.0f,%.0f) = %.2f\n", i, k_nb, p[i].x, p[i].y, p[i].k_avg);
	}

	printf("Elapsed time = %e seconds\n", finish - start);

	return 0;
}

int compare(const void* x, const void* y)
{
		return ceil(((Point*) x)->k_avg - ((Point*) y)->k_avg);
}
