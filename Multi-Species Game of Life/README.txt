Caio Correa Leal Paiva
Student ID: 27339887
COMP 426

Libraries used:
GLFW 3
GLEW
TBB

Developed with Visual Studio 2015

To Run the program:
Open solution on VS 2015
Go to the project's properties
	In the VC++ Directories tab
		Edit the Include directories to the include folder in the project
		Edit the Libraries directories to the Lib folder in the project
	In the C/C++ tab, under additional include directories add the TBB include folder
	In the Linker Tab/ 
	In the General sub tab:
		Add the TBB lib with the VS version under additional library directories
	In the Input sub tab:
		Edit Additional Dependencies and add:
			opengl32.lib
			glfw3.lib
			glew32s.lib