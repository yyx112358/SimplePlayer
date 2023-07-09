//
//  GLBaseDefine.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "../../../../../thirdParty/glm/glm/fwd.hpp"
#include <memory>

#if __APPLE__

#define GL_SILENCE_DEPRECATION
#import <OpenGL/gl3.h>

#endif


typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放
