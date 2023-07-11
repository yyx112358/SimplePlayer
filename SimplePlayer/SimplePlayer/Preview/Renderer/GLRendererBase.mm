//
//  GLRendererBase.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#include "GLRendererBase.hpp"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

using namespace sp;

bool GLRendererBase::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;
    
    _program->Activate();
    
    // 创建一个FBO使用的空纹理
    _frameBuffer->UpdateAttachTextures({_outputTexture});
    _frameBuffer->Activate();
    
    // 创建纹理
    for (auto &texture : _textures) {
        if (texture->Activate() == false)
            return false;
    }
    
    if (_outputTexture->Activate() == false)
        return false;
    
    //        // 这一行的作用是解除vertexBufId的激活状态，避免其它操作不小心改动到这里。不过这种情况很少见。
    //        glBindVertexArray(0);
    //        if (GLCheckError())
    //            return false;
    
    _needUpdate = false;
    return true;
}

bool GLRendererBase::_InternalRender()
{
    // 上屏绘制
    _frameBuffer->Activate();
    glViewport(0, 0, _outputTexture->width(), _outputTexture->height());
    glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
    _program->Activate(); // 启用Shader程序
    GLCheckError();
    
    _vertexArray.Activate();
    for (int i = 0; i < _textures.size(); i++) {
        assert(_textures[i]->id().has_value());
        
        glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元1
        glBindTexture(GL_TEXTURE_2D, *_textures[i]->id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        
        char textureName[] = "texture00";
        snprintf(textureName, sizeof(textureName), "texture%d", i);
        _program->UpdateUniform(textureName, i); // 更新纹理uniform
    }
    
    GLCheckError();
    
    _program->UpdateUniform("transform", _transform);
    
    _program->FlushUniform();
    
    _vertexArray.Render();
    GLCheckError();
    
    
    if (GLCheckError())
        return false;
    
    return true;
}
