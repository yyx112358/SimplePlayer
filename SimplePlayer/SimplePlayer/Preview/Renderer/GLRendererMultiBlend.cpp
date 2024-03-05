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


GLRendererMultiBlend::GLRendererMultiBlend(std::shared_ptr<IGLContext> context) : GLRendererBase(context)
{
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
    _vertexUpdatedNum = 0;
}

size_t GLRendererMultiBlend::MAX_SUPPORT_INPUT_SIZE() 
{
    return IGLContext::GetMaxFragmentTextureUnits();
}

bool GLRendererMultiBlend::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;
    
    return GLRendererBase::_InternalUpdate();
}

bool GLRendererMultiBlend::_InternalRender() 
{
    // 上屏绘制
    _frameBuffer->Activate();
    glViewport(0, 0, _outputTexture->width(), _outputTexture->height());
    glClearColor(_clearColor[0], _clearColor[1], _clearColor[2], _clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    
    //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // 取消注释后将启用线框模式
    _program->Activate(); // 启用Shader程序
    GLCheckError();
    
    std::vector<std::shared_ptr<GLTexture>> texturesNotRender = _textures;
    std::vector<VideoTransform2D> transformsNotRender = _textureTransforms;
    while(texturesNotRender.size() > 0) {
        // OpenGL一次可渲染的纹理数有上限，因此需要分批渲染，一次渲染textureSize个
        size_t textureSize = std::min<size_t>(texturesNotRender.size(), MAX_SUPPORT_INPUT_SIZE());
        std::vector<std::shared_ptr<GLTexture>> textures(texturesNotRender.begin(), texturesNotRender.begin() + textureSize);
        std::vector<VideoTransform2D> transforms(transformsNotRender.begin(), transformsNotRender.begin() + textureSize);
        
        std::vector<GLint> textureIds;
        std::vector<glm::mat4> transformMats;
        
        // 如果已经上传的顶点数和待渲染的个数不一样，就需要重新传递顶点
        // 不建议使用discard丢弃顶点，参考https://stackoverflow.com/questions/8509051/is-discard-bad-for-program-performance-in-opengl
        if (_vertexUpdatedNum == 0 || _vertexUpdatedNum != textures.size())
            _vertexUpdatedNum = _UpdateVertexArray(textureSize);
        _vertexArray.Activate();
        
        for (int i = 0; i < textures.size(); i++) {
            SPASSERT(textures[i]->id().has_value());
            
            glActiveTexture(GL_TEXTURE0 + i); // 激活纹理单元i
            textures[i]->Activate(); // 绑定纹理。根据上下文，这个纹理绑定到了纹理单元1
            
            textureIds.push_back(i);
            
            transforms[i]._inSize.width = textures[i]->width();
            transforms[i]._inSize.height = textures[i]->height();
            transforms[i]._outSize.width = _outputTexture->width();
            transforms[i]._outSize.height = _outputTexture->height();
            
            transformMats.emplace_back(transforms[i].toMatrix());
        }
        UpdateUniform("textures", textureIds);
        UpdateUniform("transforms", transformMats);
        _program->FlushUniform();
        GLCheckError();
        
        _vertexArray.Render();
        GLCheckError();
        
        static bool b = true;
        if (b) {
            if (auto buffer = _frameBuffer->DownloadFrameBuffer(GL_BGRA)) {
                SPNSObjectHolder holder = writeRGBA2UIImage(buffer->data.get(), buffer->width, buffer->height, 4, true);
                b = false;
            }
        }
        texturesNotRender.erase(texturesNotRender.begin(), texturesNotRender.begin() + textureSize);
        transformsNotRender.erase(transformsNotRender.begin(), transformsNotRender.begin() + textureSize);
    }
    
    if (GLCheckError())
        return false;
    
    return true;
}

size_t GLRendererMultiBlend::_UpdateVertexArray(size_t textureNum) 
{
    std::vector<VertexBlend> vtxBufs;
    for (int i = 0; i < textureNum && i < MAX_SUPPORT_INPUT_SIZE(); i++) {
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
    }
    
    _vertexArray.UpdateVertexBufferEx(toBuffer(vtxBufs));
    _vertexArray.UpdateElementBuffer({});
    return _vertexArray.Activate() ? textureNum : 0;
}

GLVertexArray::VertexBufferEx toBuffer(const std::vector<GLRendererMultiBlend::VertexBlend> &src)
{
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
