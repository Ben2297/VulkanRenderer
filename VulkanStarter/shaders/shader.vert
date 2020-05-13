#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float renderTex;
} ubo;

layout(binding = 1) uniform LightingConstants {
    vec3 lightPosition; 
	vec3 lightAmbient; 
	vec3 lightDiffuse;
	vec3 lightSpecular;
	float lightSpecularExponent;
} lighting;

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
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragLightVector;
layout(location = 3) out vec3 fragEyeVector;
layout(location = 4) out vec3 fragSpecularLighting;
layout(location = 5) out vec3 fragDiffuseLighting;
layout(location = 6) out vec3 fragAmbientLighting;
layout(location = 7) out float fragSpecularCoefficient;
layout(location = 8) out vec3 fragNormal;
layout(location = 9) out vec3 fragPos;
layout(location = 10) out float fragRenderTex;
layout(location = 11) out vec3 fragShadowCoord;

void main() {
	vec4 WCS_position = ubo.model * vec4(inPosition, 1.0);

	vec4 LCS_position = shadow.view * WCS_position;

	fragShadowCoord = vec3(shadow.proj * LCS_position);

	vec4 VCS_position = ubo.view * WCS_position;

    fragPos = vec3(ubo.model * vec4(inPosition, 1.0));
	fragNormal = vec3(ubo.model * vec4(inNormal, 1.0));
	fragColor = inColor;
	fragTexCoord = inTexCoord;

	fragEyeVector = vec3(0.0f, 40.0f, 70.0f);

	fragLightVector = lighting.lightPosition;
	fragSpecularLighting = lighting.lightSpecular;
	fragDiffuseLighting = lighting.lightDiffuse;
	fragAmbientLighting = lighting.lightAmbient;
	fragSpecularCoefficient = lighting.lightSpecularExponent;

	fragRenderTex = ubo.renderTex;

	gl_Position = ubo.proj * VCS_position;
}