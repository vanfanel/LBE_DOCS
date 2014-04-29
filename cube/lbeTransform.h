typedef struct {
	float m[4][4];
} lbeMatrix;

void lbeLoadIdentity (lbeMatrix *resultado);
void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b);
