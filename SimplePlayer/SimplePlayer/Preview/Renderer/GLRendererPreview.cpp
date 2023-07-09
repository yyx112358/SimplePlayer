//
//  GLRendererPreview.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLRendererPreview.hpp"

#include "../../../../thirdParty/glm/glm/glm.hpp"
#include "../../../../thirdParty/glm/glm/gtc/matrix_transform.hpp"

using namespace sp;


void GLRendererPreview::_UpdateTransform()
{
    if (_textures.size() == 0 || _fillmode == EFillMode::Free)
        return;
    
    GLfloat texWidth = _textures[0]->width(), texHeight = _textures[0]->height();
    if (_rotation == ERotation::Rotation90 || _rotation == ERotation::Rotation270)
        std::swap(texWidth, texHeight);
    GLfloat scaleX = 1, scaleY = 1;
    switch(_fillmode) {
        case EFillMode::Fit:
            if (GLfloat scale = _previewWidth / texWidth; texHeight * scale <= _previewHeight) {
                scaleX = scale;
                scaleY = scale;
            } else {
                scaleX = _previewHeight / texHeight;
                scaleY = _previewHeight / texHeight;
            }
            break;
            
        case EFillMode::Stretch:
            scaleX = _previewWidth / texWidth;
            scaleY = _previewHeight / texHeight;
            break;
            
        case EFillMode::Fill:
            if (GLfloat scale = _previewWidth / texWidth; texHeight * scale >= _previewHeight) {
                scaleX = scale;
                scaleY = scale;
            } else {
                scaleX = _previewHeight / texHeight;
                scaleY = _previewHeight / texHeight;
            }
            break;
            
        case EFillMode::Origin:
            scaleX = 1;
            scaleY = 1;
            break;
            
        case EFillMode::Free:
            return;
    }
    if (_flipX)
        scaleX *= -1;
    if (_flipY)
        scaleY *= -1;
    
    _transform = glm::identity<glm::mat4>();
    _transform = glm::scale(_transform, glm::vec3(scaleX, scaleY, 1.0f));   // 缩放到preview区域内
    _transform = glm::scale(_transform, glm::vec3(texWidth / _previewWidth, texHeight / _previewHeight, 1.0f)); // 归一化
    _transform = glm::rotate(_transform, glm::pi<GLfloat>() / 2 * (int)_rotation, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
}

bool GLRendererPreview::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;
    
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
    _program->Activate();
    _vertexArray.Activate();
    
    _needUpdate = false;
    return true;
}

bool GLRendererPreview::_InternalRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // 绑定到屏幕
    glViewport(0, 0, _previewWidth, _previewHeight);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    assert(_textures.size() > 0);
    if (_textures.size() == 0)
        return false;
    
    _program->Activate();
    _vertexArray.Activate();
    glActiveTexture(GL_TEXTURE0 + 0); // 激活纹理单元1
    glBindTexture(GL_TEXTURE_2D, *_textures[0]->id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
    _program->UpdateUniform("screenTexture", 0); // 更新纹理uniform
    
    _UpdateTransform();
    _program->UpdateUniform("transform", _transform); // 更新转换矩阵
    _program->FlushUniform();
    
    _vertexArray.Render();
    
    if (GLCheckError())
        return false;
    
    return true;
}
