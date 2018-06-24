#pragma once
#include "../register.h"

#include "message.h"
#include "blockQueue.h"
#include "../dtorCallback.h"

#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <string>

//template<typename keyType, typename valueType>class Register; 报错:不完全类型,只能include

class cVmContext;

//using namespace std;

//service相关信息
class cServiceInfo : virtual public DtorCallback, virtual public Register<int, cVmContext> {
public:
	cServiceInfo(const std::string &);
		
	void pushMsg2FreeVm(const Message & msg);//投递消息到空闲的vm(发布工作给服务时,多个同质的vm,谁有空就给谁,大家都忙时则放到本队列)

	//void pushMsg2VmByVmId() {//根据vm id投递消息(回复消息给这个vm时)
	//}
	cBlockQueue<Message> msgQueue; //消息队列
		
	unsigned int handle();
	size_t getVmCount();
	std::string & serviceName();

private:

	std::string serviceName_; //本服务的名字
	unsigned int handle_;//
		
	std::mutex mutex_;

	static int lastHandle_;
};
