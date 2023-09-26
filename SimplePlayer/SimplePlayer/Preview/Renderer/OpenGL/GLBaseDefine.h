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


typedef std::unique_ptr<GLuint, void(*)(GLuint *)> GL_IdHolder; // 持有GL ID的unique_ptr，支持自动释放

// unique_ptr替代。
// 有自带deleter的unique_ptr在XCode调试器无法查看内部的值
class GLIdHolder {
public:
    typedef void (*DELETER)(GLuint);
public:
    explicit GLIdHolder(DELETER deleter): _id(std::nullopt), _deleter(deleter) { assert(_deleter != nullptr); }
    explicit GLIdHolder(std::optional<GLuint>id, DELETER deleter): _id(id), _deleter(deleter) { assert(_deleter != nullptr); }
    GLIdHolder(GLIdHolder &&other): _id(other.release()), _deleter(other._deleter) { assert(_deleter != nullptr); }
    GLIdHolder(const GLIdHolder &) = delete;
    GLIdHolder& operator= (GLIdHolder &&other)
    {
        assert(other._deleter == _deleter);
        reset(other.release());
        return *this;
    }
    ~GLIdHolder() { reset(); }
    
    
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
