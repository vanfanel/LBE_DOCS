typedef struct {
	float m[4][4];
} lbeMatrix;

void lbeLoadIdentity (lbeMatrix *resultado);
void lbeRotate (lbeMatrix *resultado, int axis_x, int axis_y, int axis_z, float deg);
void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b);
void lbePrintMatrix(lbeMatrix *mat);
