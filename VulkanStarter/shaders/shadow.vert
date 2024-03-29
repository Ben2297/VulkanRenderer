#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float renderTex;
	mat4 mvp;
} ubo;

layout(binding = 3) uniform ShadowBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	mat4 mvp;
	mat4 biasmvp;
} shadow;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

void main() {
    gl_Position = shadow.mvp * vec4(inPosition, 1.0);
}