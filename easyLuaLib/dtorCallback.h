#pragma once
#include <map>
#include <memory>
#include <functional>

//����д����
//using namespace std;
//template<typename T> class function;


class DtorCallback{
public:
	DtorCallback():nextId_(0){
	}

	int addDtorCallback(const std::function<void(nullptr_t*)> & f) {
		std::shared_ptr<nullptr_t> p(nullptr, f);
		//���Ա���,ÿһ��p���ǵȼ۵�,��Ϊ����ָ��nullptr,����f��Ӱ��
		//����Ҫ��
		
		++this->nextId_;
		this->pointerMap_[this->nextId_] = p;//������ܲ�,����Ҫ��
		return this->nextId_;
	}
	void removeDtorCallback(int id) {
		this->pointerMap_.erase(id);
	}


private:
	int nextId_;
	std::map<int,std::shared_ptr<std::nullptr_t>> pointerMap_;

};