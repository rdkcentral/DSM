#include "container-runtime.hpp"
#include <iostream>

void empty_callback(std::string id, ContainerRuntime::ContainerState state){
    std::cout<<"empty_callback(id:"<<id<<", state:"<<state<<")"<<std::endl;
}

// Container Data Structure
//////////////////////////////////////////////////////////////////////////////////////////
ContainerData::ContainerData(std::string ext_id):ext_id(ext_id), state(ContainerRuntime::Undefined){}
void ContainerData::notify(){update_callback(ext_id, state);}

// Container Data Runtime
//////////////////////////////////////////////////////////////////////////////////////////
ContainerRuntime::ContainerRuntime(const nlohmann::json config): 
        config(config), 
        worker{std::make_shared<ThreadWorker>()}, 
        container_adapter(nullptr){
    std::cout<<"<<create>> ContainerRuntime::ContainerRuntime(default)"<<std::endl;
    container_adapter = std::make_shared<ContainerRuntimeAdapter>();

    worker->start();
}

auto ContainerRuntime::start(const std::string &id, const std::string &bundle_path) -> bool {
    std::cout<<"ContainerRuntime::start(bundle_path:"<<bundle_path <<")"<<std::endl;
    worker->execute_async_task([=](){
        container_adapter->start(id, bundle_path);
    });
    return false; 
}
auto ContainerRuntime::stop(std::string id) -> bool {
    std::cout<<"ContainerRuntime::stop(container_id:"<<id<<")"<<std::endl;
    worker->execute_async_task([=](){
        container_adapter->stop(id);
    });
    return false;
}
auto ContainerRuntime::list() -> std::vector<std::shared_ptr<ContainerData> > {
    std::cout<<"auto ContainerRuntime::list()"<<std::endl;
    container_adapter->list();
    return containers_list; 
}

auto ContainerRuntime::find_container(std::string ext_id) -> std::shared_ptr<ContainerData>{
    auto container = std::find_if(begin(containers_list), end(containers_list), 
                                [=] (std::shared_ptr<ContainerData> elem){ return elem->ext_id==ext_id; });
    if (container == end(containers_list)) return nullptr;
    return *container;
}

auto ContainerRuntime::pause(std::string id) -> bool { return false; }
auto ContainerRuntime::resume(std::string id) -> bool { return false; }
auto ContainerRuntime::getState(std::string id) -> std::string { return "idle"; }
auto ContainerRuntime::getInfo(std::string id) -> nlohmann::json { 
    auto detail = container_adapter->detail(id);
    return detail;
}
