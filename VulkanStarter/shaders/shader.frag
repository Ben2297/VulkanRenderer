#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D texSampler;

layout(binding = 3) uniform RenderOptions {
	bool renderTex;
} renderOptions;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragLightVector;
layout(location = 3) in vec3 fragEyeVector;
layout(location = 4) in vec3 fragSpecularLighting;
layout(location = 5) in vec3 fragDiffuseLighting;
layout(location = 6) in vec3 fragAmbientLighting;
layout(location = 7) in float fragSpecularCoefficient;
layout(location = 8) in vec3 fragNormal;
layout(location = 9) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 textureColor = vec3(texture(texSampler, fragTexCoord));

	vec3 ambient = fragColor;
	if (renderOptions.renderTex) {
		ambient = fragAmbientLighting * textureColor;
	} else {
		ambient = fragAmbientLighting * fragColor;
	}

	vec3 lightDir = normalize(fragLightVector - fragPos);
	vec3 normal = normalize(fragNormal);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = {0.0f, 0.0f, 0.0f};
	if (renderOptions.renderTex) {
		vec3 diffuse = diff * textureColor;
	} else {
		vec3 diffuse = diff * fragColor;
	}

	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), fragSpecularCoefficient);
	vec3 specular = fragSpecularLighting * spec;

	outColor = vec4(ambient + diffuse + specular, 1.0);
}