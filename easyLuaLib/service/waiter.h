#pragma once

#include <mutex> 
#include <condition_variable>

class Waiter {
public:
	Waiter() :
		quitFlag_(false),
		vmId_(0),
		serviceId_(0)
	{
	}
	void wait() {
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->cv_.wait(lock, [&]() {return this->quitFlag_; });//阻塞在这里等待
	}
	void notify(unsigned int vmId_, unsigned int serviceId_) {
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->quitFlag_ = true;
		this->vmId_ = vmId_;
		this->serviceId_ = serviceId_;
		this->cv_.notify_all();
	}

	unsigned int getVmId() {
		return this->vmId_;
	}

	unsigned int getServiceId() {
		return this->serviceId_;
	}

private:
	unsigned int serviceId_, vmId_;
	bool quitFlag_;
	std::mutex mutex_;
	std::condition_variable cv_;
};