
// TODO: Add OpenCL kernel code here.

typedef struct Cell {
	bool alive[10];
};

__kernel void initializeCells(__global struct Cell* cellGrid, __global const int *numSpecies, __global const float *seed, __global int *width,  __global int *height)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	*height = get_global_size(1);

	int WIDTH = get_global_size(0);

	unsigned int randomGen = 1103515245 * (x) + 12345;
 
 	int rand1 = (randomGen % *numSpecies);
 	int rand2 = randomGen % 100;
 
 	int speciesID = rand1;
 	if (rand2 <= 60) {
 		cellGrid[y*WIDTH + x].alive[speciesID] = true;
		*width+=1;
 	}
 	else
 		cellGrid[y*WIDTH + x].alive[speciesID] = false;
}