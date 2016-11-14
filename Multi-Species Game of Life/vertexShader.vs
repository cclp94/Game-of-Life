#version 330

uniform mat4 view_matrix, model_matrix, proj_matrix;

layout (location=0)in  vec3 in_Position;		//vertex position
layout (location=1) in vec2 texCoord;

out vec3 out_Color;
out vec2 TextCoord;

void main () {
	mat4 CTM = proj_matrix * view_matrix * model_matrix;
	gl_Position = CTM * vec4 (in_Position, 1.0);

	out_Color = vec3 (0.8,0.2,0.2);
	TextCoord = texCoord;
}