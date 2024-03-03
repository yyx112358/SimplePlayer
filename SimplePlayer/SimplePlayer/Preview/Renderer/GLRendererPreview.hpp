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
        GetPreviewTransform()._outSize.width = width;
        GetPreviewTransform()._outSize.height = height;
    }
    
    void UpdatePreviewFillMode(sp::EDisplayFillMode fillmode)
    {
        GetPreviewTransform()._fillmode = fillmode;
    }
    
    void UpdatePreviewRotation(sp::EDisplayRotation rotation)
    {
        GetPreviewTransform()._displayRotation = rotation;
    }
    
    void UpdatePreviewFlip(bool flipX, bool flipY)
    {
        GetPreviewTransform()._flipX = flipX;
        GetPreviewTransform()._flipY = flipY;
    }
    
    sp::VideoTransform2D & GetPreviewTransform() {
        return *static_cast<sp::VideoTransform2D *>(_transform.get());
    }
protected:
    
    bool _InternalUpdate() override;
    
    bool _InternalRender() override;
};



}

