#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cyGL.h>
#include <ctype.h>

#include <cyTriMesh.h>
#include <cyMatrix.h>

#include <iostream>



int window_width = 700;
int window_height = 700;

float g_angleX = 0.0f;     // rotation around X-axis
float g_angleY = 0.0f;     // rotation around Y-axis
float g_cameraDist = 5.0f; // how far the camera is from the origin

bool g_leftMouseDown = false;
bool g_rightMouseDown = false;

double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;


cy::Matrix4f modelMatrix = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
};

cy::Matrix4f viewMatrix;
cy::Matrix4f projMatrix;
cy::Matrix4f mvpMatrix;


GLuint vao;
GLuint vertexBuffer;
GLuint normalBuffer;

cy::TriMesh trimesh_obj;

cy::GLSLShader glsl_shader;
cy::GLSLProgram glsl_prog;
GLuint vs_id;
GLuint fs_id;

void render();
void initialize();
void getShaders();
void matrixSetup();
void matrixTransform();

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);


int main(void)
{

    trimesh_obj.LoadFromFileObj("Objects/teapot.obj", true); // read teapot

    GLFWwindow* window;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(window_width, window_height, "Project 2", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    // init glew
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }

    initialize();
    // get shaders
    getShaders();

    glClearColor(0.0, 0.0, 0.0, 1.0); //black background

    //setup matrixes
    matrixSetup();

    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {

        /* Render here */
        render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

    }

    glfwTerminate();
    return 0;
}

void getShaders()
{
    glsl_prog.BuildFiles("Shaders/vertShader.vs", "Shaders/fragShader.fs");
    vs_id = glsl_shader.GetID();
    fs_id = glsl_shader.GetID();
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glsl_prog.Bind();

    // Compute the Model-View-Projection matrix
    mvpMatrix = projMatrix * viewMatrix * modelMatrix;
    GLfloat mvpMat[16];
    mvpMatrix.Get(mvpMat);
    GLuint mvpID = glGetUniformLocation(glsl_prog.GetID(), "mvp");
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, mvpMat);

    // Also send the view matrix for normal transformation
    GLfloat viewMat[16];
    viewMatrix.Get(viewMat);
    GLuint viewID = glGetUniformLocation(glsl_prog.GetID(), "view");
    glUniformMatrix4fv(viewID, 1, GL_FALSE, viewMat);

    // Bind VAO (it now contains both vertex and normal buffers and the element buffer)
    glBindVertexArray(vao);

    // Draw the triangles using the index buffer.
    glDrawElements(GL_TRIANGLES, trimesh_obj.NF() * 3, GL_UNSIGNED_INT, 0);
}


void initialize() {
    glEnable(GL_DEPTH_TEST);

    // Generate a vertex array and bind it
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // --- Vertex Buffer ---
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * trimesh_obj.NV(),
        &trimesh_obj.V(0), GL_STATIC_DRAW);
    // Set up vertex attribute pointer at location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // --- Normal Buffer ---
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cy::Vec3f) * trimesh_obj.NV(),
        &trimesh_obj.VN(0), GL_STATIC_DRAW);
    // Set up normal attribute pointer at location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // --- Element (Index) Buffer ---
    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        sizeof(unsigned int) * trimesh_obj.NF() * 3,
        &trimesh_obj.F(0), GL_STATIC_DRAW);

    // Unbind
    glBindVertexArray(0);
}


void matrixSetup()
{
    // VIEW MATRIX
    cy::Vec3f eye(0.0f, 0.0f, 5.0f);      // camera position
    cy::Vec3f center(0.0f, 0.0f, 0.0f);   // look-at target
    cy::Vec3f up(0.0f, 1.0f, 0.0f);       // "up" direction
    viewMatrix.SetView(eye, center, up);

    // PROJECTION MATRIX
    float aspect = (float)window_width / (float)window_height;
    projMatrix.SetPerspective((3.14159f) / 3.0f, aspect, 0.1f, 100.0f);
}


// Mouse button callback
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_leftMouseDown = true;
            // Store where the mouse was when the button was pressed
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            g_leftMouseDown = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_rightMouseDown = true;
            // Store where the mouse was when the button was pressed
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            g_rightMouseDown = false;
        }
    }
}

// Mouse movement callback
void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    // If no mouse button is held, don’t do anything
    if (!g_leftMouseDown && !g_rightMouseDown) {
        return;
    }

    // Calculate the difference
    double dx = xpos - g_lastMouseX;
    double dy = ypos - g_lastMouseY;

    // Update the last positions
    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    // If left mouse is down, adjust camera angles
    if (g_leftMouseDown) {
        // Some scaling factor to slow down rotation
        float rotateSpeed = 0.01f;

        // yaw around the Y-axis
        g_angleY += float(dx * rotateSpeed);
        // pitch around the X-axis
        g_angleX += float(dy * rotateSpeed);

        matrixTransform();
    }

    // If right mouse is down, adjust camera distance
    if (g_rightMouseDown) {
        float zoomSpeed = 0.01f;
        g_cameraDist += float(dy * zoomSpeed);
        if (g_cameraDist < 0.1f) g_cameraDist = 0.1f; // avoid going through origin

        matrixTransform();
    }
}

void matrixTransform()
{
    float radY = g_angleY;
    float radX = g_angleX;
    float r = g_cameraDist;

    cy::Vec3f eye;
    eye.x = r * sin(radY) * cos(radX);
    eye.y = r * sin(radX);
    eye.z = r * cos(radY) * cos(radX);

    cy::Vec3f center(0.0f, 0.0f, 0.0f);
    cy::Vec3f up(0.0f, 1.0f, 0.0f);

    viewMatrix.SetView(eye, center, up);
}