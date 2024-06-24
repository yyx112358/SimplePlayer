//
//  GLRendererPreview.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLRendererPreview.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "VideoTransform.hpp"

using namespace sp;

GLRendererPreview::GLRendererPreview(std::shared_ptr<IGLContext> context):GLRendererBase(context) {
    UpdateShader({R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoords;

    uniform mat4 transform;
    out vec2 TexCoords;

    void main()
    {
        gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
        TexCoords = aTexCoords;
    }
    )"}, {R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoords;

    uniform sampler2D screenTexture;

    void main()
    {
        FragColor = texture(screenTexture, TexCoords);
    }
    )"});
}

bool GLRendererPreview::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;

    _program->Activate();
    _vertexArray.Activate();
    
    _needUpdate = false;
    return true;
}

bool GLRendererPreview::_InternalRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 绑定到屏幕
    glViewport(0, 0, GetPreviewTransform()._outSize.width, GetPreviewTransform()._outSize.height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    SPASSERT(_textures.size() > 0);
    if (_textures.size() == 0)
        return false;
    
    _program->Activate();
    _vertexArray.Activate();
    glActiveTexture(GL_TEXTURE0 + 0); // 激活纹理单元1
    _textures[0]->Activate(); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
    _program->UpdateUniform("screenTexture", 0); // 更新纹理uniform
    
    GetPreviewTransform()._inSize.width = _textures[0]->width();
    GetPreviewTransform()._inSize.height = _textures[0]->height();
    _program->UpdateUniform("transform", GetPreviewTransform().toMatrix()); // 更新转换矩阵
    _program->FlushUniform();
    
    _vertexArray.Render();
    
    if (GLCheckError())
        return false;
    
    return true;
}
