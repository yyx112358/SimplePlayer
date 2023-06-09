//
//  GLContext.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/6/9.
//

#ifndef GLContext_hpp
#define GLContext_hpp
#include <any>

namespace sp {

class Parameter {
public:
    std::any parameter;
    std::any shadowParameter;
    
    void update(const std::any&input) {
        shadowParameter = input;
    }
    std::any& get() {
        return shadowParameter.has_value() ? shadowParameter : parameter;
    }
    std::any getReal() {
        return parameter;
    }
    std::any& getAndUpdate() {
        parameter.swap(shadowParameter);
        shadowParameter.reset();
        return get();
    }
    bool isUpdated() {
        return shadowParameter.has_value() == false;
    }
};

}

#endif /* GLContext_hpp */
