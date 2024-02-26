//
//  GLRendererPreview.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "GLRendererBase.hpp"
#include "VideoTransform.hpp"

namespace sp {


class GLRendererPreview : public GLRendererBase
{

public:
    GLRendererPreview(std::shared_ptr<IGLContext> context);
    virtual ~GLRendererPreview() {}
    
    void UpdatePreviewSize(int width, int height)
    {
        _transformPreview.outSize.width = width;
        _transformPreview.outSize.height = height;
    }
    
    void UpdatePreviewFillMode(sp::VideoTransformFillmode::EFillMode fillmode)
    {
        _transformPreview.fillmode = fillmode;
    }
    
    void UpdatePreviewRotation(sp::VideoTransformFillmode::EDisplayRotation rotation)
    {
        _transformPreview.displayRotation = rotation;
    }
    
    void UpdatePreviewFlip(bool flipX, bool flipY)
    {
        _transformPreview.flipX = flipX;
        _transformPreview.flipY = flipY;
    }
    
protected:
    virtual void _UpdateTransform();
    
    bool _InternalUpdate() override;
    
    bool _InternalRender() override;
    
protected:
    VideoTransformFillmode _transformPreview;
};



}

