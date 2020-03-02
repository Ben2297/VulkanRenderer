#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform LightingConstants {
	vec4 lightPosition;
	vec4 lightAmbient;
	vec4 lightDiffuse;
	vec4 lightSpecular;
	float lightSpecularExponent;
} lighting;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragLightVector;
layout(location = 3) out vec4 fragEyeVector;
layout(location = 4) out vec3 fragSpecularLighting;
layout(location = 5) out vec3 fragDiffuseLighting;
layout(location = 6) out vec3 fragAmbientLighting;
layout(location = 7) out vec3 fragSpecularCoefficient;
layout(location = 8) out vec4 fragNormal;


void main() {
	vec4 VCS_position = ubo.view * ubo.model * inPosition;

	fragNormal = ubo.view * ubo.model * inNormal;

	fragLightVector = ubo.view * lighting.lightPosition - VCS_position;

	fragEyeVector = - VCS_position;

    gl_Position = ubo.proj * VCS_position;

    fragColor = inColor;
	fragTexCoord = inTexCoord;

	fragSpecularLighting = lighting.lightSpecular;
	fragDiffuseLighting = lighting.lightDiffuse;
	fragAmbientLighting = lighting.lightAmbient;
	fragSpecularCoefficient = lighting.lightSpecularExponent;
}