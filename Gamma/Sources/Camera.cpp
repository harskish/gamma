#include "Camera.hpp"
#include <glad/glad.h>

CameraBase::CameraBase(CameraType type, GLFWwindow *window) {
    this->type = type;
    this->mWindow = window;
    this->fovDeg = 65.0f;
    this->zNear = 0.01f;
    this->zFar = 20.0f;
    this->V = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f)); // TODO: store inverse?
    viewportUpdate();
}

glm::vec3 CameraBase::getPosition() {
    glm::mat4 viewModel = glm::inverse(V);
    glm::vec4 posHom = viewModel[3];
    return glm::vec3(posHom / posHom.w);
}

void CameraBase::viewportUpdate() {
    int w, h;
    glfwGetFramebufferSize(mWindow, &w, &h);
    glViewport(0, 0, w, h);
    
    float ar = (float)w / (float)h;
    if (type == CameraType::ORTHO) {
        float scale = 3.0f; // scaled to look about the same as persp. w/ 65 fov
        P = glm::ortho(-scale * ar, scale * ar, -scale, scale, zNear, zFar);
    }
    else {
        P = glm::perspective(glm::radians(fovDeg), ar, zNear, zFar);
    }
}

void CameraBase::setType(CameraType t) {
    type = t;
    viewportUpdate();
}

void CameraBase::toggleType() {
    type = (type == CameraType::PERSP) ? CameraType::ORTHO : CameraType::PERSP;
    viewportUpdate();
}
