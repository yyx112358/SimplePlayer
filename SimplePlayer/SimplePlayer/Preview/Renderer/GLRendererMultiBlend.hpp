//
//  GLRendererMultiBlend.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/1.
//

#pragma once

#include "GLRendererBase.hpp"
#include "VideoTransform.hpp"
#include <bitset>

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
    void SetDisplayRotation(int id, EDisplayRotation value) {
        _textureTransforms[id]._displayRotation = value;
    }
    EDisplayRotation GetDisplayRotation(int id) const {
        return _textureTransforms[id]._displayRotation;
    }
    void SetFreeRotation(int id, float value) {
        _textureTransforms[id]._freeRotation = value;
    }
    float GetFreeRotation(int id) const {
        return _textureTransforms[id]._freeRotation;
    }
    void SetScale(int id, float value) {
        _textureTransforms[id]._scaleX = _textureTransforms[id]._scaleY = value;
    }    
    
protected:
    bool _InternalUpdate() override;
    bool _InternalRender() override;
    
protected:
    static constexpr size_t ARRAY_SIZE = 64;
    bool _needUpdateVertex = false;
    
    /// 使用中的纹理标志
    std::bitset<ARRAY_SIZE> _activateTextureFlags;
    std::vector<sp::VideoTransform2D> _textureTransforms;
};

}
