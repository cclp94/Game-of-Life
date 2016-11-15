typedef struct Cell {
	bool alive[10];
};

__kernel void computeTick(__global const struct Cell* cellGrid, __global struct Cell* copyCell, __global const int *numSpecies)
{
	int i = get_global_id(0); //also y
	int j = get_global_id(1); //also x

	int HEIGHT = 768;
	int WIDTH = 1024;

	for (int currentSpecies = 0; currentSpecies < *numSpecies; currentSpecies++) {
		int numberAliveNeighbors = 0;
		for (int iN = -1; iN <= 1; iN++)
			for (int jN = -1; jN <= 1; jN++)
				if (i != 0 && i != HEIGHT - 1 && j != 0 && j != WIDTH - 1 && !(iN == 0 && jN == 0) && cellGrid[((i + iN)*WIDTH) + (j + jN)].alive[currentSpecies])
					numberAliveNeighbors++;
		if (numberAliveNeighbors < 2 || numberAliveNeighbors > 3)
			copyCell[i*WIDTH + j].alive[currentSpecies] = false;
		else if (cellGrid[i*WIDTH + j].alive[currentSpecies] || (!cellGrid[i*WIDTH + j].alive[currentSpecies] && numberAliveNeighbors == 3))
			copyCell[i*WIDTH + j].alive[currentSpecies] = true;
		else {
			copyCell[i*WIDTH + j].alive[currentSpecies] = false;
		}
	}
}