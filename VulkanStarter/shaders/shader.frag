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
layout(location = 7) in float fragSpecularCoefficient;
layout(location = 8) in vec3 fragNormal;
layout(location = 9) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 ambient = fragAmbientLighting * fragColor;

	vec3 norm = normalize(fragNormal);
	vec3 lightDir = normalize(fragLightVector - fragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * fragColor;

	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = fragSpecularCoefficient * spec * fragColor;

	vec4 textureColor = texture(texSampler, fragTexCoord);

	vec3 lightingColor = (ambient + diffuse + specular);

	outColor = vec4(lightingColor, 1.0) * textureColor;
}