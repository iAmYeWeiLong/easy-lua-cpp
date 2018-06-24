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

//template<typename keyType, typename valueType>class Register; ����:����ȫ����,ֻ��include

class cVmContext;

//using namespace std;

//service�����Ϣ
class cServiceInfo : virtual public DtorCallback, virtual public Register<int, cVmContext> {
public:
	cServiceInfo(const std::string &);
		
	void pushMsg2FreeVm(const Message & msg);//Ͷ����Ϣ�����е�vm(��������������ʱ,���ͬ�ʵ�vm,˭�пվ͸�˭,��Ҷ�æʱ��ŵ�������)

	//void pushMsg2VmByVmId() {//����vm idͶ����Ϣ(�ظ���Ϣ�����vmʱ)
	//}
	cBlockQueue<Message> msgQueue; //��Ϣ����
		
	unsigned int handle();
	size_t getVmCount();
	std::string & serviceName();

private:

	std::string serviceName_; //�����������
	unsigned int handle_;//
		
	std::mutex mutex_;

	static int lastHandle_;
};
