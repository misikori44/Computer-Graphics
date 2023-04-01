#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
//#include <rg/Camera.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
unsigned int loadCubemap(vector<std::string> faces);
unsigned int loadTexture(const char *path);
// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 600;

// camera
// rg camera better than openGL camera?
bool blinn = false;
float counter = 0;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = true;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}
//              : camera() {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n'
        << backpackPosition.x << '\n'
        << backpackPosition.y << '\n'
        << backpackPosition.z << '\n'
        << backpackScale << '\n'
        << camera.Yaw << '\n'
        << camera.Pitch << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z
           >> backpackPosition.x
           >> backpackPosition.y
           >> backpackPosition.z
           >> backpackScale
           >> camera.Yaw
           >> camera.Pitch;
    }
}

ProgramState *programState;


void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Space", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glm::vec3 planetPosition = glm::vec3(30.0f);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC1_ALPHA, GL_ONE_MINUS_SRC_COLOR); try later.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shaders: load and configure
    // -------------------------
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
//    Shader shader("resources/shaders/cubemap.vs", "resources/shaders/cubemap.fs");
    Shader halconShader("resources/shaders/halcon.vs", "resources/shaders/halcon.fs");
    Shader planetShader("resources/shaders/planetLight.vs", "resources/shaders/planetLight.fs");

    float skyboxVertices[] = {
            // aPos
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // configuring Skybox

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // load textures.
    unsigned int planetTex = loadTexture("resources/objects/planet/texture planete 01.jpg");
    unsigned int mTex = loadTexture("resources/objects/Moon/Moon.jpg");

    // order for skybox: x+, x-, y+, y-, z+, z-
    // 1 3 6 5 2 4 -> works for skybox2 :) ; also for skybox1
    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    vector<std::string> faces{
            FileSystem::getPath("resources/textures/skybox1/1.png"),
            FileSystem::getPath("resources/textures/skybox1/3.png"),
            FileSystem::getPath("resources/textures/skybox1/6.png"),
            FileSystem::getPath("resources/textures/skybox1/5.png"),
            FileSystem::getPath("resources/textures/skybox1/2.png"),
            FileSystem::getPath("resources/textures/skybox1/4.png")
    };
    stbi_set_flip_vertically_on_load(true);

    unsigned int cubemapTexture = loadCubemap(faces);

    // configure shaders

    skyboxShader.use();
    skyboxShader.setInt("skyboxTex", 0);

    planetShader.use();
    planetShader.setInt("tex", 1);

    // load and configure models.
    // -----------
    Model deathStar2("resources/objects/planet/planet.obj");
    deathStar2.SetShaderTextureNamePrefix("material.");
    Model shipHalcon("resources/objects/halcon/Halcon_Milenario.obj");
    shipHalcon.SetShaderTextureNamePrefix("material.");
    Model deathStar("resources/objects/Moon/Moon.obj");
    deathStar.SetShaderTextureNamePrefix("material.");

    // TODO : fix later.


    PointLight& planetLight = programState->pointLight; // for planet
    planetLight.position = glm::vec3(30.0f, 30.0f, 30.0f);
    planetLight.ambient = glm::vec3(0.42f);
    planetLight.diffuse = glm::vec3(0.39f);
    planetLight.specular = glm::vec3(2.7f);

    planetLight.constant = 1.0f;
    planetLight.linear = 0.07f;
    planetLight.quadratic = 0.032f;


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms

//        pointLight.ambient += glm::vec3(counter  * 0.002f);
//        pointLight.diffuse += glm::vec3(counter  * 0.002f);
//        pointLight.specular += glm::vec3(counter  * 0.002f);

        glm::vec3 halconPosition = glm::vec3(planetPosition.x + sin(-currentFrame/2)*40.0f, 26.5f, planetPosition.z + cos(-currentFrame/2) * 40.0f);

        glDepthFunc(GL_LESS);

        // render the ship.
        halconShader.use();
        halconShader.setVec3("pointLight.position", glm::vec3(halconPosition.x, 32.0f, halconPosition.z));
//        halconShader.setVec3("pointLight.position", glm::vec3(10.0f * cos(currentFrame), 7.0f, 10.0f * sin(currentFrame)));
        halconShader.setVec3("pointLight.ambient", glm::vec3(0.44f, 0.44f, 0.44f) + glm::vec3(counter * 0.05f));
        halconShader.setVec3("pointLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f) + glm::vec3(counter * 0.05f));
        halconShader.setVec3("pointLight.specular", glm::vec3(1.6f, 1.6f, 1.6f) + glm::vec3(counter * 0.05f));
        halconShader.setFloat("pointLight.constant", 1.0f);
        halconShader.setFloat("pointLight.linear", 0.07f);
        halconShader.setFloat("pointLight.quadratic", 0.032f);
        halconShader.setVec3("viewPosition", programState->camera.Position);
        halconShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 400.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();

        // make ship go round.
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, halconPosition);
        model = glm::rotate(model, currentFrame / 4, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.015f));

        halconShader.use();
        halconShader.setMat4("projection", projection);
        halconShader.setMat4("view", view);
        halconShader.setMat4("model", model);

//        halconShader.setVec3("dirLight.direction", halconPosition);
//        halconShader.setVec3("dirLight.direction", programState->camera.Position);
//        halconShader.setVec3("dirLight.direction", glm::vec3(planetPosition.x + cos(currentFrame), planetPosition.y, planetPosition.z + sin(currentFrame)));
        halconShader.setVec3("dirLight.ambient", glm::vec3(0.57f));
        halconShader.setVec3("dirLight.diffuse", glm::vec3(0.75f));
        halconShader.setVec3("dirLight.specular", glm::vec3(0.85f));
        halconShader.setBool("blinn", blinn);

        glEnable(GL_CULL_FACE);
        glDepthFunc(GL_LESS);
        glCullFace(GL_BACK);
        shipHalcon.Draw(halconShader);
        glDisable(GL_CULL_FACE);

        // render the deathstar.

        model = glm::mat4(1.0f);
        model = glm::translate(model, planetPosition);
        model = glm::rotate(model, currentFrame / 6, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(3.06f));

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, planetTex);
//        glBindTexture(GL_TEXTURE_2D, mTex);

        planetLight.diffuse += glm::vec3(counter * 0.009f);
        planetLight.ambient += glm::vec3(counter * 0.009f);
        planetLight.specular += glm::vec3(counter * 0.009f);

        glDepthFunc(GL_LESS);
        planetShader.use();
        planetShader.setVec3("dirLight.direction", glm::vec3(planetPosition.x + cos(currentFrame), planetPosition.y, planetPosition.z + sin(currentFrame)));
        planetShader.setVec3("dirLight.ambient", glm::vec3(0.42f) + glm::vec3(counter * 0.17f));
        planetShader.setVec3("dirLight.diffuse", glm::vec3(0.65f) + glm::vec3(counter * 0.17f));
        planetShader.setVec3("dirLight.specular", glm::vec3(0.85f));

        planetShader.setVec3("pointLight.position", glm::vec3(halconPosition.x, 32.0f, halconPosition.z));
        planetShader.setVec3("pointLight.ambient", planetLight.ambient);
        planetShader.setVec3("pointLight.diffuse", planetLight.diffuse);
        planetShader.setVec3("pointLight.specular", planetLight.specular);
        planetShader.setFloat("pointLight.constant", planetLight.constant);
        planetShader.setFloat("pointLight.linear", planetLight.linear);
        planetShader.setFloat("pointLight.quadratic", planetLight.quadratic);
        planetShader.setVec3("viewPos", programState->camera.Position);
//        planetShader.setVec3("lightPos", planetPosition);
        planetShader.setBool("blinn", blinn);
        planetShader.setMat4("projection", projection);
        planetShader.setMat4("view", view);
        planetShader.setMat4("model", model);
//        deathStar2.Draw(planetShader);
        deathStar.Draw(planetShader);
        // render another planet?


        //draw skybox as last
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        model = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); //depth function back to normal state.

//        if (programState->ImGuiEnabled)
            DrawImGui(programState);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    {
        ImGui::Begin("Demolishing the DeathStar.");
        ImGui::Text("Press X repeatedly to fire.");
        ImGui::Text("B to turn on/off Blinn-Phong.");
        ImGui::End();

    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if(key == GLFW_KEY_R && action == GLFW_PRESS){
        programState->camera.Position = glm::vec3(1.0f, 1.0f, 1.0f);

    }

    if(key == GLFW_KEY_B && action == GLFW_PRESS){
        blinn = !blinn;
        if(blinn){
            std::cerr << "Blinn" << "\n";
        }
        else{
            std::cerr << "Phong" << "\n";
        }
    }

    if(key == GLFW_KEY_X && action == GLFW_PRESS){
        if(counter < 5) {
            counter++;

        }
        else{
            glfwSetWindowShouldClose(window, true);
            std::cerr << "Task completed." << '\n';
        }

    }
}


unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
