//
//  GLRendererPreview.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "GLRendererBase.hpp"

namespace sp {


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
    GLRendererPreview(std::shared_ptr<IGLContext> context):GLRendererBase(context) {}
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
    virtual void _UpdateTransform();
    
    bool _InternalUpdate() override;
    
    bool _InternalRender() override;
    
protected:
    int _previewWidth = 0, _previewHeight = 0;
    EFillMode _fillmode = EFillMode::Fit;
    ERotation _rotation = ERotation::Rotation0;
    bool _flipX = false, _flipY = false;
};



}

