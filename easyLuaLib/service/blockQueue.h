#pragma once

#include <mutex>
#include <deque>
#include <condition_variable>

#ifndef __GXX_EXPERIMENTAL_CXX0X__
#define __GXX_EXPERIMENTAL_CXX0X__
#endif

using namespace std;


template<typename T>
class cBlockQueue{
	public:

	void put(const T& x){
		unique_lock<mutex> lock(mutex_);
		queue_.push_back(x);
		cv_.notify_one();
	}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
	void put(T&& x){
		unique_lock<mutex> lock(mutex_);
		queue_.push_back(std::move(x));
		cv_.notify_one();
	}
#endif

	T takeNonBlock(){//非阻塞获取
		unique_lock<mutex> lock(mutex_);		
		if(queue_.empty())
			return T();	
		
		

#ifdef __GXX_EXPERIMENTAL_CXX0X__
		T front(std::move(queue_.front()));
#else
		T front(queue_.front());
#endif
		queue_.pop_front();
		return front;
	}
			
	T take(){//阻塞
		unique_lock<mutex> lock(mutex_);
		cv_.wait(lock, [this](){return !queue_.empty();});
#ifdef __GXX_EXPERIMENTAL_CXX0X__
		T front(std::move(queue_.front()));
#else
		T front(queue_.front());
#endif
		queue_.pop_front();
		return front;
	}

	T takeTimeOut(int iMilliSecond){//阻塞,带超时
		unique_lock<mutex> lock(mutex_);
		if(!cv_.wait_for(lock, std::chrono::milliseconds(iMilliSecond), [this](){return !queue_.empty();}))				
			return T();//nullptr;//超时返回		

#ifdef __GXX_EXPERIMENTAL_CXX0X__
		T front(std::move(queue_.front()));
#else
		T front(queue_.front());
#endif
		this->queue_.pop_front();
		return front;
	}
	
	size_t size() const{
		unique_lock<mutex> lock(this->mutex_);
		return this->queue_.size();
	}

private:	
	mutable std::mutex mutex_;
	std::deque<T> queue_;
	std::condition_variable cv_;
};


/* 暂时用不上
class cBlockBool {
public:
	void notify() {
		unique_lock<mutex> lock(mutex_);
		flag = true;
		cv_.notify_one();
	}

	void wait() {//阻塞
		unique_lock<mutex> lock(mutex_);
		cv_.wait(lock, [this]() {return flag;});
		flag = false;
	}

private:
	mutable mutex mutex_;
	bool flag;
	std::condition_variable cv_;
};
*/