typedef struct Cell {
	bool alive[10];
};

__kernel void calcImage(__global const struct Cell* cellGrid, __global const int *currentSpecies, write_only image2d_t rgb, __global const int *WIDTH, __global const int *HEIGHT, __global const float *color)
{
	float4 m_color = (float4) (color[0], color[1], color[2], 1.0f);
	for(int y = 0; y < *HEIGHT; y++){
		for(int x = 0; x < *WIDTH; x++){ 
			if (cellGrid[y*(*WIDTH) + x].alive[*currentSpecies]){
				int2 coord = (int2) (x, y);
				write_imagef(rgb, coord,m_color);
			}
		}
	}

}