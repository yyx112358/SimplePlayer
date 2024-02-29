//
//  GLRendererMultiBlend.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/1.
//

#include "GLRendererMultiBlend.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "ImageWriterUIImage.h"


using namespace sp;

GLVertexArray::VertexBufferEx toBuffer(const std::vector<GLRendererMultiBlend::VertexBlend> &src);


GLRendererMultiBlend::GLRendererMultiBlend(std::shared_ptr<IGLContext> context) : GLRendererBase(context) {
    UpdateShader({
        R"(
        #version 330 core
        layout (location = 0) in vec3   inPos;
        layout (location = 1) in vec2   inTexCoord;
        layout (location = 2) in int    inTexId;

        out vec2 vtxTexCoord;
        flat out int  vtxTexId;
        
        uniform mat4 transforms[16];

        void main()
        {
            gl_Position = transforms[inTexId] * vec4(inPos.xyz, 1.0);
            vtxTexCoord = inTexCoord;
            vtxTexId = inTexId;
        }
        )"},

        {R"(
        #version 330 core
        in vec2  vtxTexCoord;
        flat in int   vtxTexId;

        out vec4 FragColor;

        uniform sampler2D textures[16];

        void main()
        {
            FragColor = texture(textures[vtxTexId], vtxTexCoord);
        }
        )"});
    _needUpdateVertex = true;
}

size_t GLRendererMultiBlend::MAX_SUPPORT_INPUT_SIZE() 
{
    static size_t value = std::min<size_t>(IGLContext::GetMaxFragmentTextureUnits(), ARRAY_SIZE);
    return value;
}

bool GLRendererMultiBlend::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;
    
    if (_needUpdateVertex == false)
        return true;
    
    std::vector<VertexBlend> vtxBufs;
    for (int i = 0; i < _textures.size() && i < MAX_SUPPORT_INPUT_SIZE(); i++) {
        float depth = (float)i / MAX_SUPPORT_INPUT_SIZE();
        std::vector<VertexBlend> vtxBuf = {
            {{-1.0, 1.0, depth}, {0.0, 0.0}, i}, // 左上
            {{ 1.0, 1.0, depth}, {1.0, 0.0}, i}, // 右上
            {{-1.0,-1.0, depth}, {0.0, 1.0}, i}, // 左下
            {{-1.0,-1.0, depth}, {0.0, 1.0}, i}, // 左下
            {{ 1.0, 1.0, depth}, {1.0, 0.0}, i}, // 右上
            {{ 1.0,-1.0, depth}, {1.0, 1.0}, i}, // 右下
        };
        vtxBufs.insert(vtxBufs.end(), vtxBuf.begin(), vtxBuf.end());

        _activateTextureFlags[i] = true;
    }
    
    _vertexArray.UpdateVertexBufferEx(toBuffer(vtxBufs));
    _vertexArray.UpdateElementBuffer({});
    _vertexArray.Activate();
    _needUpdateVertex = false;
    
    _textureTransforms.resize(2);
    _textureTransforms[0].inSize.width = _textures[0]->width();
    _textureTransforms[0].inSize.height = _textures[0]->height();
    _textureTransforms[0].outSize.width = _outputTexture->width();
    _textureTransforms[0].outSize.height = _outputTexture->height();
    _textureTransforms[0].scale = 0.5;
    _textureTransforms[0].displayRotation = VideoTransformFillmode::EDisplayRotation::Rotation0;
//    _textureTransforms[0] = glm::identity<glm::mat4>();
//    _textureTransforms[0] = glm::translate(_textureTransforms[0], glm::vec3(0.3, 0.4, 1.0f));
//    _textureTransforms[0] = glm::scale(_textureTransforms[0], glm::vec3(0.3, 0.3, 1.0f));
//    _textureTransforms[0] = glm::scale(_textureTransforms[0], glm::vec3(texWidth / _previewWidth, texHeight / _previewHeight, 1.0f)); // 归一化
//    _textureTransforms[0] = glm::rotate(_textureTransforms[0], glm::pi<GLfloat>() / 2 * (int)30, glm::vec3(0.0f, 0.0f, 1.0f)); // 旋转
    
    
    _textureTransforms[1].inSize.width = _textures[1]->width();
    _textureTransforms[1].inSize.height = _textures[1]->height();
    _textureTransforms[1].outSize.width = _outputTexture->width();
    _textureTransforms[1].outSize.height = _outputTexture->height();
    _textureTransforms[1].scale = 0.003;
    
    return GLRendererBase::_InternalUpdate();
}

bool GLRendererMultiBlend::_InternalRender() {
    // 上屏绘制
    _frameBuffer->Activate();
    glViewport(0, 0, _outputTexture->width(), _outputTexture->height());
    glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
    _program->Activate(); // 启用Shader程序
    GLCheckError();
    
    static int r = 0;
    _textureTransforms[0].freeRotation = r++;
    
    _vertexArray.Activate();
    std::vector<GLint> textureIds;
    std::vector<glm::mat4> transforms;
    for (int i = 0; i < _textures.size(); i++) {
        SPASSERT(_textures[i]->id().has_value());
        
        glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元1
        _textures[i]->Activate(); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
        
        textureIds.push_back(i);
        transforms.emplace_back(_textureTransforms[i].toMatrix());
    }
    UpdateUniform("textures", textureIds);
    
    GLCheckError();
    
    _program->UpdateUniform("transforms", transforms);
    
    _program->FlushUniform();
    
    _vertexArray.Render();
    GLCheckError();
    
    bool b = true;
    if (auto buffer = _frameBuffer->DownloadFrameBuffer(GL_BGRA)) {
        SPNSObjectHolder holder = writeRGBA2UIImage(buffer->data.get(), buffer->width, buffer->height, 4, true);
        b = false;
    }
    
    if (GLCheckError())
        return false;
    
    return true;
}

GLVertexArray::VertexBufferEx toBuffer(const std::vector<GLRendererMultiBlend::VertexBlend> &src) {
    GLVertexArray::VertexBufferEx vtxBuf;
    vtxBuf.vertexNum = src.size();
    vtxBuf.descs = {
        {std::type_index(typeid(GLfloat)), src[0].location.size()},
        {std::type_index(typeid(GLfloat)), src[0].texture.size()},
        {std::type_index(typeid(GLint)), 1},
    };
    size_t length = sizeof(src[0]) * src.size();
    SPASSERT0(length == vtxBuf.sizeInByte());
    vtxBuf.buf.reset(new char[length]);
    memcpy(vtxBuf.buf.get(), src.data(), length);
    return vtxBuf;
}
