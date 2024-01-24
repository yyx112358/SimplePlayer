//
//  SPMediaModel.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#pragma once

#include <vector>
#include <memory>

namespace sp {

class SPMediaModel : std::enable_shared_from_this<SPMediaModel> {
public:
    std::vector<std::vector<std::string>> videoTracks;
    
};

}
