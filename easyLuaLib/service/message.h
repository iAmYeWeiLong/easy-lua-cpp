#pragma once
#include <memory>
#include <string>

class Message {
public:
	Message() :
		sourceVm(0),
		session(0),
		data(nullptr),
		sz(0)
	{
	
	}

	Message(const std::string & sourceServiceName,unsigned int sourceVm, int session, const std::shared_ptr<uint8_t> & data, size_t sz):
		sourceServiceName(sourceServiceName),
		sourceVm(sourceVm),
		session(session),
		data(data),
		sz(sz)
	{

	}

	bool invalid() {
		return data == nullptr;
	}
	std::string sourceServiceName;
	unsigned int sourceVm;
	int session;
	std::shared_ptr<uint8_t> data;
	size_t sz;//消息长度,高8位是消息type
};