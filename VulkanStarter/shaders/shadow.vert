#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float renderTex;
} ubo;

layout(binding = 3) uniform ShadowBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} shadow;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;

void main() {
    vec4 WCS_position = ubo.model * vec4(inPosition, 1.0);

	vec4 LCS_position = shadow.view * WCS_position;

	fragColor = vec3(0.0, 0.0, 0.0);

	gl_Position = shadow.proj * LCS_position;
}