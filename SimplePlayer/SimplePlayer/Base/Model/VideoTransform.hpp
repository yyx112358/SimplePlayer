//
//  VideoTransform.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/2/26.
//

#pragma once

#include "Geometry.hpp"
#include "SPEnum.h"

#include "glm/fwd.hpp"
#include <optional>

namespace sp {

class IVideoTransform {
public:
    virtual ~IVideoTransform() {}
    
    virtual IVideoTransform *clone() const = 0;
    
    virtual bool fromTransform(const IVideoTransform &other) = 0;
    virtual glm::mat4 toMatrix() const = 0;
    
// 如果支持从mat4转换，可以大大减少fromTransform的难度。但并非所有的mat4都能转换到特定的transform，待议
//    virtual bool fromMatrix(const glm::mat4 &other) = 0; //
};


/// @brief 2D视频变换，支持2D音视频的绝大部分需求
/// 变换次序是 缩放+翻转 -> 旋转 -> 平移
class VideoTransform2D : public IVideoTransform {
    
public:
    virtual IVideoTransform *clone() const override;
    
    virtual bool fromTransform(const IVideoTransform &other) {return false;}
    virtual glm::mat4 toMatrix() const;

public:
    /// 缩放，与fillmode组合起来看。例如，fillmode = Fit, scale = 0.5代表首先缩放到正好可以以Fit模式显示在outSize下，随后再缩放到原来的0.5倍
    float _scaleX = 1, _scaleY = 1;
    
    /// 填充模式
    EDisplayFillMode _fillmode = EDisplayFillMode::Fit;
    
    /// 90度倍数的旋转，逆时针为正方向。会影响scale = 1时的实际缩放倍数
    EDisplayRotation _displayRotation = EDisplayRotation::Rotation0;
    
    /// 在displayRotation基础上叠加的自由旋转角
    float _freeRotation = 0;
    
    /// 翻转
    bool _flipX = false, _flipY = false;
    
    /// 输入图像分辨率，上层一般不需要关心
    sp::Size _inSize;
    
    /// 输出分辨率
    sp::Size _outSize;
};

}
