#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextCoord;

void main() {
	outColor = vec4(fragTextCoord, 0.0, 1.0);
}