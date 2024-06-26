//
//  GLBaseDefine.h
//  SimplePlayer
//
//  Created by YangYixuan on 2023/7/9.
//

#pragma once

#include "glm/fwd.hpp"
#include <memory>
#include <optional>

#if __APPLE__
#define GL_SILENCE_DEPRECATION
#import <OpenGL/gl3.h>
#endif

#include "SPLog.h"


// 持有GL ID，支持自动释放
// std::unique_ptr<GLuint, void(*)(GLuint *)>在XCode调试器无法查看内部的值
class GL_IdHolder {
public:
    typedef void (*DELETER)(GLuint);
public:
    explicit GL_IdHolder(DELETER deleter): _id(std::nullopt), _deleter(deleter) { SPASSERT(_deleter != nullptr); }
    explicit GL_IdHolder(std::optional<GLuint>id, DELETER deleter): _id(id), _deleter(deleter) { assert(_deleter != nullptr); }
    GL_IdHolder(GL_IdHolder &&other): _id(other.release()), _deleter(other._deleter) { SPASSERT(_deleter != nullptr); }
    GL_IdHolder(const GL_IdHolder &) = delete;
    GL_IdHolder& operator= (GL_IdHolder &&other)
    {
        SPASSERT(other._deleter == _deleter);
        reset(other.release());
        return *this;
    }
    ~GL_IdHolder() { reset(); }
    
    
    bool has_value() const { return _id.has_value(); }
    explicit operator bool() const { return _id.has_value(); }
    
    GLuint operator*() const { return *_id; }
    std::optional<GLuint> id() const { return _id; }
    std::optional<GLuint>& id() { return _id; }
    
    void reset(std::optional<GLuint>id = std::nullopt)
    {
        if (_id.has_value() && _deleter)
            _deleter(*_id);
        _id = id;
    }
    std::optional<GLuint> release()
    {
        decltype(_id) ret = _id;
        _id.reset();
        return ret;
    }
    
protected:
    std::optional<GLuint> _id = std::nullopt;
    const DELETER _deleter = nullptr;
};
