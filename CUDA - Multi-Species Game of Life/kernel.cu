#define GLEW_STATIC
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include "cuda_runtime.h"
#include "cuda_runtime_api.h"
#include "device_launch_parameters.h"
#include "curand_kernel.h"
#include "cuda_gl_interop.h"
#include <time.h>

#include <stdio.h>
#include <iostream>


#define WIDTH 1024
#define HEIGHT 768
#define LIFE_RATE 70
#define MAX_COLORS 10
#define MAX_FRAMES 30

struct Cell {
	bool alive[10] = { false };
};

GLubyte colorList[11*3] = {
	 255, 0, 0 ,
	 0, 255, 0 ,
	 0, 0, 255 ,
	 1, 255, 254 ,
	 255, 166, 254 ,
	 255, 219, 102 ,
	 0, 100, 1 ,
	 149, 0, 58 ,
	 255, 0, 246 ,
	 255, 147, 126 ,
	 254, 137, 0 
};

int numSpecies = 0;

using namespace std;

cudaError_t gameOfLife();
void tickGame(GLubyte *rgb, Cell *device_cell, Cell *device_result_cell, GLubyte *device_colorList);
bool initializeOpenGL();
bool cleanUp();
void render(GLubyte *rgb);
void initializeGame(Cell *device_cell, Cell *device_result_cell);

GLFWwindow* window = 0x00;
GLfloat point_size = 3.0f;

GLubyte *rgb;
GLuint TEX, PBO;
cudaGraphicsResource *CUDA_PBO;

int main()
{
	cudaError_t cudaStatus = gameOfLife();
	cleanUp();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "addWithCuda failed!");
        return 1;
    }
    // cudaDeviceReset must be called before exiting in order for profiling and
    // tracing tools such as Nsight and Visual Profiler to show complete traces.
    cudaStatus = cudaDeviceReset();
    if (cudaStatus != cudaSuccess) {
        fprintf(stderr, "cudaDeviceReset failed!");
        return 1;
    }
    return 0;
}

cudaError_t gameOfLife() {
	cudaError_t cudaStatus;
	GLubyte *device_colorList;
	

	Cell *device_cell, *device_result_cell;
	initializeOpenGL();
	// Choose which GPU to run on, change this on a multi-GPU system.
	try {
		cudaStatus = cudaSetDevice(0);
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
			throw - 1;
		}

		// Allocate GPU buffers for Cell Grids  .
		cudaStatus = cudaMalloc((void**)&device_cell, WIDTH*HEIGHT * sizeof(Cell));
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMalloc failed!");
			throw - 1;
		}

		cudaStatus = cudaMalloc((void**)&device_result_cell, WIDTH*HEIGHT * sizeof(Cell));
		if (cudaStatus != cudaSuccess) {
			fprintf(stderr, "cudaMalloc failed!");
			throw -1;
		}

		initializeGame(device_cell, device_result_cell);

		cudaMalloc((void**)&device_colorList, 11 * 3 * sizeof(GLubyte));

		cudaMemcpy(device_colorList, colorList, 11 * 3 * sizeof(GLubyte), cudaMemcpyHostToDevice);

		glfwSetTime(0);
		int nFrames = 0;
		while (!glfwWindowShouldClose(window)) {
			nFrames++;
			tickGame(rgb, device_cell, device_result_cell, device_colorList);
			render(rgb);
			Cell *tmp = device_cell;
			device_cell = device_result_cell;
			device_result_cell = tmp;
			if (1.0 <= glfwGetTime()) {
				cout << nFrames << " fps" << endl;
				nFrames = 0;
				glfwSetTime(0);
			}
		}

		cudaFree(device_colorList);
		cudaFree(device_cell);
		cudaFree(device_result_cell);
		delete[] rgb;
	}
	catch (int e) {
		cudaFree(device_cell);
		cudaFree(device_result_cell);
	}
	return cudaStatus;
}

__global__ void computeInteration(int numSpecies, GLubyte *rgb, Cell *cellGrid, Cell *copyCell, GLubyte *species) {

	int i = blockIdx.y * blockDim.y + threadIdx.y;
	int j = blockIdx.x * blockDim.x + threadIdx.x;
	rgb[i*(WIDTH * 3) + (j * 3) + 0] = 0;
	rgb[i*(WIDTH * 3) + (j * 3) + 1] = 0;
	rgb[i*(WIDTH * 3) + (j * 3) + 2] = 0;
	for (int currentSpecies = 0; currentSpecies < numSpecies; currentSpecies++) {
		if (cellGrid[i*WIDTH + j].alive != NULL && cellGrid[i*WIDTH + j].alive[currentSpecies]) {
			rgb[i*(WIDTH * 3) + (j * 3) + 0] += species[(currentSpecies * 3) + 0];
			rgb[i*(WIDTH * 3) + (j * 3) + 1] += species[(currentSpecies * 3) + 1];
			rgb[i*(WIDTH * 3) + (j * 3) + 2] += species[(currentSpecies * 3) + 2];
		}
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

void tickGame(GLubyte *rgb, Cell *device_cell, Cell *device_result_cell, GLubyte *device_colorList) {
	cudaGraphicsMapResources(1, &CUDA_PBO, 0);
	GLubyte *device_rgb;
	size_t num_bytes = WIDTH*HEIGHT*3;
	cudaGraphicsResourceGetMappedPointer((void**)&device_rgb,
		&num_bytes, CUDA_PBO);

	
	dim3  grid(128, 64);
	dim3 block(WIDTH / grid.x, HEIGHT / grid.y);
	computeInteration << <grid, block >> > (numSpecies, device_rgb, device_cell, device_result_cell, device_colorList);
	cudaDeviceSynchronize();
	cudaError_t cudaStatus;
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
		throw - 1;
	}
	cudaGraphicsUnmapResources(1, &CUDA_PBO, 0);
}

__global__ void initialState(int numSpecies, Cell *cellGrid, Cell *copyCell)
{

	int j = blockIdx.y * blockDim.y + threadIdx.y;
	int i = blockIdx.x * blockDim.x + threadIdx.x;

	curandState_t state;

	/* we have to initialize the state */
	curand_init(clock(), 0, 0, &state);

	int rand1 = (curand(&state) % numSpecies);
	int rand2 = curand(&state) % 100;

	int speciesID = rand1;
	if (rand2 <= LIFE_RATE) {
		cellGrid[j*WIDTH + i].alive[speciesID] = true;
	}else
		cellGrid[j*WIDTH + i].alive[speciesID] = false;
}

void initializeGame(Cell *device_cell, Cell *device_result_cell) {
	dim3  grid(512, 256);
	dim3 block(WIDTH/ grid.x, HEIGHT/ grid.y);
	cudaError_t cudaStatus;
	std::cout << "Indicate the number of species (Recommended 5 - 10): ";
	std::cin >> numSpecies;
	initialState <<<grid, block >>>(numSpecies, device_cell, device_result_cell);
	cudaDeviceSynchronize();
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
		throw - 1;
	}
}

bool initializeOpenGL() {
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Multi-Species Game", NULL, NULL);
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glViewport(0, 0, w, h);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);


	rgb = new GLubyte[WIDTH * HEIGHT*3];

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &TEX);
	glBindTexture(GL_TEXTURE_2D, TEX);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
		GL_UNSIGNED_BYTE, rgb);

	glGenBuffers(1, &PBO);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, PBO);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, WIDTH * HEIGHT * 3 * sizeof(GLubyte),
		rgb, GL_STREAM_COPY);

	cudaError result = cudaGraphicsGLRegisterBuffer(&CUDA_PBO, PBO,
		cudaGraphicsMapFlagsWriteDiscard);
	return result == cudaSuccess;
}

bool cleanUp() {
	glfwTerminate();
	return true;
}

void draw() {
	glBindTexture(GL_TEXTURE_2D, TEX);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, PBO);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT,
		GL_RGB, GL_UNSIGNED_BYTE, 0);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(-1, 1);
	glEnd();

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void render(GLubyte *rgb) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
	glPointSize(point_size);
	draw();
	glfwPollEvents();
	glfwSwapBuffers(window);
}
