#include "GammaRenderer.hpp"
#include <glad/glad.h>

void GammaRenderer::render(void) {
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
