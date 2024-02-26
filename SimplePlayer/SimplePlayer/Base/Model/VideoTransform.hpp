//
//  VideoTransform.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/26.
//

#pragma once

#include "Geometry.hpp"

#include "glm/fwd.hpp"
#include <optional>

namespace sp {

class IVideoTransform {
public:
    virtual ~IVideoTransform() {}
    
    virtual bool fromTransform(const IVideoTransform &other) = 0;
    virtual glm::mat4 toMatrix() const = 0;
    
// 如果支持从mat4转换，可以大大减少fromTransform的难度。但并非所有的mat4都能转换到特定的transform，待议
//    virtual bool fromMatrix(const glm::mat4 &other) = 0; //
};

/// @brief 支持根据Fillmode自动转换
class VideoTransformFillmode : public IVideoTransform {
public:
public:
    /// 填充模式
    enum class EFillMode
    {
        Fit,        // 【默认】适应，缩放至刚好不超出屏幕，可能有空隙。
        Stretch,    // 拉伸，填满屏幕。可能改变长宽比。
        Fill,       // 填充，缩放至刚好填满屏幕。不改变长宽比。
        Origin,     // 保持原始尺寸，不缩放。
    };
    
    /// 旋转，逆时针为正方向
    enum class EDisplayRotation
    {
        Rotation0,
        Rotation90,
        Rotation180,
        Rotation270,
    };
    
public:
    virtual bool fromTransform(const IVideoTransform &other) {return false;}
    virtual glm::mat4 toMatrix() const;

public:
    /// 填充模式
    EFillMode fillmode = EFillMode::Fit;
    
    /// 90度倍数的旋转，逆时针为正方向
    EDisplayRotation displayRotation = EDisplayRotation::Rotation0;
    
    /// 在displayRotation基础上叠加的自由旋转角
    float freeRotation = 0;
    
    /// 缩放，与fillmode组合起来看。例如，fillmode = Fit, scale = 0.5代表首先缩放到正好可以以Fit模式显示在outSize下，随后再缩放到原来的0.5倍
    float scale = 1;
    
    /// 翻转
    bool flipX = false, flipY = false;
    
    /// 输入图像分辨率，上层一般不需要关心
    sp::Size inSize;
    
    /// 输出分辨率
    sp::Size outSize;
};

/// @brief 视频旋转数据描述
/// @note 操作次序：transform -> scale -> rotation ->flip。
/// @note 底层实际上操作的都是VideoTransform转换而成的mat4 
class VideoTransformAbs : public IVideoTransform {
public:
    virtual bool fromTransform(const IVideoTransform &other) {return false;}
    virtual glm::mat4 toMatrix() const;
    
public:
    // 操作次序：transform -> scale -> rotation ->flip
    
    float transformX = 0, transformY = 0;
    float scaleX = 1, scaleY = 1;
    float rotation = 0;
//    ERotation rightRotation = ERotation::Rotation0; // 90度整数倍旋转，优先级低于非整数倍旋转
    bool flipX = false, flipY = false;
};

}
