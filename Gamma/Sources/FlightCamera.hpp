#pragma once
#include "Camera.hpp"
#include <glm/glm.hpp>

class FlightCamera : public CameraBase
{
public:
    FlightCamera(CameraType type, GLFWwindow *window);
    ~FlightCamera() = default;

    void handleMouseButton(int key, int action) override;
    void handleMouseCursor(double x, double y) override;
    void handleMouseScroll(double dx, double dy) override;
    void move(CameraMovement direction, float deltaT) override;

private:
    void updateViewMatrix();

    float rotationSpeed;
    float movementSpeed;
    glm::vec2 rotation; // no rotation around z
    glm::vec3 position;
};