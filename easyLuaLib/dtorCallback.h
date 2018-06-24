#pragma once
#include <map>
#include <memory>
#include <functional>

//不会写声明
//using namespace std;
//template<typename T> class function;


class DtorCallback{
public:
	DtorCallback():nextId_(0){
	}

	int addDtorCallback(const std::function<void(nullptr_t*)> & f) {
		std::shared_ptr<nullptr_t> p(nullptr, f);
		//测试表明,每一个p都是等价的,因为都是指向nullptr,不受f的影响
		//后期要加
		
		++this->nextId_;
		this->pointerMap_[this->nextId_] = p;//这个性能差,后期要改
		return this->nextId_;
	}
	void removeDtorCallback(int id) {
		this->pointerMap_.erase(id);
	}


private:
	int nextId_;
	std::map<int,std::shared_ptr<std::nullptr_t>> pointerMap_;

};