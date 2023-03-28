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


in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D tex;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLight;

void main()
{
    vec3 color = texture(tex, TexCoords).rgb;
    vec3 viewDir = normalize(viewPos - FragPos);

    // diffuse
//     vec3 lightDir = normalize(lightPos - FragPos);
    vec3 lightDir = normalize(-dirLight.direction);
    vec3 normal = normalize(Normal);
    float diff = max(dot(normal, lightDir), 0.0);

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;

    spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);

    vec3 ambient = dirLight.ambient * color; // let light.ambient = 0.42f;
    vec3 diffuse = dirLight.diffuse * diff * color;
    vec3 specular = dirLight.specular * spec * color;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

// vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color)
// {
//     vec3 lightDir = normalize(light.position - fragPos);
//     float diff = max(dot(lightDir, normal), 0.0);
//
//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = 0.0;
//     spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
//
//     vec3 ambient = light.ambient * color;
//     vec3 diffuse = light.diffuse * diff * color;
//     vec3 specular = light.specular * spec * color;
//
//     ambient *= attenuation;
//     diffuse *= attenuation;
//     specular *= attenuation;
//     return (ambient + diffuse + specular);
// }