//
//  ISPGraph.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2024/1/13.
//

#pragma once
#include "ISPGraphContext.hpp"
#include "ISPMediaControl.hpp"
#include "SPMediaModel.hpp"

namespace sp {

class ISPGraph : public ISPMediaControl, public ISPGraphContext, public std::enable_shared_from_this<ISPGraph>
{
public:
    ISPGraph() = default;
    ISPGraph(const ISPGraph&) = delete;
    ISPGraph& operator=(const ISPGraph&) = delete;
    
public:
    virtual std::future<bool> updateModel(const SPMediaModel &model, bool isSync) = 0;
};

}
