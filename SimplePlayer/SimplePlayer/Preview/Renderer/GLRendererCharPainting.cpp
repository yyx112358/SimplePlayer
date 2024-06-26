//
//  GLRendererCharPainting.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#include "GLRendererCharPainting.hpp"
#include "ImageWriterUIImage.h"

using namespace sp;

GLRendererCharPainting::GLRendererCharPainting(std::shared_ptr<IGLContext> context):GLRendererBase(context)
{
    const GLchar *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 transform;
    uniform sampler2D textures[2];
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
                vec4 color = texture(textures[0], vec2(float(x) / texWidth, float(y) / texHeight));
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
        
    uniform sampler2D textures[2];

    void main()
    {
        FragColor = texture(textures[1], vtxTexCoord).rgba * vec4(vtxColor, 1.0);
        FragColor = vec4(vtxColor, 1.0);
    })";
    UpdateShader({vertexShaderSource}, {fragmentShaderSource});
    
}

bool GLRendererCharPainting::UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) 
{
    if (_textures.size() != textures.size())
        _needUpdate = true;
    else {
        for (int i = 0; i < textures.size(); i++) {
            if (_textures[i] == nullptr || textures[i] == nullptr
                || _textures[i]->getBuffer().has_value() == false
                || textures[i]->getBuffer().has_value() == false
                || textures[i]->getBuffer()->equalExceptData(*_textures[i]->getBuffer()) == false) {
                _needUpdateVertex = true;
                break;
            }
        }
    }
    return GLRendererBase::UpdateTexture(textures);
}

void GLRendererCharPainting::SetCharSize(int width, int height)
{
    _charWidth = width;
    _charHeight = height;
    _needUpdate = true;
    _needUpdateVertex = true;
}


void GLRendererCharPainting::_AddVertex()
{
    if (_needUpdateVertex == false)
        return;
    
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
    _needUpdateVertex = false;
}

bool GLRendererCharPainting::_InternalUpdate()
{
    if (_needUpdate == false)
        return true;
    
    
    _AddVertex();
    
    return GLRendererBase::_InternalUpdate();
}

bool GLRendererCharPainting::_InternalRender()
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
        if (auto buffer = _frameBuffer->DownloadFrameBuffer(GL_BGRA)) {
//                writeBMP2File("output.bmp", buffer->data.get(), buffer->width, buffer->height, 4);
            SPNSObjectHolder holder = writeRGBA2UIImage(buffer->data.get(), buffer->width, buffer->height, 4, true);
            b = false;
        }
    }
    return true;
}
