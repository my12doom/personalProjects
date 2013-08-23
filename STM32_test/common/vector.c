#include "vector.h"
#include <math.h>

void vector_add(vector *a, vector *b)		// a = a+b
{
	int i;
	for (i=0; i<3; i++)
		a->array[i] += b->array[i];
}
void vector_sub(vector *a, vector *b)		// a = a-b
{
	int i;
	for (i=0; i<3; i++)
		a->array[i] -= b->array[i];
}
void vector_divide(vector *a, float b)		// a = a/b
{
	int i;
	for (i=0; i<3; i++)
		a->array[i] /= b;
}
void vector_multiply(vector *a, float b)	// a = a*b
{
	int i;
	for (i=0; i<3; i++)
		a->array[i] *= b;
}
void vector_rotate(vector *v, float *delta)
{
	vector v_tmp = *v;
	v->V.z -= delta[0]  * v_tmp.V.x + delta[1] * v_tmp.V.y;
	v->V.x += delta[0]  * v_tmp.V.z - delta[2]   * v_tmp.V.y;
	v->V.y += delta[1] * v_tmp.V.z + delta[2]   * v_tmp.V.x;
}

float vector_length(vector *v)
{
	return sqrt(v->array[0]*v->array[0] + v->array[1] * v->array[1] + v->array[2] * v->array[2]);
}