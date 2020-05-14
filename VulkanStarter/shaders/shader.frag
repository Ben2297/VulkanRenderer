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
layout(location = 11) in vec4 fragShadowCoord;

layout(location = 0) out vec4 outColor;

void main() {

	//Set texture color
	vec3 textureColor = texture(texSampler[0], fragTexCoord).rgb;
	if (fragRenderTex == 0.0f) {
		textureColor = vec3(1.0f, 1.0f, 1.0f);
	}
	
	//Set ambient lighting
	vec3 off = {0.0f, 0.0f, 0.0f};
	vec3 ambient = {0.0f, 0.0f, 0.0f};
	if (fragAmbientLighting != off)
	{
		ambient = (fragAmbientLighting * fragColor) * 0.6;
	} else
	{
		//ambient = fragColor;
		ambient = (vec3(0.8f, 0.8f, 0.8f) * fragColor) * 0.6;
	}

	//Set diffuse lighting
	vec3 diffuse = {0.0f, 0.0f, 0.0f};
	vec3 lightDir = normalize(fragLightVector - fragPos);
	vec3 normal = normalize(fragNormal);
	float diff = max(dot(normal, lightDir), 0.0);
	if (fragDiffuseLighting != off)
	{
		diffuse = (diff * (fragDiffuseLighting * fragColor) * 0.5);
	}

	//Set specular lighting
	vec3 specular = {0.0f, 0.0f, 0.0f};
	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), fragSpecularCoefficient);
	if (fragSpecularLighting != off)
	{
		specular = (fragSpecularLighting * spec * fragColor) * 0.2;
	}
	
	float shadowDepth = texture(shadowSampler, fragShadowCoord.xy).r;

	float fragmentDepth = fragShadowCoord.z;

	if (shadowDepth < fragmentDepth)
	{
		//diffuse *= 0.5;
		//specular *= 0.5;
	}

	vec3 lightingColor = (ambient + diffuse + specular);
	
	outColor = vec4(lightingColor, 1.0f);
}