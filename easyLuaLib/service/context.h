#pragma once

#include "uv.h" //Ϊ��ʹ��uv_async_t
#include "blockQueue.h"
#include "../dtorCallback.h"
#include "message.h"

#include <memory>
#include <mutex>
#include <vector>

class cServiceInfo;
//vm��������
class cVmContext : public virtual std::enable_shared_from_this<cVmContext>, virtual public DtorCallback {
public:
	cVmContext(const std::shared_ptr<cServiceInfo> &, const std::shared_ptr<uv_async_t> &);
	void pushMsg(const Message & msg);//�ظ���Ϣ���.��Ϊָ����Ŀ��vm

	Message popMsg();
	bool isIdle(void);
	int newSession(void);
			
	unsigned int serviceHandle();
	unsigned int handle();

	std::shared_ptr<cServiceInfo> service;//��������
private:
	unsigned int handle_;//
	int sessionId;
	bool bIdle;
	
	cBlockQueue<Message> msgQueue_; //��Ϣ����(�и���Ϣ����,�������Ҳ���ʱ���򸸶���������)	
	std::shared_ptr<uv_async_t> asyncWatcher_;
	std::mutex mutex_;

	static int lastHandle_;
};
