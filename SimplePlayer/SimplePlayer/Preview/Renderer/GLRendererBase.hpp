//
//  GLRendererBase.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLRendererBase_hpp
#define GLRendererBase_hpp

#import <Foundation/Foundation.h>
#include <cstddef>
#include <string>
#include <optional>
#include <array>
#include <any>


#define GL_SILENCE_DEPRECATION
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl3.h>

#import "../../../../thirdParty/glm/glm/glm.hpp"
#include "../../../../thirdParty/glm/glm/gtc/matrix_transform.hpp"
#include "../../../../thirdParty/glm/glm/gtc/type_ptr.hpp"

#import "GLContext.hpp"
#include "ImageWriterBmp.h"

namespace sp {

class GLRendererBase {
public:
    GLRendererBase(std::shared_ptr<GLContext> context) :_context(context), _program(new GLProgram(context)) { }
    
    virtual ~GLRendererBase() {
        _context->switchContext();
    }
    
    bool UpdateShader(const std::vector<std::string> &vertexShader, const std::vector<std::string> &fragmentShader) {
        _program->UpdateShader(vertexShader, fragmentShader);
        return true;
    }
    
    bool UpdateTexture(const std::vector<ImageBuffer> &buffers) {
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
//        assert(buffers.size() <= MAX_TEXTURE_UNIT);
        
        for (auto &buffer : buffers) {
            auto texture = std::make_shared<GLTexture>(_context, buffer);
            _textures.emplace_back(std::move(texture));
        }
        _needUpdate = true;
        return true;
    }
    
    bool UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) {
        GLint MAX_TEXTURE_UNIT;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MAX_TEXTURE_UNIT);
        assert(textures.size() <= MAX_TEXTURE_UNIT);
        
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
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        return _InternalUpdate();
    }
    
    bool Render() {
        // 获取当前OpenGL上下文
        if (_context == nullptr || _context->switchContext() == false)
            return false;
        // 更新内部参数
        if (_InternalUpdate() == false)
            return false;
        
        return _InternalRender();
    }
protected:
    virtual bool _InternalUpdate() {
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
        //        if (CheckError())
        //            return false;
        
        _needUpdate = false;
        return true;
    }
    
    virtual bool _InternalRender() {
        // 上屏绘制
        _frameBuffer->Activate();
        glViewport(0, 0, _outputTexture->width(), _outputTexture->height());
        glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        
        //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        _program->Activate(); // 启用Shader程序
        CheckError();
        
        _vertexArray.Activate();
        for (int i = 0; i < _textures.size(); i++) {
            assert(_textures[i]->id().has_value());
            
            glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元1
            glBindTexture(GL_TEXTURE_2D, *_textures[i]->id()); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
            
            char textureName[] = "texture00";
            snprintf(textureName, sizeof(textureName), "texture%d", i);
            _program->UpdateUniform(textureName, i); // 更新纹理uniform
        }
        
        CheckError();
        
        _program->UpdateUniform("transform", _transform);
        
        _program->FlushUniform();
        
        _vertexArray.Render();
        CheckError();
        
        
        if (CheckError())
            return false;
        
        return true;
    }
    

    

    
protected:
    const std::shared_ptr<GLContext> _context;
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

class GLRendererPreview : public GLRendererBase
{
public:
    /// 填充模式
    enum class EFillMode
    {
        Fit,        // 【默认】适应，缩放至刚好不超出屏幕，可能有空隙。
        Stretch,    // 拉伸，填满屏幕。可能改变长宽比。
        Fill,       // 填充，缩放至刚好填满屏幕。不改变长宽比。
        Origin,     // 保持原始尺寸，不缩放。
        Free,       // 自由，由_transform变换矩阵决定
    };
    /// 旋转，逆时针为正方向
    enum class ERotation
    {
        Rotation0,
        Rotation90,
        Rotation180,
        Rotation270,
    };

public:
    GLRendererPreview(std::shared_ptr<GLContext> context):GLRendererBase(context) {}
    virtual ~GLRendererPreview() {}
    
    void UpdatePreviewSize(int width, int height)
    {
        _previewWidth = width;
        _previewHeight = height;
    }
    
    void UpdatePreviewFillMode(EFillMode fillmode)
    {
        _fillmode = fillmode;
    }
    
    void UpdatePreviewRotation(ERotation rotation)
    {
        _rotation = rotation;
    }
    
    void UpdatePreviewFlip(bool flipX, bool flipY)
    {
        _flipX = flipX;
        _flipY = flipY;
    }
    
protected:
    virtual void _UpdateTransform()
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
    
    bool _InternalUpdate() override
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
    
    bool _InternalRender() override
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
        
        if (CheckError())
            return false;
        
        return true;
    }
    
protected:
    int _previewWidth = 0, _previewHeight = 0;
    EFillMode _fillmode = EFillMode::Fit;
    ERotation _rotation = ERotation::Rotation0;
    bool _flipX = false, _flipY = false;
};

class GLRendererCharPainting : public GLRendererBase {
public:
    GLRendererCharPainting(std::shared_ptr<GLContext> context):GLRendererBase(context) {}
    virtual ~GLRendererCharPainting() {}
    
    void SetCharSize(int width, int height)
    {
        _charWidth = width;
        _charHeight = height;
        _needUpdate = true;
    }
    
protected:
    
    void _AddVertex()
    {
        // 将整个画面划分为_charWidth * _charHeight的小矩形
        int texWidth = _textures[0]->width(), texHeight = _textures[0]->height();
        int charWidth = _charWidth, charHeight = _charHeight;
        std::vector<GLVertexArray::VertexBuffer> bufs;
        std::vector<GLVertexArray::ElementBuffer> elems;
        GLuint rectCnt = 0;
        for (int y = 0; y < texHeight; y += charHeight) {
            for (int x = 0; x < texWidth; x += charWidth) {
                // 这里其实还有优化空间，矩形上的每一个点也是相邻矩形上的点，因此VBO数量可以压缩为1/4。
                // 但是现在的性能已经很好了，一帧不到2ms。不做进一步优化了。
                { // 左上
                    float posX = x, posY = y;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  0;
                    bufs.push_back(vtx);
                }
                { // 右上
                    float posX = x + charWidth, posY = y;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  1;
                    bufs.push_back(vtx);
                }
                { // 左下
                    float posX = x, posY = y + charHeight;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  2;
                    bufs.push_back(vtx);
                }
                { // 右下
                    float posX = x + charWidth, posY = y + charHeight;
                    GLVertexArray::VertexBuffer vtx;
                    vtx.location[0] = -1 + (posX / texWidth) * 2;
                    vtx.location[1] = -1 + (1 - posY / texHeight) * 2;
                    vtx.texture[0]  =  3;
                    bufs.push_back(vtx);
                }
                elems.push_back({rectCnt * 4 + 0, rectCnt * 4 + 1, rectCnt * 4 + 2});
                elems.push_back({rectCnt * 4 + 1, rectCnt * 4 + 3, rectCnt * 4 + 2});
                rectCnt++;
            }
        }
        _vertexArray.UpdateVertexBuffer(bufs);
        _vertexArray.UpdateElementBuffer(elems);
        _vertexArray.Activate();
    }
    
    bool _InternalUpdate() override
    {
        if (_needUpdate == false)
            return true;
        
        const GLchar *vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;

        uniform mat4 transform;
        uniform sampler2D texture0;
        uniform sampler2D texture1;
        uniform int texWidth, texHeight;
        uniform int charWidth, charHeight;

        out vec3 vtxColor;
        out vec2 vtxTexCoord;

        void main()
        {
            gl_Position = transform * vec4(aPos.x, aPos.y, 0.0, 1.0);
        
            // 计算采样矩形坐标范围，aTexCoord.x是顶点在小矩形中的位置（0左上，1右上，2左下，3右下）
            int posX = int((aPos.x + 1) / 2 * texWidth + 0.5), posY = int((-aPos.y + 1) / 2 * texHeight + 0.5); // 注意，这里需要+0.5，以避免浮点误差导致计算出的整数posX、posY比预期值小1
            if (aTexCoord.x == 1 || aTexCoord.x == 3)
                posX -= charWidth;
            if (aTexCoord.x == 2 || aTexCoord.x == 3)
                posY -= charHeight;
            
            int left = posX / charWidth * charWidth, top = posY / charHeight * charHeight;
            int right = min(left + charWidth, texWidth);
            int bottom = min(top + charHeight, texHeight);
            
            // 计算平均灰度
            float sumR = 0, sumG = 0, sumB = 0;
            int xstep = 4, ystep = 4; // 不需要每一个点取值，取一部分就可以
            int amount = 0;
            for (int y = top; y < bottom; y += ystep)
            {
                for (int x = left; x < right; x += xstep)
                {
                    vec4 color = texture(texture0, vec2(float(x) / texWidth, float(y) / texHeight));
                    sumR += color.r;
                    sumG += color.g;
                    sumB += color.b;
                    amount++;
                }
            }
            // 计算均值
            float gray = 0.299f * sumR / amount + 0.587f * sumG / amount + 0.114 * sumB / amount;
            gray *= 255.f / 256; // 限制最大值，避免溢出
            vtxColor = vec3(sumR / amount, sumG / amount, sumB / amount);
            
            // 计算对应的字符纹理坐标。字符纹理从左到右划分为256个charWidth * charHeight矩形，第n个矩形的平均灰度值为n。
            float charTexX = int(gray * 256) / 256.0, charTexY = 0;
            if (aTexCoord.x == 1 || aTexCoord.x == 3)
                charTexX = charTexX + 1.0f / 256;
            if (aTexCoord.x == 2 || aTexCoord.x == 3)
                charTexY = 1;
            vtxTexCoord = vec2(charTexX, charTexY);
        })";

        const GLchar *fragmentShaderSource = R"(
        #version 330 core
        in vec3  vtxColor;
        in vec2  vtxTexCoord;
        
        out vec4 FragColor;
        
        uniform sampler2D texture0;
        uniform sampler2D texture1;

        void main()
        {
            FragColor = texture(texture1, vtxTexCoord).rgba * vec4(vtxColor, 1.0);
        })";
        UpdateShader({vertexShaderSource}, {fragmentShaderSource});
        
        
        _AddVertex();
        
        return GLRendererBase::_InternalUpdate();
    }
    
    bool _InternalRender() override
    {
//        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
        
        UpdateUniform("texWidth", _textures[0]->width());
        UpdateUniform("texHeight", _textures[0]->height());
        UpdateUniform("charWidth", _charWidth);
        UpdateUniform("charHeight", _charHeight);
        
        GLRendererBase::_InternalRender();
//        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // 取消注释后将启用线框模式
        
        // 存bmp图
        static bool b = true;
        if (b) {
            if (auto buffer = _frameBuffer->GetOutputTexture()->DownloadBuffer(GL_BGRA))
                writeBMP2File("output.bmp", buffer->data.get(), buffer->width, buffer->height, 4);
            b = false;
        }
        return true;
    }
    
protected:
    int _charWidth = 8, _charHeight = 12;
};


}

#endif /* GLRendererBase_hpp */
