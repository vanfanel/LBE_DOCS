#include <stdio.h>

int main (){
	int mat[2][2];
	//Guardamos en column-major...
	mat[0][0]=1;
	mat[1][0]=221;
	//mat[2][1]=532;
	//mat[3][1]=987;

	int* p = mat+1;
	printf ("valor adyacente = %d\n" ,(int*)*p); 
}
