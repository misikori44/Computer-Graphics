#version 330 core
out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    float shininess;
};


in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D tex;
uniform bool blinn;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color);

void main()
{
    vec3 color = texture(tex, TexCoords).rgb;
    vec3 viewDir = normalize(viewPos - FragPos);

    // diffuse
    vec3 lightDir = normalize(-dirLight.direction);
    vec3 normal = normalize(Normal);
    float diff = max(dot(normal, lightDir), 0.0);

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;

    if(blinn){

        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    }
    else{

        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);

    }

    vec3 ambient = dirLight.ambient * color; // let light.ambient = 0.42f;
    vec3 diffuse = dirLight.diffuse * diff * color;
    vec3 specular = dirLight.specular * spec * color;

    vec3 point = CalcPointLight(pointLight, normal, FragPos, viewDir, color);
    FragColor = vec4((ambient + diffuse + specular) + point, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);

    float spec = 0.0f;
    if(blinn){

        vec3 halfwayDir = normalize(lightDir + viewDir);
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0f);

    }else{

        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0f);

    }

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(tex, TexCoords).rgb);
    vec3 diffuse = light.diffuse * diff * vec3(texture(tex, TexCoords).rgb);
    vec3 specular = light.specular * spec * vec3(texture(tex, TexCoords).rgb);

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}