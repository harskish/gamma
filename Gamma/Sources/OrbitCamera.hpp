#pragma once
#include "Camera.hpp"
#include <glm/glm.hpp>

class OrbitCamera : public CameraBase
{
public:
    OrbitCamera(CameraType type, GLFWwindow *window);
    ~OrbitCamera() = default;

    void handleMouseButton(int key, int action) override;
    void handleMouseCursor(double x, double y) override;
    void handleMouseScroll(double dx, double dy) override;
    void move(CameraMovement direction, float deltaT) override {};

private:
    void updateViewMatrix();

    float speed;
    float distance;
    glm::vec2 angles; // longitude, latitude
    glm::vec3 center;
};