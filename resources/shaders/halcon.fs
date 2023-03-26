#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;


struct PointLight{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};


struct Material{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};


uniform PointLight pointLight;
uniform vec3 viewPosition;
uniform Material material;


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);


void main()
{
   vec3 normal = normalize(Normal);
   vec3 viewDir = normalize(viewPosition - FragPos);
   vec3 result = CalcPointLight(pointLight, normal, FragPos, viewDir);


   FragColor = vec4(result, 1.0);
}


vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);

    float spec = 0.0f;
    spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords).xxx);
    ambient *= attenuation*2.5;
    diffuse *= attenuation*3.5;
    specular *= attenuation*2.5;
    return (ambient + diffuse + specular);
}

