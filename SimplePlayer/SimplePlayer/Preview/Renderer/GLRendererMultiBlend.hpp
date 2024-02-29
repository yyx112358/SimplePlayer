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
protected:
    bool _InternalUpdate() override;
    bool _InternalRender() override;
    
protected:
    static constexpr size_t ARRAY_SIZE = 64;
    bool _needUpdateVertex = false;
    
    /// 使用中的纹理标志
    std::bitset<ARRAY_SIZE> _activateTextureFlags;
    std::vector<sp::VideoTransformFillmode> _textureTransforms;
};

}
