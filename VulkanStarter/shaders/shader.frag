#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D texSampler[2];

layout(binding = 4) uniform sampler2D shadowSampler;

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
layout(location = 11) in vec3 fragShadowCoord;

layout(location = 0) out vec4 outColor;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * 0.1 * 300.0) / (300.0 + 0.1 - z * (300.0 - 0.1));
}

void main() {
	float shadowDepth = texture(shadowSampler, fragShadowCoord.xy).r;

	float fragmentDepth = fragShadowCoord.z;

	vec3 textureColor = vec3(texture(texSampler[0], fragTexCoord));

	textureColor = vec3(0.862f, 0.854f, 0.854f);

	if (fragRenderTex == 0.0f) {
		textureColor = vec3(1.0f, 1.0f, 1.0f);
	}
	
	vec3 off = {0.0f, 0.0f, 0.0f};
	vec3 ambient = {0.0f, 0.0f, 0.0f};
	if (fragAmbientLighting != off) {
		ambient = (fragAmbientLighting * fragColor) * 0.6;
	} else {
		ambient = textureColor;
	}

	vec3 lightingColor = ambient;
	
	vec3 lightDir = normalize(fragLightVector - fragPos);
	vec3 normal = normalize(fragNormal);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = (diff * (fragDiffuseLighting * fragColor) * 0.5);

	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), fragSpecularCoefficient);
	vec3 specular = (fragSpecularLighting * spec * fragColor) * 0.2;

	if (shadowDepth < fragmentDepth)
	{
		lightingColor += diffuse + specular;
	}
	else
	{
		lightingColor = vec3(0.0f, 0.0f, 0.0f);
	}
	
	outColor = vec4(vec3(shadowDepth), 1.0f);
}