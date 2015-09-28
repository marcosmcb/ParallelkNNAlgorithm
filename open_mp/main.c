#include <omp.h>

#define MIN_POINTS 1
#define MAX_VALUE 1000

typedef struct _point {
  float x;
  float y;
} Point;

int main() {
  int n_points;
  int i;
  float** distances;
  float** averages;
  Point* points;

  do {
    scanf("%d", &n_points);
  } while(n_points < MIN_POINTS);

  do {
    scanf("%d", &k);
  } while(k > n_points || k < 2);

  points = (Point *) malloc(sizeof(Point) * n_points);

  for(i = 0; i < n_points; i++) {
    points[i].x = rand() % MAX_VALUE + 1;
    points[i].y = rand() % MAX_VALUE + 1;
  }

  distances = (float**) malloc(sizeof(float*) * n_points);
  averages = (float*) malloc(sizeof(float) * n_points);

  

}