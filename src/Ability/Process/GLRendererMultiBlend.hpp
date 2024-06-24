//
//  GLRendererMultiBlend.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/1.
//

#pragma once

#include "GLRendererBase.hpp"
#include "VideoTransform.hpp"

namespace sp {

/// 多输入混合
class GLRendererMultiBlend : public GLRendererBase {
public:
    struct VertexBlend {
        std::array<GLfloat, 3>location;
        std::array<GLfloat, 2>texture;
        GLint textureId;
    };
    
public:
    GLRendererMultiBlend(std::shared_ptr<IGLContext> context);
    virtual ~GLRendererMultiBlend() = default;
    
    static size_t MAX_SUPPORT_INPUT_SIZE(); // 最大支持的输入纹理数
    
    virtual bool UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) override {
        _textures = textures;
        _textureTransforms.resize(_textures.size());
        return true;
    }
    
public:
    void UpdateTransform(std::vector<sp::VideoTransform2D> transforms) {
        _textureTransforms = transforms;
    }
    void UpdateTransform(int id, sp::VideoTransform2D transform){
        _textureTransforms[id] = transform;
    }
    virtual const IVideoTransform &GetTransform() const final {
        SPASSERT_NOT_IMPL;
        return _textureTransforms[0];
    }
    sp::VideoTransform2D GetBlendTransform(int id) const {
        return _textureTransforms.at(id);
    }
    sp::VideoTransform2D &GetBlendTransform(int id) {
        return _textureTransforms.at(id);
    }
    
public:
    void SetDisplayRotation(int id, EDisplayRotation value) { _textureTransforms[id]._displayRotation = value; }
    EDisplayRotation GetDisplayRotation(int id) const { return _textureTransforms[id]._displayRotation; }
    
    void SetFreeRotation(int id, float value) { _textureTransforms[id]._freeRotation = value; }
    float GetFreeRotation(int id) const { return _textureTransforms[id]._freeRotation; }
    
    void SetScale(int id, float value) { _textureTransforms[id]._scaleX = _textureTransforms[id]._scaleY = value; }
    
    void SetTransX(int id, float value) { _textureTransforms[id]._transX = value; }
    void SetTransY(int id, float value) { _textureTransforms[id]._transY = value; }
    
protected:
    bool _InternalUpdate() override;
    bool _InternalRender() override;
    
    size_t _UpdateVertexArray(size_t textureNum);
protected:
    size_t _vertexUpdatedNum = 0;
    
    // TODO: Alpha透明度
    std::vector<sp::VideoTransform2D> _textureTransforms;
};

}
