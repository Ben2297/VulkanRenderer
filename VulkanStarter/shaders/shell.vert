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

layout(push_constant) uniform PushConstants
{
    float currentLayer;
} constants;

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
layout(location = 11) out float currLayer;

void main() {
	vec3 gravity = {0.0f, -1.0f, 0.0f};
	gravity = vec3(vec4(gravity, 1.0) * ubo.model);
	float displacementFactor = pow(constants.currentLayer, 2);
	float maxHairLength = 2.0f;
	vec3 pos = inPosition + inNormal * maxHairLength * constants.currentLayer;
	pos = pos + gravity * displacementFactor;

    fragPos = vec3(ubo.model * vec4(pos, 1.0));
	fragNormal = mat3(transpose(inverse(ubo.model))) * inNormal;
	fragColor = inColor;
	fragTexCoord = inTexCoord * 6.0;

	fragEyeVector = vec3(30.0f, 0.0f, 30.0f);

	fragLightVector = lighting.lightPosition;
	fragSpecularLighting = lighting.lightSpecular;
	fragDiffuseLighting = lighting.lightDiffuse;
	fragAmbientLighting = lighting.lightAmbient;
	fragSpecularCoefficient = lighting.lightSpecularExponent;

	fragRenderTex = ubo.renderTex;

	currLayer = constants.currentLayer;

	gl_Position = ubo.proj * ubo.view * vec4(fragPos, 1.0);
}