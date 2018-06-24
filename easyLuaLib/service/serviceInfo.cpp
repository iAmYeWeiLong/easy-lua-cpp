#include "serviceInfo.h"

#include "serviceUtil.h"
#include "message.h"
#include "context.h"

#include <functional>
#include <memory>
#include <string>
#include <cassert>

using namespace std;

int cServiceInfo::lastHandle_ = 0;
cServiceInfo::cServiceInfo(const std::string & name):serviceName_(name){
	std::lock_guard<mutex> lock(this->mutex_);
	this->handle_ = ++lastHandle_;
}

/*
void cServiceInfo::addAssociatedVm(const shared_ptr<cVmContext> & vm) {//新增一个worker,即是vm
	std::lock_guard<mutex> lock(this->mutex_);
	this->map_[vm->handle()] = vm;
		
	auto this_ = std::enable_shared_from_this<cServiceInfo>::shared_from_this();//一定要这样写,不然有二义性

	auto cb = std::bind(&cServiceInfo::removeVmContext, std::weak_ptr<cServiceInfo>(this_),std::placeholders::_1,vm->handle());
	//vm->addDtorCallback(cb);
}
*/

//static
/*
void cServiceInfo::removeVmContext(const std::weak_ptr<cServiceInfo> & wkServiceInfo,nullptr_t* _,int vmId) {
	std::shared_ptr<cServiceInfo> _this(wkServiceInfo.lock());
	if (_this){
		_this->map_.erase(vmId);
	}
}
*/
void cServiceInfo::pushMsg2FreeVm(const Message & msg) {//投递消息到空闲的vm(发布工作给服务时,多个同质的vm,谁有空就给谁,大家都忙时则放到本队列)
	std::lock_guard<mutex> lock(this->mutex_);
	//如果只有1个vm,直接放在这个vm里面

	shared_ptr<cVmContext> pIdle;
	for (auto vm : this->map_) {
		auto vmId = vm.first;
		auto vmPtr = vm.second;
		auto vm_shared_ptr = vmPtr.lock();
		assert(vm_shared_ptr);
		if (vm_shared_ptr->isIdle()) {
			pIdle = vm_shared_ptr;
			break;
		}
	}
	if (pIdle) {
		pIdle->pushMsg(msg);
	}else{
		this->msgQueue.put(msg);
	}
}

unsigned int cServiceInfo::handle() {
	return this->handle_;
}
std::string & cServiceInfo::serviceName() {
	return this->serviceName_;
}
size_t cServiceInfo::getVmCount() {
	return this->map_.size();
}
