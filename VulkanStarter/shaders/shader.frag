#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragLightVector;
layout(location = 3) in vec3 fragEyeVector;
layout(location = 4) in vec3 fragSpecularLighting;
layout(location = 5) in vec3 fragDiffuseLighting;
layout(location = 6) in vec3 fragAmbientLighting;
layout(location = 7) in vec3 fragSpecularCoefficient;
layout(location = 8) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 ambientLight = fragAmbientLighting * fragColor;

	vec3 normEyeVector = normalize(fragEyeVector);
	vec3 normLightVector = normalize(fragLightVector);
	vec3 normNormal = normalize(fragNormal);

	float diffuseDotProduct = dot(normLightVector, normNormal);
	vec3 diffuseLight = fragAmbientLighting * fragColor * diffuseDotProduct;
}