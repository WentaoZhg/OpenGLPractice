#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cyGL.h>
#include <ctype.h>

#include <cyTriMesh.h>
#include <cyMatrix.h>

#include <iostream>
#include <lodepng.h>
#include <vector>

// Window size.
int window_width = 700;
int window_height = 700;

// Object camera parameters.
float g_angleX = 0.0f;
float g_angleY = 0.0f;
float g_cameraDist = 5.0f;

// Plane camera parameters (used when ALT/Option is pressed).
float plane_angleX = 0.0f;
float plane_angleY = 0.0f;
float plane_cameraDist = 5.0f;

// Toggle which set of camera parameters to update.
bool usePlaneControls = true;

// Mouse state.
bool g_leftMouseDown = false;
bool g_rightMouseDown = false;
double g_lastMouseX = 0.0;
double g_lastMouseY = 0.0;

// Transformation matrices for the object.
cy::Matrix4f modelMatrix = {
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1
};
cy::Matrix4f viewMatrix;
cy::Matrix4f projMatrix;
cy::Matrix4f mvpMatrix;

// Transformation matrices for the plane.
cy::Matrix4f planeViewMatrix;

// Global VAO and VBO handles for the object.
GLuint vao;
GLuint vertexBuffer;
GLuint normalBuffer;
GLuint texCoordBuffer;

// Our mesh.
cy::TriMesh trimesh_obj;

// Expanded arrays (one entry per vertex for each triangle)
std::vector<cy::Vec3f> expandedPositions;
std::vector<cy::Vec3f> expandedNormals;
std::vector<cy::Vec2f> expandedTexCoords;

// Shaders for the object.
cy::GLSLShader glsl_shader;
cy::GLSLProgram glsl_prog;
GLuint vs_id;
GLuint fs_id;

// Diffuse texture.
GLuint diffuseTexID = 0;

// Render texture for off-screen rendering
cy::GLRenderTexture<GL_TEXTURE_2D> renderTexture;

// Shader for rendering the plane
cy::GLSLProgram planeShader;

// Plane geometry
GLuint planeVAO, planeVBO, planeEBO;

void expandMeshData();
void render();
void initialize();
void initializePlane();
void getShaders();
void getPlaneShaders();
void matrixSetup();
void updateObjectView();
void updatePlaneView();
GLuint loadTexture(const char* filename);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);

int main(int argc, char* argv[])
{
    // Require the OBJ filename as its first command-line argument.
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " model.obj" << std::endl;
        return -1;
    }

    // Load the OBJ file (with textures/MTL enabled).
    if (!trimesh_obj.LoadFromFileObj(argv[1], true))
    {
        std::cerr << "Failed to load OBJ file: " << argv[1] << std::endl;
        return -1;
    }

    // Initialize GLFW.
    if (!glfwInit())
        return -1;
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Project 2", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW.
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "GLEW initialization failed" << std::endl;
        return -1;
    }

    // Load the diffuse texture (if specified in the MTL file).
    std::string diffuseTexFilename;
    if (trimesh_obj.NM() > 0)
    {
        diffuseTexFilename = trimesh_obj.M(0).map_Kd;
    }
    std::string fullTexturePath = "Objects/" + diffuseTexFilename;
    if (!diffuseTexFilename.empty())
    {
        std::cout << "Loading diffuse texture: " << fullTexturePath << std::endl;
        diffuseTexID = loadTexture(fullTexturePath.c_str());
        if (diffuseTexID == 0)
        {
            std::cerr << "Failed to load diffuse texture." << std::endl;
            return -1;
        }
    }
    else
    {
        std::cerr << "No diffuse texture specified in the MTL file." << std::endl;
    }

    // Set up object and plane data.
    initialize();
    getShaders();
    matrixSetup();

    if (!renderTexture.Initialize(true, 4, window_width, window_height))
    {
        std::cerr << "Failed to initialize render texture." << std::endl;
        return -1;
    }

    // Initialize plane geometry and its shader.
    initializePlane();
    getPlaneShaders();

    glClearColor(0.0, 0.0, 0.0, 1.0); // black background

    // Set up mouse callbacks.
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);

    // Main loop.
    while (!glfwWindowShouldClose(window))
    {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}


// Expand the mesh data: for each face, use the face indices to push back the corresponding data.
void expandMeshData()
{
    expandedPositions.clear();
    expandedNormals.clear();
    expandedTexCoords.clear();

    for (unsigned int i = 0; i < trimesh_obj.NF(); i++)
    {
        auto face = trimesh_obj.F(i);     // indices for positions
        auto normalFace = trimesh_obj.FN(i); // indices for normals
        auto texFace = trimesh_obj.FT(i);    // indices for texture coordinates

        for (int j = 0; j < 3; j++)
        {
            expandedPositions.push_back(trimesh_obj.V(face.v[j]));
            expandedNormals.push_back(trimesh_obj.VN(normalFace.v[j]));
            cy::Vec3f vt = trimesh_obj.VT(texFace.v[j]);
            expandedTexCoords.push_back(cy::Vec2f(vt.x, vt.y));
        }
    }
}

// Load and compile the object shaders.
void getShaders()
{
    glsl_prog.BuildFiles("Shaders/vertShader.vs", "Shaders/fragShader.fs");
    vs_id = glsl_prog.GetID();
    fs_id = glsl_prog.GetID();
}

// Load and compile the plane shaders.
void getPlaneShaders()
{
    planeShader.BuildFiles("Shaders/planeShader.vs", "Shaders/planeShader.fs");
}


// Update the view matrix for the object.
void updateObjectView()
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

// Update the view matrix for the plane (when ALT is pressed).
void updatePlaneView()
{
    float radY = plane_angleY;
    float radX = plane_angleX;
    float r = plane_cameraDist;

    cy::Vec3f eye;
    eye.x = r * sin(radY) * cos(radX);
    eye.y = r * sin(radX);
    eye.z = r * cos(radY) * cos(radX);

    cy::Vec3f center(0.0f, 0.0f, 0.0f);
    cy::Vec3f up(0.0f, 1.0f, 0.0f);
    planeViewMatrix.SetView(eye, center, up);
}


// Set up the projection and initial view matrices for the object.
void matrixSetup()
{
    cy::Vec3f eye(0.0f, 0.0f, 5.0f);
    cy::Vec3f center(0.0f, 0.0f, 0.0f);
    cy::Vec3f up(0.0f, 1.0f, 0.0f);
    viewMatrix.SetView(eye, center, up);

    float aspect = (float)window_width / (float)window_height;
    projMatrix.SetPerspective((3.14159f) / 3.0f, aspect, 0.1f, 100.0f);
}

// Initialize the object’s VAO/VBOs.
void initialize() {
    glEnable(GL_DEPTH_TEST);

    // Expand mesh data.
    expandMeshData();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex Buffer.
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, expandedPositions.size() * sizeof(cy::Vec3f),
        expandedPositions.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // positions at location 0.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(cy::Vec3f), (void*)0);

    // Normal Buffer.
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, expandedNormals.size() * sizeof(cy::Vec3f),
        expandedNormals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); // normals at location 1.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(cy::Vec3f), (void*)0);

    // Texture Coordinate Buffer.
    glGenBuffers(1, &texCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, expandedTexCoords.size() * sizeof(cy::Vec2f),
        expandedTexCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2); // texture coords at location 2.
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(cy::Vec2f), (void*)0);

    glBindVertexArray(0);
}

// Initialize the plane geometry.
void initializePlane()
{
    // Define a full-screen square (two triangles) with position and texture coordinates.
    float planeVertices[] = {
        // positions         // tex coords
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f
    };
    unsigned int planeIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &planeVAO);
    glBindVertexArray(planeVAO);

    glGenBuffers(1, &planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &planeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    // Position attribute.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    // Texture coordinate attribute.
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}


// Render function.
void render() {
    renderTexture.Bind();
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update object view matrix.
    updateObjectView();

    // (Optional) Update plane view if using plane controls.
    if (usePlaneControls)
    {
        updatePlaneView();
    }
    // Set up the plane's perspective projection.
    cy::Matrix4f planeProj;
    planeProj.SetPerspective((3.14159f) / 3.0f, 1.0f, 0.1f, 100.0f);
    cy::Matrix4f planeMVP = planeProj * planeViewMatrix; // plane model is identity.
    GLfloat planeMVPMat[16];
    planeMVP.Get(planeMVPMat);
    GLuint planeMVPID = glGetUniformLocation(planeShader.GetID(), "mvp");
    glUniformMatrix4fv(planeMVPID, 1, GL_FALSE, planeMVPMat);

    // Bind the teapot shader.
    glsl_prog.Bind();

    // Compute the combined model-view matrix.
    cy::Matrix4f modelView = viewMatrix * modelMatrix;
    // Compute the MVP: projection * (view * model)
    cy::Matrix4f mvp = projMatrix * modelView;

    GLfloat mvpMat[16];
    mvp.Get(mvpMat);
    GLuint mvpID = glGetUniformLocation(glsl_prog.GetID(), "mvp");
    glUniformMatrix4fv(mvpID, 1, GL_FALSE, mvpMat);

    GLfloat viewMat[16];
    modelView.Get(viewMat);
    GLuint viewID = glGetUniformLocation(glsl_prog.GetID(), "view");
    glUniformMatrix4fv(viewID, 1, GL_FALSE, viewMat);

    // Bind the diffuse texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseTexID);
    GLuint texUniform = glGetUniformLocation(glsl_prog.GetID(), "diffuseTexture");
    glUniform1i(texUniform, 0);

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, expandedPositions.size());
    glBindVertexArray(0);

    renderTexture.Unbind();

    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    planeShader.Bind();

    // Pass a small color offset to the plane shader.
    GLuint offsetID = glGetUniformLocation(planeShader.GetID(), "colorOffset");
    glUniform3f(offsetID, 0.1f, 0.1f, 0.1f);

    // Bind the render texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTexture.GetTextureID());
    GLuint planeTexUniform = glGetUniformLocation(planeShader.GetID(), "renderedTexture");
    glUniform1i(planeTexUniform, 0);

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}


// Mouse button callback. Determines which camera parameters to update based on whether ALT is pressed.
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Check if ALT/Option is pressed.
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
    {
        usePlaneControls = true;
    }
    else
    {
        usePlaneControls = false;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_leftMouseDown = true;
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            g_leftMouseDown = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            g_rightMouseDown = true;
            glfwGetCursorPos(window, &g_lastMouseX, &g_lastMouseY);
        }
        else if (action == GLFW_RELEASE) {
            g_rightMouseDown = false;
        }
    }
}


// Mouse movement callback. Updates either the object or plane camera parameters.
void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (!g_leftMouseDown && !g_rightMouseDown)
        return;

    double dx = xpos - g_lastMouseX;
    double dy = ypos - g_lastMouseY;

    g_lastMouseX = xpos;
    g_lastMouseY = ypos;

    const float rotateSpeed = 0.01f;
    const float zoomSpeed = 0.01f;

    if (g_leftMouseDown) {
        if (usePlaneControls) {
            plane_angleY += float(dx * rotateSpeed);
            plane_angleX += float(dy * rotateSpeed);
        }
        else {
            g_angleY += float(dx * rotateSpeed);
            g_angleX += float(dy * rotateSpeed);
        }
    }
    if (g_rightMouseDown) {
        if (usePlaneControls) {
            plane_cameraDist += float(dy * zoomSpeed);
            if (plane_cameraDist < 0.1f)
                plane_cameraDist = 0.1f;
        }
        else {
            g_cameraDist += float(dy * zoomSpeed);
            if (g_cameraDist < 0.1f)
                g_cameraDist = 0.1f;
        }
    }
}


// Loads a PNG image using LodePNG and creates an OpenGL texture.
GLuint loadTexture(const char* filename)
{
    std::vector<unsigned char> image;
    unsigned width, height;
    unsigned error = lodepng::decode(image, width, height, filename);
    if (error)
    {
        std::cerr << "Error decoding PNG " << filename << ": "
            << lodepng_error_text(error) << std::endl;
        return 0;
    }

    std::cout << "Loaded texture " << filename << " (" << width << "x" << height << ")" << std::endl;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, image.data());

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}