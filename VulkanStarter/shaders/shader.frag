#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragLightVector;
layout(location = 3) in vec4 fragEyeVector;
layout(location = 4) in vec3 fragSpecularLighting;
layout(location = 5) in vec3 fragDiffuseLighting;
layout(location = 6) in vec3 fragAmbientLighting;
layout(location = 7) in vec3 fragSpecularCoefficient;
layout(location = 8) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
	vec4 ambientLight = fragAmbientLighting * fragColor;

	vec4 normEyeVector = normalise(fragEyeVector);
	vec4 normLightVector = normalise(fragLightVector);
	vec4 normNormal = normalise(fragNormal);

	float diffuseDotProductProduct = dot(normLightVector, normNormal);
	vec4 diffuseLight = fragAmbientLighting * fragColor * diffuseDotProduct;

	vec4 halfAngleVector = normalise((normEyeVector + normLightVector) / 2.0);
	float specularDotProduct = dot(halfAngleVector, normNormal);
	float specularPower = pow(specularDotProduct, fragSpecularCoefficient);
	vec4 specularLight = fragSpecularLighting * fragColor * specularPower;

	vec4 lightingColor = ambientLight + diffuseLight + specularLight;

	vec4 textureColor = texture(texSampler, fragTexCoord);

    outColor = lightingColor * textureColor;
}