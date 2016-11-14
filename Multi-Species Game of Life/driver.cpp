#define GLEW_STATIC
#include <windows.h>
#include <GL\glew.h>
#include <GLFW\glfw3.h>
#include <glm\ext.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <CImg.h>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype>
#include <thread>

#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/blocked_range2d.h>
#include <tbb/tbbmalloc_proxy.h>

using namespace tbb;

#define WIDTH 1024
#define HEIGHT 768
#define LIFE_RATE 70
#define MAX_COLORS 10
#define MAX_FRAMES 30

struct Cell {
	bool *alive;
};

struct Color {
	GLubyte R;
	GLubyte G;
	GLubyte B;
};

struct Species {
	Color color;
	int id;
};

GLubyte colorList[11][3] = {
	{ 255, 0, 0 },
	{0, 255, 0},
	{0, 0, 255},
	{1, 255, 254},
	{255, 166, 254},
	{255, 219, 102},
	{0, 100, 1},
	{149, 0, 58},
	{255, 0, 246},
	{255, 147, 126},
	{254, 137, 0}
};

int numSpecies = 0;

//Prototypes
void tickGame();
void initializeGrid(int numSpecies, int beginY, int endY, int beginX, int endX);
void initializeGame();
void initializeSpecies(Species* s, int numSpecies);
void render(GLubyte *rgb);
bool initializeOpenGL();
bool cleanUp();

GLFWwindow* window = 0x00;
Species *species;

GLfloat point_size = 3.0f;

Cell *cellGrid, *copyCell;
GLubyte *rgb;
int main() {
	initializeOpenGL();
	cellGrid = new Cell[WIDTH*HEIGHT];
	copyCell = new Cell[WIDTH*HEIGHT];
	initializeGame();
	while (!glfwWindowShouldClose(window)){
		tickGame();
		render(rgb);
		Cell *tmp = cellGrid;
		cellGrid = copyCell;
		copyCell = tmp;
	}
	cleanUp();
	return 0;
}

class computeTick {
	Cell *cellGrid, *copyCell;
	GLubyte *rgb;
	Species *species;
public:
	computeTick(GLubyte *rgb, Cell *c, Cell *c2, Species *s) :
		rgb(rgb), cellGrid(c), copyCell(c2), species(s) {}
	void operator()(const blocked_range2d<size_t>& r) const {
		for (int i = r.cols().begin(); i < r.cols().end(); i++) {
			for (int j = r.rows().begin(); j < r.rows().end(); j++) {
				for (int currentSpecies = 0; currentSpecies < numSpecies; currentSpecies++) {
					if (cellGrid[i*WIDTH + j].alive != NULL && cellGrid[i*WIDTH + j].alive[currentSpecies]) {
						rgb[i*(WIDTH * 3) + (j * 3) + 0] += species[currentSpecies].color.R;
						rgb[i*(WIDTH * 3) + (j * 3) + 1] += species[currentSpecies].color.G;
						rgb[i*(WIDTH * 3) + (j * 3) + 2] += species[currentSpecies].color.B;
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
		}
	}
};

void tickGame() {
	rgb = new GLubyte[HEIGHT*WIDTH*3]();
	tbb:parallel_for(blocked_range2d<size_t>(0, WIDTH, 0, HEIGHT), computeTick(rgb, cellGrid, copyCell, species), auto_partitioner());
}

void initializeGame() {
	std::cout << "Indicate the number of species (Recommended 5 - 10): ";
	std::cin >> numSpecies;
	species = new Species[numSpecies];
	initializeSpecies(species, numSpecies);
	initializeGrid(numSpecies, 0, HEIGHT, 0, WIDTH);
}



void initializeGrid(int numSpecies, int beginY, int endY, int beginX, int endX) {
	srand(time(NULL));
	for (int i = beginY; i < endY; i++) {
		for (int j = beginX; j < endX; j++) {
			int speciesID = (std::rand() % numSpecies);
			cellGrid[i*WIDTH +j].alive = new bool[numSpecies]();
			copyCell[i*WIDTH + j].alive = new bool[numSpecies]();
			for (int k = 0; k < numSpecies; k++)
				cellGrid[i*WIDTH + j].alive[k] = false;
			float rand = ((std::rand() % 10000) + 1) / 100.0f;
			if (rand <= LIFE_RATE) {
				cellGrid[i*WIDTH + j].alive[speciesID] = true;
			}
		}
	}
}

void initializeSpecies(Species* s, int numSpecies) {
	for (int i = 0; i < numSpecies; i++) {
		Species *sp = new Species;
		sp->color.R = colorList[i][0];
		sp->color.G = colorList[i][1];
		sp->color.B = colorList[i][2];
		sp->id = i;
		s[i] = *sp;
	}
}

void render(GLubyte *rgb) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
	glPointSize(point_size);
	glDrawPixels(WIDTH,HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, rgb);
	delete[] rgb;
	rgb = NULL;
	glfwPollEvents();
	glfwSwapBuffers(window);
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
	return true;
}

bool cleanUp() {
	glfwTerminate();
	delete[] cellGrid;
	delete[] copyCell;
	delete[] species;
	return true;
}