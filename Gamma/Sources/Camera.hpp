#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType {
    ORTHO,
    PERSP
};

class CameraBase
{
public:
    CameraBase(CameraType type, GLFWwindow *window);
    virtual ~CameraBase() = default;

    // View and projection matrices
    glm::mat4 getV() { return V; };
    glm::mat4 getP() { return P; };
    glm::mat4 getVP() { return P * V; };
    glm::vec3 getPosition();

    // Handle change in window size
    void viewportUpdate();
    
    // Camera movement scheme
    virtual void handleMouseButton(int key, int action) = 0;
    virtual void handleMouseCursor(double x, double y) = 0;
    virtual void handleMouseScroll(double dx, double dy) = 0;

    void setType(CameraType t);
    void toggleType();

protected:
    CameraType type;
    float fovDeg;
    float zNear;
    float zFar;
    glm::mat4 V;
    glm::mat4 P;

    // Mouse state
    double lastX, lastY;
    bool mouseLeftDown = false;
    bool mouseMiddleDown = false;
    bool mouseRightDown = false;

    // Needed for projection matrix, cursor position
    GLFWwindow *mWindow;
};