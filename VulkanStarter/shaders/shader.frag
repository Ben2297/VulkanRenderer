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
layout(location = 12) in float fragRenderShadowMap;

layout(location = 0) out vec4 outColor;

float ShadowCalculation(vec4 fragShadowCoord)
{
    //Perform perspective divide
    vec3 projCoords = fragShadowCoord.xyz / fragShadowCoord.w;

    //Transform to [0,1] range
    //projCoords = projCoords * 0.5 + 0.5;

    //Get depth from shadow map
    float closestDepth = texture(shadowSampler, projCoords.xy).r;

    //Get depth of fragment from light coordinates
    float currentDepth = projCoords.z;

	//Calculate depth bias
	vec3 lightDir = normalize(fragLightVector - fragPos);

	float cosTheta = clamp(dot(normalize(fragNormal), lightDir), 0, 1.0);
	float bias = 0.0005*tan(acos(cosTheta));
	bias = clamp(bias, 0, 0.01);

	//Check if in shadow and perform PCF
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowSampler, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(shadowSampler, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;

	if(projCoords.z > 1.0)
        shadow = 0.0;
    
    return shadow;
}  

void main() {

	float shadow = ShadowCalculation(fragShadowCoord);
	float visibility = 1.0;
	if (shadow == 1.0)
	{
		visibility = 0.5;
	}

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
		ambient = fragColor;
	}

	//Set diffuse lighting
	vec3 diffuse = {0.0f, 0.0f, 0.0f};
	vec3 lightDir = normalize(fragLightVector - fragPos);
	vec3 normal = normalize(fragNormal);
	float diff = max(dot(normal, lightDir), 0.0);
	if (fragDiffuseLighting != off)
	{
		diffuse = (diff * (visibility * fragDiffuseLighting * fragColor) * 0.5);
	}

	//Set specular lighting
	vec3 specular = {0.0f, 0.0f, 0.0f};
	vec3 viewDir = normalize(fragEyeVector - fragPos);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	float spec = pow(max(dot(normal, halfwayDir), 0.0), fragSpecularCoefficient);
	if (fragSpecularLighting != off)
	{
		specular = (visibility * fragSpecularLighting * spec * fragColor) * 0.2;
	}

	//vec3 lightingColor = (ambient + (1.0 - shadow) * (diffuse + specular));
	vec3 lightingColor = (ambient + diffuse + specular);

	if (fragRenderShadowMap == 0.0f)
	{
		lightingColor = vec3(shadow);
	}
	
	outColor = vec4(lightingColor, 1.0f);
}