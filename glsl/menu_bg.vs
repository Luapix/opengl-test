#version 330 core

layout (location = 0) in vec2 attrPos;

out vec2 vertPos;

void main() {
	gl_Position = vec4(attrPos, 0.0, 1.0);
	vertPos = attrPos;
}
