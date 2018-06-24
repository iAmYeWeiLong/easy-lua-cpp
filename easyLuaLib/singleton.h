#pragma once

#include <memory>
#include <mutex> //std::call_once

template<typename T>
class Singleton{
public:
	//todo:透明传参给构造函数
	static T& instance(){
		std::call_once(once_, &Singleton::init);
		return *value_;
	}

private:
	Singleton();
	~Singleton();

	static void init(){
		value_.reset(new T());
	}

private:
	static std::once_flag once_;
	static std::shared_ptr<T> value_;
};

template<typename T>
std::once_flag Singleton<T>::once_;

template<typename T>
std::shared_ptr<T> Singleton<T>::value_;