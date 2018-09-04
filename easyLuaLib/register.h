#pragma once

#include <memory>
#include <map>
#include <mutex>

//using namespace std;
// 弱引用对象管理器.(对象生命周期不属这里管理)
template<typename keyType,typename valueType>
class Register:public virtual std::enable_shared_from_this<Register<keyType, valueType>>{
public:
	typedef std::map<keyType, std::weak_ptr<valueType>> mapType;
	//typedef mapType::value_type mapValueType; //报错
	//typedef std::map<keyType,valueType>::value_type mapValueType;//也报错
	
	std::shared_ptr<valueType> fetch(const keyType & key) {
		std::lock_guard<mutex> lock(this->mutex_);

		auto it = this->map_.find(key);
		if (it == this->map_.end()) {
			return nullptr;
		}
		return it->second.lock();
	}

	template <typename ... Args>
	std::shared_ptr<valueType> fetchOrCreate(const keyType & key, Args && ... args) {//Args ... args
		std::lock_guard<mutex> lock(this->mutex_);
/*
		auto it = this->map_.find(key);
		if (it == this->map_.end()) {
			std::shared_ptr<valueType> obj(new valueType(args ...));
			this->map_[key] = obj;
			return obj;
		}
		return it->second.lock();
*/
		auto ib = this->map_.lower_bound(key);
		if (ib != this->map_.end() && !(this->map_.key_comp()(key, ib->first))) {//找到了
			return ib->second.lock();
		}else{//找不到
			std::shared_ptr<valueType> obj(new valueType(args ...));  //std::forward<Args>(args)...
			//this->map_.insert(ib, mapValueType(key, obj)); #暂时实现不了,不知如何取得mapValueType,用下面这行吧
			this->map_[key] = obj;//效率低,没办法
			return obj;
		}
	}
	
	void checkIn(const std::shared_ptr<valueType> & value) {
		std::lock_guard<mutex> lock(this->mutex_);

		auto key = value->handle();
		auto ib = this->map_.lower_bound(key);
		if (ib != this->map_.end() && !(this->map_.key_comp()(key, ib->first))) {//找到了
			return;// ib->second.lock();
		}
		else {//找不到
			auto cb = std::bind(&Register::removeItemCallback, std::weak_ptr<Register>(shared_from_this()), std::placeholders::_1, key);
			value->addDtorCallback(cb);
			//shared_ptr<cServiceInfo> serviceInfo(new cServiceInfo(key));
			//this->map_.insert(ib, mapValueType(key, value));
			this->map_[key] = value;//性能差,暂时就这样.因为没有办法取得mapValueType
			//return serviceInfo;
		}
	}
	mapType & getItemsMap() {
		return this->map_;
	}
private:
	static void removeItemCallback(const std::weak_ptr<Register> & weakThis, nullptr_t * _, const keyType & key) {
		std::shared_ptr<Register> this_(weakThis.lock());
		if (this_) {
			this_->map_.erase(key);
		}
	}
	
	//std::map<keyType, std::weak_ptr<valueType>>  map_;
	
	std::mutex mutex_;
public://todo:临时改为公有的,因为serviceInfo要访问
	mapType map_;
};