//
//  ISPUnit.hpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/10/27.
//

#pragma once

#include <memory>
#include <future>

#include "Pipeline.hpp"
#include "ISPMediaControl.hpp"

namespace sp {

class ISPUnit : public ISPMediaControl, public std::enable_shared_from_this<ISPUnit> {
public:
    
    virtual bool addPipeline(std::shared_ptr<sp::Pipeline>) = 0;
    
    virtual bool connect(std::shared_ptr<ISPUnit>) = 0;
    virtual bool disconnect() = 0;
    
protected:
    virtual bool _input() = 0;
    virtual bool _process(std::shared_ptr<sp::Pipeline>) = 0;
    virtual bool _output() = 0;
};

class SPUnitBase : public ISPUnit {
public:
    
    
    
};

}
