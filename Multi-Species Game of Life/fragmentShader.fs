#version 330

in vec3 out_Color;
in vec2 TextCoord;

uniform sampler2D texture_diffuse1;		// Main Texture

out vec4 frag_colour;	//final output color used to render the point

void main () {

	vec3 color = texture(texture_diffuse1, TextCoord).rgb;


	frag_colour = vec4 (color, 1.0);
}