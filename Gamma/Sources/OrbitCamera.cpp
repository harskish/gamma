#include "OrbitCamera.hpp"
#include <iostream>
#include <GLFW/glfw3.h>

OrbitCamera::OrbitCamera(CameraType type, GLFWwindow * window) : CameraBase(type, window) {
    this->speed = 0.5f;
    this->distance = 3.0f;
    this->angles = glm::vec2(0.0f);
    this->center = glm::vec3(0.0f);
    updateViewMatrix();
}

void OrbitCamera::handleMouseButton(int key, int action) {
    switch (key) {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_PRESS) {
            glfwGetCursorPos(mWindow, &lastX, &lastY);
            mouseLeftDown = true;
        }
        if (action == GLFW_RELEASE) {
            mouseLeftDown = false;
        }
        break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
        if (action == GLFW_PRESS) mouseMiddleDown = true;
        if (action == GLFW_RELEASE) mouseMiddleDown = false;
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS) mouseRightDown = true;
        if (action == GLFW_RELEASE) mouseRightDown = false;
        break;
    }
}

void OrbitCamera::handleMouseCursor(double x, double y) {
    if (!mouseLeftDown)
        return;
    
    glm::vec2 delta(x - lastX, y - lastY);
    angles += speed * delta;
    
    if (angles.x > 360.0f) angles.x -= 360.0f;
    if (angles.x < 0.0f) angles.x += 360.0f;
    angles.y = glm::clamp(angles.y, -90.0f, 90.0f);

    lastX = x;
    lastY = y;

    updateViewMatrix();
}

void OrbitCamera::handleMouseScroll(double dx, double dy) {
    distance = glm::max(zNear, (float)(distance - 0.2f * dy));
    updateViewMatrix();
}

void OrbitCamera::updateViewMatrix() {
    float lon = glm::radians(angles.x);
    float lat = glm::radians(angles.y);

    glm::mat4 V = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -distance));
    V = glm::rotate(V, lat, glm::vec3(1.0f, 0.0f, 0.0f));
    V = glm::rotate(V, lon, glm::vec3(0.0f, 1.0f, 0.0f));
    this->V = glm::translate(V, -center);
}


