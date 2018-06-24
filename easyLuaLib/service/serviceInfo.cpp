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
void cServiceInfo::addAssociatedVm(const shared_ptr<cVmContext> & vm) {//����һ��worker,����vm
	std::lock_guard<mutex> lock(this->mutex_);
	this->map_[vm->handle()] = vm;
		
	auto this_ = std::enable_shared_from_this<cServiceInfo>::shared_from_this();//һ��Ҫ����д,��Ȼ�ж�����

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
void cServiceInfo::pushMsg2FreeVm(const Message & msg) {//Ͷ����Ϣ�����е�vm(��������������ʱ,���ͬ�ʵ�vm,˭�пվ͸�˭,��Ҷ�æʱ��ŵ�������)
	std::lock_guard<mutex> lock(this->mutex_);
	//���ֻ��1��vm,ֱ�ӷ������vm����

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
