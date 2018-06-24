#pragma once

#include "uv.h" //为了使用uv_async_t
#include "blockQueue.h"
#include "../dtorCallback.h"
#include "message.h"

#include <memory>
#include <mutex>
#include <vector>

class cServiceInfo;
//vm的上下文
class cVmContext : public virtual std::enable_shared_from_this<cVmContext>, virtual public DtorCallback {
public:
	cVmContext(const std::shared_ptr<cServiceInfo> &, const std::shared_ptr<uv_async_t> &);
	void pushMsg(const Message & msg);//回复消息类的.因为指定了目标vm

	Message popMsg();
	bool isIdle(void);
	int newSession(void);
			
	unsigned int serviceHandle();
	unsigned int handle();

	std::shared_ptr<cServiceInfo> service;//所属服务
private:
	unsigned int handle_;//
	int sessionId;
	bool bIdle;
	
	cBlockQueue<Message> msgQueue_; //消息队列(有父消息队列,本队列找不到时就向父队列请求工作)	
	std::shared_ptr<uv_async_t> asyncWatcher_;
	std::mutex mutex_;

	static int lastHandle_;
};
