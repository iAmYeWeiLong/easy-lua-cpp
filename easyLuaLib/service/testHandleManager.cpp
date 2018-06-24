//#include "handleManager.h"
////
//#include "serviceUtil.h"
//#include "message.h"
//#include "context.h"
//
//
#include "serviceInfo.h"
//#include <memory>

//#include "../dtorCallback.h"
//
//#include "../register.h"

void test() {
	/*std::shared_ptr<Register<int, cVmContext>> contexRegister(new Register<int, cVmContext>());

	std::shared_ptr<cServiceInfo> service(new cServiceInfo("ff"));
	std::shared_ptr<cVmContext> context(new cVmContext(service));

	contexRegister->fetchOrCreate(1121, service);
	contexRegister->checkIn(context);
	auto fuck = contexRegister->fetch(context->handle());
	printf("fuck name =%d\n", fuck->serviceHandle());
	*/
	/*
	shared_ptr<cServiceInfo> serviceInfo (new cServiceInfo("fff"));
	HandleManager<cServiceInfo> x();
	x.registerObj(serviceInfo);
	x.retire(1);
	x.retireAll();
	x.grab(2);
	x.findName("fada");
	x.nameHandle(212, "asdfa");
	*/
}