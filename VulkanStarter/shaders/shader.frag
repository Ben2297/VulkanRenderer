#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform LightingConstants{
    vec3 lightPos; 
	vec3 viewPos; 
	vec3 lightColor;
	vec3 objectColor;
} lighting;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 fragColor;

void main() {
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lighting.lightColor;
  	
    // diffuse 
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lighting.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lighting.lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(lighting.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lighting.lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * lighting.objectColor;
    fragColor = vec4(result, 1.0);
}