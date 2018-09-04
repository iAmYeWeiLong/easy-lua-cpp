#include "context.h"

#include "../util.h"
#include "uv.h" //Ϊ��ʹ��uv_async_t
#include "blockQueue.h"
#include "message.h"
#include "serviceUtil.h"
#include "serviceInfo.h"

#include <lua.hpp>
#include <string>
#include <vector>
#include <map>
#include <cassert>

using namespace std;

int cVmContext::lastHandle_ = 0;

cVmContext::cVmContext(const std::shared_ptr<cServiceInfo> & serviceInfo, const std::shared_ptr<uv_async_t> & asyncWatcher) :
	service(serviceInfo), asyncWatcher_(asyncWatcher),bIdle(true)
{
	std::lock_guard<mutex> lock(this->mutex_);
	//����vm id
	//HandleManager<cVmContext> & mgr = Singleton<HandleManager<cVmContext>>::instance();
	this->handle_ = ++lastHandle_;
}

void cVmContext::pushMsg(const Message & msg) {//�ظ���Ϣ���.��Ϊָ����Ŀ��vm
	std::lock_guard<mutex> lock(this->mutex_);

	this->bIdle = false;
	this->msgQueue_.put(msg);
	uv_async_send(&*this->asyncWatcher_);//����ǿ��Կ��̵߳��õ�,���ǲ��ǿ��Զ���߳�ͬʱ������??
}

Message cVmContext::popMsg(){
	std::lock_guard<mutex> lock(this->mutex_);

	if (this->msgQueue_.size() != 0) {
		this->bIdle = false;
		return this->msgQueue_.take();//��Ϊ�����жϹ��ǿ���,��ֻ��һ��������,���Բ��ᷢ��û��Ԫ�ض��������������	
	}else {// context ��û����Ϣ,ȥ service ����
/*		auto p = this->service.lock();
		assert(p);	*/	
		Message msg = this->service->msgQueue.takeNonBlock();
		if (nullptr != msg.data) {
			this->bIdle = false;
			return msg;
		}
		else {
			this->bIdle = true;
			return Message();//��ʾû��
		}		
	}
}
bool cVmContext::isIdle(void) {
	std::lock_guard<mutex> lock(this->mutex_);

	return this->bIdle;
}
int cVmContext::newSession(void) {
	std::lock_guard<mutex> lock(this->mutex_);

	// session always be a positive number
	auto session = ++this->sessionId;
	if (session <= 0) {
		this->sessionId = 1;
		return 1;
	}
	return session;
}

unsigned int cVmContext::handle() {
	return this->handle_;
}

unsigned int cVmContext::serviceHandle() {
	return this->service->handle();
}

