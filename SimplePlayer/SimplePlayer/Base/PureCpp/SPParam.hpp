//
//  SPParam.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/10/27.
//

#pragma once

#include <variant>
#include <any>
#include <memory>

namespace sp {

typedef std::variant
   <bool, int32_t, uint32_t, float, double,
    std::string,
    std::weak_ptr<void>, std::shared_ptr<void>,
    std::any>
SPParam;


}
