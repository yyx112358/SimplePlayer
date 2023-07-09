//
//  GLRendererBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLRendererBase_hpp
#define GLRendererBase_hpp

#include "../../../../thirdParty/glm/glm/ext/matrix_float4x4.hpp"

#import "IGLContext.hpp"
#import "GLProgram.hpp"
#include "GLTexture.hpp"
#include "GLVertexArray.hpp"
#include "GLFrameBuffer.hpp"

#include "ImageReader.hpp"
#include "ImageWriterBmp.h"

namespace sp {

class GLRendererBase {
public:
    GLRendererBase(std::shared_ptr<IGLContext> context) :_context(context), _program(new GLProgram(context)) { }
    
    virtual ~GLRendererBase() {
        _context->SwitchContext();
    }
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
        _program->UpdateShader(vertexShader, fragmentShader);
        return true;
    }
    
    bool UpdateTexture(const std::vector<ImageBuffer> &buffers) {
        for (auto &buffer : buffers) {
            auto texture = std::make_shared<GLTexture>(_context, buffer);
            _textures.emplace_back(std::move(texture));
        }
        _needUpdate = true;
        return true;
    }
    
    bool UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        _textures = textures;
        _needUpdate = true;
        return true;
    }
    
    bool UpdateOutputTexture(std::shared_ptr<GLTexture> texture) {
        _outputTexture = texture;
        _needUpdate = true;
        return true;
    }
    
    std::shared_ptr<GLTexture> GetOutputTexture() {
        return _outputTexture;
    }
    
    bool UpdateTransform(const glm::mat4 &transform) {
        _transform = transform;
        return true;
    }
    
    const glm::mat4 &GetTransform() {
        return _transform;
    }
    
    bool UpdateUniform(const std::string &name, GLUniform uniform) {
        return _program->UpdateUniform(name, uniform);
    }
    
    void SetClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
        _clearColor[0] = r;
        _clearColor[1] = g;
        _clearColor[2] = b;
        _clearColor[3] = a;
    }
    
    void SetEnableBlend(bool enable) {
        if (enable)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }
    
    // 立即更新
    bool SyncUpdate() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->SwitchContext() == false)
            return false;
        // 更新内部参数
        return _InternalUpdate();
    }
    
    bool Render() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->SwitchContext() == false)
            return false;
        // 更新内部参数
        if (_InternalUpdate() == false)
            return false;
        
        return _InternalRender();
    }

protected:
    virtual bool _InternalUpdate();
    virtual bool _InternalRender();

protected:
    const std::shared_ptr<IGLContext> _context;
    bool _needUpdate = true;
    
    /// 输入纹理，多输入
    std::vector<std::shared_ptr<GLTexture>> _textures;
    /// 输出纹理，单输出
    std::shared_ptr<GLTexture> _outputTexture;
    /// 变换矩阵
    glm::mat4 _transform = glm::mat4(1.0f);
    
    std::unique_ptr<GLProgram> _program;
    GLVertexArray _vertexArray = GLVertexArray(_context);
    std::shared_ptr<GLFrameBuffer> _frameBuffer = std::make_shared<GLFrameBuffer>(_context);
    
    std::array<GLfloat, 4> _clearColor;
};




}

#endif /* GLRendererBase_hpp */
