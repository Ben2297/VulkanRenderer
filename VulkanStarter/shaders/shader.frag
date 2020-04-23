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
layout(location = 10) in float fragRenderTex;
layout(location = 11) in float currLayer;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 textureColor = vec3(texture(texSampler, fragTexCoord));
	float alpha = texture(texSampler, fragTexCoord).r;

	if (fragRenderTex == 0.0f) {
		textureColor = vec3(1.0f, 1.0f, 1.0f);
	}

	vec3 furColor = {0.96f, 0.95f, 0.035f};
	
	vec3 off = {0.0f, 0.0f, 0.0f};
	vec3 ambient = {0.0f, 0.0f, 0.0f};
	if (fragAmbientLighting != off) {
		ambient = (fragAmbientLighting * textureColor) * 0.6;
	} else {
		ambient = textureColor;
	}
	
	vec3 lightDir = normalize(fragLightVector - fragPos);
	vec3 normal = normalize(fragNormal);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = (diff * (fragDiffuseLighting * textureColor) * 0.5);

	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), fragSpecularCoefficient);
	vec3 specular = (fragSpecularLighting * spec) * 0.2;

	float shadow = mix(0.4f, 1.0f, currLayer);
	furColor *= shadow;

	if (currLayer == 0.0f)
	{
		alpha = 1.0f;
	}
	
	outColor = vec4(furColor, alpha);
}