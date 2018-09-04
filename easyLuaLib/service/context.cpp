#include "context.h"

#include "../util.h"
#include "uv.h" //为了使用uv_async_t
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
	//生成vm id
	//HandleManager<cVmContext> & mgr = Singleton<HandleManager<cVmContext>>::instance();
	this->handle_ = ++lastHandle_;
}

void cVmContext::pushMsg(const Message & msg) {//回复消息类的.因为指定了目标vm
	std::lock_guard<mutex> lock(this->mutex_);

	this->bIdle = false;
	this->msgQueue_.put(msg);
	uv_async_send(&*this->asyncWatcher_);//这个是可以跨线程调用的,但是不是可以多个线程同时调用呢??
}

Message cVmContext::popMsg(){
	std::lock_guard<mutex> lock(this->mutex_);

	if (this->msgQueue_.size() != 0) {
		this->bIdle = false;
		return this->msgQueue_.take();//因为上面判断过非空了,且只有一个消费者,绝对不会发生没有元素而陷入阻塞的情况	
	}else {// context 中没有消息,去 service 上拿
/*		auto p = this->service.lock();
		assert(p);	*/	
		Message msg = this->service->msgQueue.takeNonBlock();
		if (nullptr != msg.data) {
			this->bIdle = false;
			return msg;
		}
		else {
			this->bIdle = true;
			return Message();//表示没有
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

