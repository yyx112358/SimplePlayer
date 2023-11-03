//
//  ISPUnit.cpp
//  SimplePlayer
//
//  Created by YangYixuan on 2023/10/27.
//

#include "ISPUnit.hpp"

#include <memory>
#include <future>

#include "Pipeline.hpp"

namespace sp {

class ISPUnit : std::enable_shared_from_this<ISPUnit> {
public:
    
    virtual bool start(bool isSync) = 0;
    virtual bool stop(bool isSync) = 0;
    virtual bool seek(bool isSync) = 0;
    virtual bool pause(bool isSync) = 0;
    
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
