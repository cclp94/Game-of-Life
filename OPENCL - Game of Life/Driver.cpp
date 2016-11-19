#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <CL\cl.h>
#include <time.h>

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>


#define WIDTH 1024
#define HEIGHT 768
#define LIFE_RATE 70
#define MAX_COLORS 10
#define MAX_FRAMES 30

typedef struct Cell {
	bool alive[10] = { false };
};

float colorList[11][3] = {
	{255.0f, 0.0f, 0.0f },
	{0.0f, 255.0f, 0.0f },
	{0.0f, 0.0f, 255.0f },
	{1.0f, 255.0f, 254.0f },
	{255.0f, 166.0f, 254.0f },
	{255.0f, 219.0f, 102.0f },
	{0.0f, 100.0f, 1.0f },
	{149.0f, 0.0f, 58.0f} ,
	{255.0f, 0.0f, 246.0f },
	{255.0f, 147.0f, 126.0f },
	{254.0f, 137.0f, 0.0f}
};

int numSpecies = 0;

using namespace std;

void displayPlatforms(int n, cl_platform_id platforms[100]);
void displayDevices(int n, cl_device_id devices[2]);

//cudaError_t gameOfLife();
//void tickGame(GLubyte *rgb, Cell *device_cell, Cell *device_result_cell, GLubyte *device_colorList);
//bool initializeOpenGL();
//bool cleanUp();
//void render(GLubyte *rgb);
//void initializeGame(Cell *device_cell, Cell *device_result_cell);
//
GLFWwindow* window = 0x00;
GLfloat point_size = 3.0f;
//
//GLubyte *rgb;
//GLuint TEX, PBO;
//cudaGraphicsResource *CUDA_PBO;

GLuint TEX, PBO;
float *rgb = new float[WIDTH*HEIGHT * 4];
float * renderTex;

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


	//glEnable(GL_TEXTURE_2D);
	//glGenTextures(1, &TEX);
	//glBindTexture(GL_TEXTURE_2D, TEX);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	//glTexImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT,
	//	GL_RGBA, GL_FLOAT, rgb);

	return true;
}

void render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
	glPointSize(point_size);
	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, TEX);
	//
	//glTexImage2D(GL_TEXTURE_2D, 0, 0, WIDTH, HEIGHT, 0,
	//	GL_RGBA, GL_FLOAT, rgb);
	//
	//glBegin(GL_QUADS);
	//glTexCoord2f(0, 0);
	//glVertex2f(-1, -1);
	//glTexCoord2f(1, 0);
	//glVertex2f(1, -1);
	//glTexCoord2f(1, 1);
	//glVertex2f(1, 1);
	//glTexCoord2f(0, 1);
	//glVertex2f(-1, 1);
	//glEnd();
	//
	//glBindTexture(GL_TEXTURE_2D, 0);

	glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_FLOAT, rgb);
	glfwPollEvents();
	glfwSwapBuffers(window);
}

int main()
{
	initializeOpenGL();

	//-----------------GET PLATFORMS-------------------
	cl_platform_id platforms[100];
	cl_uint platforms_n = 0;
	clGetPlatformIDs(100, platforms, &platforms_n);

	displayPlatforms(platforms_n, platforms);

	if (platforms_n == 0)
		return 1;

	//-----------------GET DEVICES---------------------
	cl_device_id gpu_devices[10], cpu_devices[10];
	cl_uint devices_n = 0, gpu_device_count = 0, cpu_device_count=0;
	cl_int err = 0;
	for (int i = 0; i < platforms_n; i++) {
		err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &gpu_devices[devices_n], &devices_n);
		gpu_device_count += devices_n;
	}
	devices_n = 0;
	for (int i = 0; i < platforms_n; i++) {
		err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &cpu_devices[devices_n], &devices_n);
		cpu_device_count += devices_n;
	}

	displayDevices(gpu_device_count, gpu_devices);
	displayDevices(cpu_device_count, cpu_devices);

	cl_context gpu_context, cpu_context;
	gpu_context = clCreateContext(NULL, 1, &gpu_devices[0], NULL, NULL, &err);
	cpu_context = clCreateContext(NULL, 1, &cpu_devices[0], NULL, NULL, &err);

	//---------------CREATE COMMAND QUEUE---------------
	
	cl_command_queue queue_gpu, queue_cpu;
	queue_gpu = clCreateCommandQueue(gpu_context, gpu_devices[0], 0, &err);
	queue_cpu = clCreateCommandQueue(cpu_context, cpu_devices[0], 0, &err);


	//--------------NUM SPECIES INPUT-----------------
	int numSpecies = 0;
	std::cout << "How many species? ";
	cin >> numSpecies;

	//------------INIT CELL GRID--------------------

	// Create buffer for species

	cl_mem device_cellGrid;

	device_cellGrid = clCreateBuffer(gpu_context,
		CL_MEM_READ_WRITE, sizeof(Cell) * WIDTH * HEIGHT,
		NULL, NULL);

	// Create buffer for numSpecies
	cl_mem device_numSpecies;

	device_numSpecies = clCreateBuffer(gpu_context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_int), &numSpecies, NULL);
	// Create buffer for time seed for ramdom
	cl_mem device_timeseed;
	int timeseed = clock();

	device_timeseed = clCreateBuffer(gpu_context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(cl_int), &timeseed, NULL);

	// Get init kernel source, compile program, enqueue for gpu
	FILE *fp;
	char *source;
	size_t source_size, program_size;

	fp = fopen("kernels.cl", "rb");
	if (!fp) {
		printf("Failed to load kernel\n");
		return 1;
	}

	fseek(fp, 0, SEEK_END);
	program_size = ftell(fp);
	rewind(fp);
	source = (char*)malloc(program_size + 1);
	source[program_size] = '\0';
	fread(source, sizeof(char), program_size, fp);
	fclose(fp);

	cl_program initializeCellsProgram; // OpenCL program
	cl_kernel initializeCellsKernel;
	// create the program, in this case from OpenCL C
	initializeCellsProgram = clCreateProgramWithSource(gpu_context,
		1, (const char**) &source, NULL, &err);
	// build the program
	std::cout << err << endl;
	err = clBuildProgram(initializeCellsProgram, 0, NULL, NULL,
		NULL, NULL);
	std::cout << err << endl;
	// create the kernel
	initializeCellsKernel = clCreateKernel(initializeCellsProgram, "initializeCells",
		&err);
	std::cout << err << endl;
	// set the kernel Argument values
	err = clSetKernelArg(initializeCellsKernel, 0,
		sizeof(cl_mem), (void*)&device_cellGrid);
	std::cout << err << endl;
	err |= clSetKernelArg(initializeCellsKernel, 1,
		sizeof(cl_mem), (void*)&device_numSpecies);
	err |= clSetKernelArg(initializeCellsKernel, 2,
		sizeof(cl_mem), (void*)&device_timeseed);

	size_t localWorkSize[2] = { 32, 32 };
	size_t globalWorkSize[2] = { WIDTH, HEIGHT };
	

	cl_event cellReady, readyNext, renderReady;

	err = clEnqueueNDRangeKernel(queue_gpu,
		initializeCellsKernel, 2, NULL, globalWorkSize,
		localWorkSize, 0, NULL, &cellReady);

	std::cout << err << endl;

	//------------------CREATE TICK KERNEL--------------------

	cl_kernel tickKernel;
	std::cout << err << endl;
	// create the kernel
	tickKernel = clCreateKernel(initializeCellsProgram, "computeTick",
		&err);
	std::cout << "Create tick kernel " << err << endl;


	cl_mem device_copyCell;

	device_copyCell = clCreateBuffer(gpu_context,
		CL_MEM_WRITE_ONLY, sizeof(Cell) * WIDTH * HEIGHT,
		NULL, NULL);

	err |= clSetKernelArg(tickKernel, 2,
		sizeof(cl_mem), (void*)&device_numSpecies);

	//--------------------CREATE CPU PROGRAM-------------------


	cl_program calcImageProgram; // OpenCL program
	cl_kernel calcImageKernel;

	// create the program, in this case from OpenCL C
	calcImageProgram = clCreateProgramWithSource(cpu_context,
		1, (const char**)&source, NULL, &err);
	// build the program
	std::cout << err << endl;
	err = clBuildProgram(calcImageProgram, 0, NULL, NULL,
		NULL, NULL);
	std::cout << err << endl;
	// create the kernel
	calcImageKernel = clCreateKernel(calcImageProgram, "calcImage",
		&err);

	// Create buffer for image2d rgb

	cl_image_format format;
	format.image_channel_data_type = CL_FLOAT;
	format.image_channel_order = CL_RGBA;

	// width & height
	cl_mem cpu_WIDTH, cpu_HEIGHT;
	int image_width = WIDTH;
	int image_height = HEIGHT;


	cpu_WIDTH = clCreateBuffer(cpu_context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(int), &image_width, NULL);

	cpu_HEIGHT = clCreateBuffer(cpu_context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		sizeof(int), &image_height, NULL);

	clSetKernelArg(calcImageKernel, 3,
		sizeof(cl_mem), (void*)&cpu_WIDTH);
	clSetKernelArg(calcImageKernel, 4,
		sizeof(cl_mem), (void*)&cpu_HEIGHT);

	//======================REPEAT=============================
	Cell * cellGrid = new Cell[WIDTH*HEIGHT];

	//------------------------------
	//-----------------------COMPUTE TICK----------------------
	glfwSetTime(0);
	int nFrames = 0;

	while (!glfwWindowShouldClose(window)) {
		nFrames++;
		//-----------------GET GRID VALUES--------------------
		err = clEnqueueReadBuffer(queue_gpu,
			device_cellGrid, CL_FALSE, 0,
			sizeof(Cell) * WIDTH * HEIGHT, cellGrid, 1,& cellReady,&readyNext);		//EVENT?
		
		cl_mem cpu_cellGrid;
		
		cpu_cellGrid = clCreateBuffer(cpu_context,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sizeof(Cell) * WIDTH * HEIGHT, cellGrid, NULL);


		//--------------------GET IMAGE TEXTURE----------------------
		// ------------ Assign statioc args--------------
		cl_mem output_image;
		output_image = clCreateImage2D(cpu_context, CL_MEM_WRITE_ONLY, &format,
			WIDTH, HEIGHT, 0, NULL, &err);

		err = clSetKernelArg(calcImageKernel, 0,
			sizeof(cl_mem), (void*)&cpu_cellGrid);
		err |= clSetKernelArg(calcImageKernel, 2,
			sizeof(cl_mem), (void*)&output_image);


		cl_mem cpu_currentSpecies;
		cl_mem cpu_color;
		cl_event * taskJoin = new cl_event[numSpecies];
		// Task for each species
		for (int i = 0; i < numSpecies; i++) {
			cpu_currentSpecies = clCreateBuffer(cpu_context,
				CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(int), &i, NULL);

			cpu_color = clCreateBuffer(cpu_context,
				CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(float) * 3, &colorList[i], NULL);

			err |= clSetKernelArg(calcImageKernel, 1,
				sizeof(cl_mem), (void*)&cpu_currentSpecies);

			clSetKernelArg(calcImageKernel, 5,
				sizeof(cl_mem), (void*)&cpu_color);

			err = clEnqueueTask(queue_cpu,
				calcImageKernel, 0, NULL, NULL);

			clReleaseMemObject(cpu_color);
			clReleaseMemObject(cpu_currentSpecies);
		}
		//-------------------------------------------------
		//-------------------COMPUTE GRID---------------
	
		// set the kernel Argument values
		err = clSetKernelArg(tickKernel, 0,
			sizeof(cl_mem), (void*)&device_cellGrid);
		err |= clSetKernelArg(tickKernel, 1,
			sizeof(cl_mem), (void*)&device_copyCell);
	
	
	
		err = clEnqueueNDRangeKernel(queue_gpu,
			tickKernel, 2, NULL, globalWorkSize,
			localWorkSize, 1, &readyNext, &cellReady);

		// Read Result rgb
		const size_t szTexOrigin[3] = { 0, 0, 0 };
		const size_t szTexRegion[3] = { WIDTH, HEIGHT, 1 };
		err = clEnqueueReadImage(queue_cpu,
			output_image, CL_FALSE, szTexOrigin, szTexRegion, 0, 0,
			rgb, 0, NULL, NULL);

		//------------SWAP CELLGRID-----------------------
		cl_mem tmp;
		tmp = device_cellGrid;
		device_cellGrid = device_copyCell;
		device_copyCell = tmp;

	//---------------------------RENDER----------------

		render();

		clReleaseMemObject(cpu_cellGrid);
		clReleaseMemObject(output_image);

		if (1.0 <= glfwGetTime()) {
			cout << nFrames << " fps" << endl;
			nFrames = 0;
			glfwSetTime(0);
		}
	}

	glfwTerminate();
	// Consume event done : get kernel for color calculation, enqueue for cpu waiting for previous kernel completion
	// When done, render texture

	// In the mean time, calculate next iteration with gpu kernel -> send event when done

	// ===========================
	free(source);
	clReleaseMemObject(device_cellGrid);
	clReleaseMemObject(device_copyCell);
	delete[] cellGrid;
	delete[] rgb;
	clReleaseMemObject(device_numSpecies);
	clReleaseMemObject(device_timeseed);
	clReleaseMemObject(cpu_HEIGHT);
	clReleaseMemObject(cpu_WIDTH);
	clReleaseKernel(initializeCellsKernel);
	clReleaseProgram(initializeCellsProgram);
	for (int i = 0; i < gpu_device_count; i++)	clReleaseDevice(gpu_devices[i]);
	for (int i = 0; i < cpu_device_count; i++)	clReleaseDevice(cpu_devices[i]);
	clReleaseKernel(tickKernel);
	clReleaseKernel(calcImageKernel);
	clReleaseProgram(calcImageProgram);
	clReleaseCommandQueue(queue_gpu);
	clReleaseCommandQueue(queue_cpu);
	clReleaseContext(gpu_context);
	clReleaseContext(cpu_context);
	return 0;
}



void displayPlatforms(int n, cl_platform_id platforms[100]) {
	printf("=== %d OpenCL platform(s) found: ===\n", n);
	for (int i = 0; i<n; i++)
	{
		char buffer[10240];
		printf("  -- %d --\n", i);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, 10240, buffer, NULL);
		printf("  PROFILE = %s\n", buffer);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_VERSION, 10240, buffer, NULL);
		printf("  VERSION = %s\n", buffer);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, 10240, buffer, NULL);
		printf("  NAME = %s\n", buffer);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, 10240, buffer, NULL);
		printf("  VENDOR = %s\n", buffer);
		clGetPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, 10240, buffer, NULL);
		printf("  EXTENSIONS = %s\n", buffer);
	}
}

void displayDevices(int n, cl_device_id devices[2]) {
	for (int i = 0; i < n; i++)
	{
		char buffer[10240];
		cl_uint buf_uint;
		cl_ulong buf_ulong;
		printf("  -- %d --\n", i);
		clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(buffer), buffer, NULL);
		printf("  DEVICE_NAME = %s\n", buffer);
		clGetDeviceInfo(devices[i], CL_DEVICE_VENDOR, sizeof(buffer), buffer, NULL);
		printf("  DEVICE_VENDOR = %s\n", buffer);
		clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, sizeof(buffer), buffer, NULL);
		printf("  DEVICE_VERSION = %s\n", buffer);
		clGetDeviceInfo(devices[i], CL_DRIVER_VERSION, sizeof(buffer), buffer, NULL);
		printf("  DRIVER_VERSION = %s\n", buffer);
		clGetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(buf_uint), &buf_uint, NULL);
		printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n", (unsigned int)buf_uint);
		clGetDeviceInfo(devices[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(buf_uint), &buf_uint, NULL);
		printf("  DEVICE_MAX_CLOCK_FREQUENCY = %u\n", (unsigned int)buf_uint);
		clGetDeviceInfo(devices[i], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(buf_ulong), &buf_ulong, NULL);
		printf("  DEVICE_GLOBAL_MEM_SIZE = %llu\n", (unsigned long long)buf_ulong);
	}
}
