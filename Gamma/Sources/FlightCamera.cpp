#include "FlightCamera.hpp"
#include <iostream>
#include <GLFW/glfw3.h>

FlightCamera::FlightCamera(CameraType type, GLFWwindow * window) : CameraBase(type, window) {
    this->rotationSpeed = 0.5f;
    this->movementSpeed = 1.0f;
    this->position = glm::vec3(0.0, 0.0, 3.0);
    this->rotation = glm::vec2(0.0);
    updateViewMatrix();
}

void FlightCamera::handleMouseButton(int key, int action) {
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

void FlightCamera::handleMouseCursor(double x, double y) {
    if (!mouseLeftDown)
        return;

    glm::vec2 delta(x - lastX, y - lastY);
    rotation += rotationSpeed * delta;

    if (rotation.x < 0) rotation.x += 360.0f;
    if (rotation.y < 0) rotation.y += 360.0f;
    if (rotation.x > 360.0f) rotation.x -= 360.0f;
    if (rotation.y > 360.0f) rotation.y -= 360.0f;

    lastX = x;
    lastY = y;

    updateViewMatrix();
}

void FlightCamera::handleMouseScroll(double dx, double dy) {
    (void)dx;
    float newSpeed = (dy > 0) ? movementSpeed * 1.2f : movementSpeed / 1.2f;
    movementSpeed = glm::max(1e-3f, glm::min(1e6f, newSpeed));
}

void FlightCamera::move(CameraMovement direction, float deltaT) {
    glm::mat4 invV = glm::inverse(V);
    glm::vec3 right(invV[0]);
    glm::vec3 up(invV[1]);
    glm::vec3 forward(-invV[2]);

    switch (direction) {
    case CameraMovement::FORWARD:
        position += deltaT * movementSpeed * forward; break;
    case CameraMovement::BACKWARD:
        position -= deltaT * movementSpeed * forward; break;
    case CameraMovement::RIGHT:
        position += deltaT * movementSpeed * right; break;
    case CameraMovement::LEFT:
        position -= deltaT * movementSpeed * right; break;
    case CameraMovement::UP:
        position += deltaT * movementSpeed * up; break;
    case CameraMovement::DOWN:
        position -= deltaT * movementSpeed * up; break;
    }

    updateViewMatrix();
}

void FlightCamera::updateViewMatrix() {
    V = glm::mat4(1.0f);
    V = glm::rotate(V, glm::radians(rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
    V = glm::rotate(V, glm::radians(rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
    V = glm::translate(V, -position);
}
