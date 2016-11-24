typedef struct Cell {
	bool alive[10];
};

__kernel void computeTick(__global const struct Cell* cellGrid, __global struct Cell* copyCell, __global const int *numSpecies)
{
	int i = get_global_id(1); //also y
	int j = get_global_id(0); //also x

	int HEIGHT = get_global_size(1);
	int WIDTH = get_global_size(0);

	for (int currentSpecies = 0; currentSpecies < *numSpecies; currentSpecies++) {
		int numberAliveNeighbors = 0;
		for (int iN = -1; iN <= 1; iN++)
			for (int jN = -1; jN <= 1; jN++)
				if (i != 0 && i != HEIGHT - 1 && j != 0 && j != WIDTH - 1 &&		// Not in the borders
					!(iN == 0 && jN == 0) &&										// Not itself
					cellGrid[((i + iN)*WIDTH) + (j + jN)].alive[currentSpecies])	// alive & same species
					numberAliveNeighbors++;
		if (numberAliveNeighbors < 2 || numberAliveNeighbors > 3) {
			copyCell[i*WIDTH + j].alive[currentSpecies] = false;
		}
		else if (cellGrid[i*WIDTH + j].alive[currentSpecies] || (!cellGrid[i*WIDTH + j].alive[currentSpecies] && numberAliveNeighbors == 3)) {
			copyCell[i*WIDTH + j].alive[currentSpecies] = true;
		}
		else {
			copyCell[i*WIDTH + j].alive[currentSpecies] = false;
		}
	}
}

float4 rand(uint2 *state)
{
	const float4 invMaxInt = (float4) (1.0f / 4294967296.0f, 1.0f / 4294967296.0f, 1.0f / 4294967296.0f, 0);
	uint x = (*state).x * 17 + (*state).y * 13123;
	(*state).x = (x << 13) ^ x;
	(*state).y ^= (x << 7);

	uint4 tmp = (uint4)
		((x * (x * x * 15731 + 74323) + 871483),
		(x * (x * x * 13734 + 37828) + 234234),
			(x * (x * x * 11687 + 26461) + 137589), 0);

	return convert_float4(tmp) * invMaxInt;
}

__kernel void initializeCells(__global struct Cell* cellGrid, __global const int *numSpecies, __global const float *seed)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	int WIDTH = get_global_size(0);

	uint2 state = (uint2) ((int) *seed, y*WIDTH + x );

	float4 randomGen = rand(&state) * 1000;

	int rand1 = (((int)randomGen.y) % *numSpecies);
	int rand2 = ((int)randomGen.x) % 100;

	int speciesID = rand1;
	if (rand2 <=50) {
		cellGrid[y*WIDTH + x].alive[speciesID] = true;
	}
	else
		cellGrid[y*WIDTH + x].alive[speciesID] = false;
}



__kernel void calcImage(__global const struct Cell* cellGrid, __global const int *currentSpecies, write_only image2d_t rgb, __global const int *WIDTH, __global const int *HEIGHT, __global const float *color)
{
	float4 m_color = (float4) (color[0], color[1], color[2], 1.0f);
	for (int y = 0; y < *HEIGHT; y++) {
		for (int x = 0; x < *WIDTH; x++) {
			if (cellGrid[y*(*WIDTH) + x].alive[*currentSpecies]) {
				int2 coord = (int2) (x, y);
				write_imagef(rgb, coord, m_color);
			}
		}
	}

}

__kernel void calcImageDataParallel(__global const struct Cell* cellGrid, __global const int * numSpecies, write_only image2d_t rgb, __global const float *colorList)
{

	int x = get_global_id(0);
	int y = get_global_id(1);

	int WIDTH = get_global_size(0);


	for (int i = 0; i < *numSpecies; i++) {
		float4 m_color = (float4) (colorList[i*3 +0], colorList[i * 3 + 1], colorList[i * 3 + 2], 1.0f);
		if (cellGrid[y*(WIDTH) + x].alive[i]) {
			int2 coord = (int2) (x, y);
			write_imagef(rgb, coord, m_color);
		}
	}

}