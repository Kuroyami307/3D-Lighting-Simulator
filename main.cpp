#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // For transformations like translate, rotate, scale
#include <glm/gtc/type_ptr.hpp>

#include "custom/shader.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h> 
#endif

bool camRotation = false;
glm::vec3 cameraPos(3.0f, 3.0f, 3.0f), targetPos(0.0f, 0.0f, 0.0f), cameraUp(0.0f, 1.0f, 0.0f);

float ambientLight = 0.1;

struct lightComponents {
    float ambientCoeff;
    float diffuseCoeff;
    float specularCoeff;
};

struct light{
    glm::vec3 position;
    glm::vec4 color;
    float intensity;
    // float attenuation;
};

class object{
    private:
    std::vector<float>vertices;
    // std::vector<float>flatVertices;
    std::vector<int>faces;
    // std::vector<float>vertexNormals;
    int flatVerticesSize;
    std::ifstream file;
    unsigned int VBO_position, VBO_normal, VAO, EBO;
    unsigned int VAO_Flat, VBO_FlatPosition, VBO_FlatShadingNormal;
    bool flatShading;

    //std::vector<float>vertexTestures;
    public:
    object()
    {
        flatShading = false;
    }

    void loadModel(const std::string &name)
    {
        std::vector<float> vertexNormals;
        std::string line;
        file.open(name.c_str());

        vertices.clear();
        vertexNormals.clear();

        if(file.is_open())
        {
            while(getline(file, line))
            {
                
                if(line[0] == 'v' && line[1] == ' ')
                {
                    float x, y, z;
                    sscanf(line.c_str(), "v %f %f %f", &x, &y, &z);
                    vertices.push_back(x);
                    vertices.push_back(y);
                    vertices.push_back(z);
                }

                if(line[0] == 'f' && line[1] == ' ')
                {
                    int x, y, z;
                    int a, b, c, p, q, r;
                    sscanf(line.c_str(), "f %d/%d/%d %d/%d/%d %d/%d/%d", &x, &a, &p, &y, &b, &q, &z, &c, &r);
                    faces.push_back(x-1);
                    faces.push_back(y-1);
                    faces.push_back(z-1);
                }
            }
        }

        vertexNormals.resize(vertices.size(), 0.0f);

        for(int i = 0, size = faces.size()/3; i < size; i++)
        {
            //retrieving face indices
            int f1, f2, f3;
            f1 = faces[3 * i + 0];
            f2 = faces[3 * i + 1];
            f3 = faces[3 * i + 2];

            //getting vertex data from the given indices
            glm::vec3 v1(vertices[3 * f1 + 0], vertices[3 * f1 + 1], vertices[3 * f1 + 2]);
            glm::vec3 v2(vertices[3 * f2 + 0], vertices[3 * f2 + 1], vertices[3 * f2 + 2]);
            glm::vec3 v3(vertices[3 * f3 + 0], vertices[3 * f3 + 1], vertices[3 * f3 + 2]);

            //calculate normal
            glm::vec3 normal = glm::normalize(glm::cross((v2 - v1), (v3 - v1)));

            
            vertexNormals[3 * f1 + 0] += normal.x;
            vertexNormals[3 * f1 + 1] += normal.y;
            vertexNormals[3 * f1 + 2] += normal.z;

            vertexNormals[3 * f2 + 0] += normal.x;
            vertexNormals[3 * f2 + 1] += normal.y;
            vertexNormals[3 * f2 + 2] += normal.z;

            vertexNormals[3 * f3 + 0] += normal.x;
            vertexNormals[3 * f3 + 1] += normal.y;
            vertexNormals[3 * f3 + 2] += normal.z;
            
        }
        for (int i = 0; i < vertexNormals.size(); i += 3) {
            glm::vec3 norm(vertexNormals[i], vertexNormals[i+1], vertexNormals[i+2]);
            norm = glm::normalize(norm);
            vertexNormals[i] = norm.x;
            vertexNormals[i+1] = norm.y;
            vertexNormals[i+2] = norm.z;
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0); 

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO_position);
        glGenBuffers(1, &VBO_normal);
        glGenBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_position);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal);
        glBufferData(GL_ARRAY_BUFFER, vertexNormals.size() * sizeof(float), vertexNormals.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces.size() * sizeof(int), faces.data(), GL_STATIC_DRAW);

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0); 

        // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        glBindVertexArray(0); 

        //set up data for flat shading
        // flatShading = true;
        std::vector<float> flatVertices;
        
        flatVertices.clear();
        int size = faces.size();
        for(int i = 0; i < size; i++)
        {
            flatVertices.push_back(vertices[3 * faces[i] + 0]);
            flatVertices.push_back(vertices[3 * faces[i] + 1]);
            flatVertices.push_back(vertices[3 * faces[i] + 2]);
        }
    
        vertexNormals.resize(flatVertices.size(), 0.0f);

        for (int i = 0; i < flatVertices.size(); i += 9)
        {
            glm::vec3 v1(flatVertices[i + 0], flatVertices[i + 1], flatVertices[i + 2]);
            glm::vec3 v2(flatVertices[i + 3], flatVertices[i + 4], flatVertices[i + 5]);
            glm::vec3 v3(flatVertices[i + 6], flatVertices[i + 7], flatVertices[i + 8]);

            glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));

            vertexNormals[i + 0] = normal.x;
            vertexNormals[i + 1] = normal.y;
            vertexNormals[i + 2] = normal.z;

            vertexNormals[i + 3] = normal.x;
            vertexNormals[i + 4] = normal.y;
            vertexNormals[i + 5] = normal.z;

            vertexNormals[i + 6] = normal.x;
            vertexNormals[i + 7] = normal.y;
            vertexNormals[i + 8] = normal.z;
        }

        flatVerticesSize = flatVertices.size();

        glGenVertexArrays(1, &VAO_Flat);
        glGenBuffers(1, &VBO_FlatPosition);
        glGenBuffers(1, &VBO_FlatShadingNormal);

        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO_Flat);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_FlatPosition);
        glBufferData(GL_ARRAY_BUFFER, flatVertices.size() * sizeof(float), flatVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, VBO_FlatShadingNormal);
        glBufferData(GL_ARRAY_BUFFER, vertexNormals.size() * sizeof(float), vertexNormals.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
    }

    void draw()
    {
        if(flatShading)
        {
            glBindVertexArray(VAO_Flat);
            glDrawArrays(GL_TRIANGLES, 0, flatVerticesSize/3);
        }
        else
        {
            glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            glDrawElements(GL_TRIANGLES, faces.size(), GL_UNSIGNED_INT, 0);
            // glBindVertexArray(0); // no need to unbind it every time 
        }
    }

    ~object()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO_position);
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &VBO_normal);

        glDeleteVertexArrays(1, &VAO_Flat);
        glDeleteBuffers(1, &VBO_FlatPosition);
        glDeleteBuffers(1, &VBO_FlatShadingNormal);
    }

    void setFlatShading(bool status)
    {
        flatShading = status;
    }

    bool getFlatShadingStatus()
    {
        return flatShading;
    }
};

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
unsigned int SCR_WIDTH = 700;
unsigned int SCR_HEIGHT = 700;

GLFWwindow* window = nullptr;
bool fPressed = false;

shader ourShader;
object sphere;

light l1 = {glm::vec3(2,2,-2), glm::vec4(1,1,1,1), 0.8};

glm::mat4 model         = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
glm::mat4 view          = glm::mat4(1.0f);
glm::mat4 projection    = glm::mat4(1.0f);

void mainLoop()
{
        // input
        // -----
        glfwPollEvents();

        glm::vec3 translate = glm::vec3(0.0f, 0.0f, 0.0f);

        processInput(window);

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            translate += glm::vec3(0.0f, 0.0f, 0.05f);

        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            translate += glm::vec3(0.0f, 0.0f, -0.05f);

        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            translate += glm::vec3(-0.05f, 0.0f, 0.0f);

        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            translate += glm::vec3(0.05f, 0.0f, 0.0f);

        if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        {
            fPressed = true;   
        }

        if(glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE && fPressed)
        {
            sphere.setFlatShading(!sphere.getFlatShadingStatus());    
            fPressed = false;
        }

        cameraPos += translate;
        targetPos +=translate;


        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw our first triangle
        ourShader.use();

        unsigned int modelLoc = glGetUniformLocation(ourShader.progID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        view  = glm::lookAt(cameraPos, targetPos, cameraUp);
        unsigned int viewLoc = glGetUniformLocation(ourShader.progID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        unsigned int projectionLoc = glGetUniformLocation(ourShader.progID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // float timeValue = glfwGetTime();
        // float greenValue = (sin(timeValue) / 2.0f) + 0.5f;

        unsigned int ourColorLoc = glGetUniformLocation(ourShader.progID, "ourColor");
        glUniform3f(ourColorLoc, 0.9f, 0.1f, 0.1f);

        unsigned int lightPosLoc = glGetUniformLocation(ourShader.progID, "lightPosition");
        glUniform3f(lightPosLoc, l1.position.x, l1.position.y, l1.position.z);

        unsigned int camPosLoc = glGetUniformLocation(ourShader.progID, "cameraPosition");
        glUniform3f(camPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);

        unsigned int lightLoc = glGetUniformLocation(ourShader.progID, "lightIntensity");
        glUniform1f(lightLoc, l1.intensity);

        sphere.draw();

        glm::mat4 rota = glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec4 temp(l1.position, 1.0f);
        temp = temp * rota;
        l1.position = glm::vec3(temp.x, temp.y, temp.z);
 
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    
    #ifdef __EMSCRIPTEN__
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    #else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    #ifdef __EMSCRIPTEN__
    if (!gladLoadGLLoader((GLADloadproc)emscripten_webgl_get_proc_address))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }
    #else
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }
    #endif

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    //make shaderObject

    #ifdef __EMSCRIPTEN__
        ourShader.loadShaders("Emiscripten/vShader.vs", "Emiscripten/fShader.fs");
    #else
        ourShader.loadShaders("Shaders/vs2.vs", "Shaders/fs2.fs");
    #endif
    //define model objects;

    // sphere.loadModel("horse.obj");
        #ifdef __EMSCRIPTEN__
        sphere.loadModel("Emiscripten/horse.obj");
    #else
        sphere.loadModel("horse.obj");
    #endif
    // model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    model  = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
    // view  = glm::lookAt(cameraPos, targetPos, cameraUp);
    projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    //camera stuffs
    //glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(mainLoop, 0, 1);
    #else
        while (!glfwWindowShouldClose(window))
        {
            mainLoop();
        }
    #endif

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteProgram(ourShader.progID);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    SCR_HEIGHT = height;
    SCR_WIDTH = width;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) 
{
    static double prevMouseX, prevMouseY;
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !camRotation)
    {
        std::cout << "Mouse button down" <<std::endl;
        glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
        camRotation = true;
    }
        

    if(camRotation)
    {
        double currMouseX = xpos, currMouseY=ypos;

        double dx = currMouseX - prevMouseX;
        double dy = currMouseY - prevMouseY;

        // std::cout << dx << " " << dy << "\n";

        glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians((float)dx), glm::vec3(0.0f, 1.0f, 0.0f));
        rot = glm::rotate(rot, glm::radians((float)dy), glm::vec3(1.0f, 0.0f, 0.0f));

        glm::vec4 temp(cameraPos, 1.0f);
        temp = rot * temp;
        cameraPos = glm::vec3(temp.x, temp.y, temp.z);

        prevMouseX = currMouseX;
        prevMouseY = currMouseY;
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        camRotation = false;
    }
}