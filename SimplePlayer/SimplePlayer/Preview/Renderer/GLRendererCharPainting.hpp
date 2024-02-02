//
//  GLRendererCharPainting.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "GLRendererBase.hpp"

namespace sp {


class GLRendererCharPainting : public GLRendererBase {
public:
    GLRendererCharPainting(std::shared_ptr<IGLContext> context);
    virtual ~GLRendererCharPainting() {}
    
    bool UpdateTexture(const std::vector<std::shared_ptr<GLTexture>> &textures) override;
    
    void SetCharSize(int width, int height);
    
protected:
    
    void _AddVertex();
    bool _InternalUpdate() override;
    bool _InternalRender() override;
    
protected:
    int _charWidth = 8, _charHeight = 12;
    bool _needUpdateVertex = true;
};


}
