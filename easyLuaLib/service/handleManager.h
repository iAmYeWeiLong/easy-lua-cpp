//#include "skynet.h"
//#include "skynet_server.h"

//#include "skynet_handle.h"
//#include "serviceUtil.h"
//#include "context.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <mutex>
#include <memory>

#include <stdint.h>

// reserve high 8 bits for remote id
//#define HANDLE_MASK 0xffffff
//#define HANDLE_REMOTE_SHIFT 24

#define DEFAULT_SLOT_SIZE 4
#define MAX_SLOT_SIZE 0x40000000

class handle_name {
public:
	char * name;
	uint32_t handle;
};

template<typename T>
class HandleManager{
private:
	std::mutex mutex_;

	//uint32_t harbor;
	uint32_t handle_index;
	int slot_size;	
	std::weak_ptr<T> * slot;
		
	int name_cap;
	int name_count;
	handle_name * nameArray;

public:
	HandleManager() {
		this->slot_size = DEFAULT_SLOT_SIZE;		
		this->slot = new std::weak_ptr<T>[this->slot_size];
		
		// reserve 0 for system
		//this->harbor = (uint32_t)(harbor & 0xff) << HANDLE_REMOTE_SHIFT;
		this->handle_index = 1;
		this->name_cap = 2;
		this->name_count = 0;
		this->nameArray = new handle_name[this->name_cap];
	}

	uint32_t checkIn(const std::shared_ptr<T> & ctx) {	
		//rwlock_wlock(&this->mutex_);
		std::lock_guard<std::mutex> lock(this->mutex_);
	
		for (;;) {
			int i;
			for (i=0;i<this->slot_size;i++) {
				auto handle = (i + this->handle_index);// &HANDLE_MASK;
				auto hash = handle & (this->slot_size-1);
				if (!this->slot[hash].lock()) {
					this->slot[hash] = ctx;
					this->handle_index = handle + 1;
					//rwlock_wunlock(&this->mutex_);
					//handle |= this->harbor;
					return handle;
				}
			}
			//assert((this->slot_size*2 - 1) <= HANDLE_MASK);			
			auto new_slot = new std::weak_ptr<T>[this->slot_size * 2];
			for (i=0;i<this->slot_size;i++) {
				auto hash = this->slot[i].lock()->handle() & (this->slot_size * 2 - 1);
				assert(!new_slot[hash].lock());
				new_slot[hash] = this->slot[i];
			}		
			delete[] this->slot;
			this->slot = new_slot;
			this->slot_size *= 2;
		}
	}

	int retire(uint32_t handle) {
		auto ret = 0;
		//rwlock_wlock(&this->mutex_);
		std::lock_guard<std::mutex> lock(this->mutex_);

		auto hash = handle & (this->slot_size-1);
		auto ctx = this->slot[hash].lock();

		if (ctx && ctx->handle() == handle) {
			this->slot[hash].reset();
			ret = 1;
			int i;
			auto j=0, n=this->name_count;
			for (i=0; i<n; ++i) {
				if (this->nameArray[i].handle == handle) {
					delete []this->nameArray[i].name;
					continue;
				} else if (i!=j) {
					this->nameArray[j] = this->nameArray[i];
				}
				++j;
			}
			this->name_count = j;
		} else {
			ctx.reset();
		}
		//rwlock_wunlock(&this->mutex_);
		if (ctx) {
			// release ctx may call skynet_handle_* , so wunlock first.
			//skynet_context_release(ctx); //ctx的生命期不归这里管
		}
		return ret;
	}

	void retireAll() {	
		for (;;) {
			auto n=0;
			int i;
			for (i=0;i<this->slot_size;i++) {
				uint32_t handle = 0;
				{
					std::lock_guard<std::mutex> lock(this->mutex_);
					auto ctx = this->slot[i].lock();
					if (ctx) {
						handle = ctx->handle();
					}
				}
				if (handle != 0) {
					if (retire(handle)) {
						++n;
					}
				}
			}
			if (n==0)
				return;
		}
	}

	std::shared_ptr<T> grab(uint32_t handle) {
		std::lock_guard<std::mutex> lock(this->mutex_);
		auto hash = handle & (this->slot_size-1);
		auto ctx = this->slot[hash].lock();
		if (ctx && ctx->handle() == handle) {
			return ctx;
		}
		return nullptr;
	}

	uint32_t findName(const char * name) {
		std::lock_guard<std::mutex> lock(this->mutex_);
		uint32_t handle = 0;
		auto begin = 0;
		auto end = this->name_count - 1;
		while (begin<=end) {
			int mid = (begin+end)/2;
			handle_name *n = &this->nameArray[mid];
			int c = strcmp(n->name, name);
			if (c==0) {
				handle = n->handle;
				break;
			}
			if (c<0) {
				begin = mid + 1;
			} else {
				end = mid - 1;
			}
		}
		return handle;
	}

	const char * nameHandle(uint32_t handle, const char *name) {		
		std::lock_guard<std::mutex> lock(this->mutex_);
		auto begin = 0;
		auto end = this->name_count - 1;
		while (begin <= end) {
			auto mid = (begin + end) / 2;
			handle_name *n = &this->nameArray[mid];
			auto c = strcmp(n->name, name);
			if (c == 0) {
				return nullptr;
			}
			if (c<0) {
				begin = mid + 1;
			}
			else {
				end = mid - 1;
			}
		}
		auto result = skynet_strdup(name);
		_insert_name_before(result, handle, begin);
		return result;
	}

private:

	void _insert_name_before(char *name, uint32_t handle, int before) {
		if (this->name_count >= this->name_cap) {
			this->name_cap *= 2;
			assert(this->name_cap <= MAX_SLOT_SIZE);
			handle_name * n = new handle_name[this->name_cap];
			int i;
			for (i=0;i<before;i++) {
				n[i] = this->nameArray[i];
			}
			for (i=before;i<this->name_count;i++) {
				n[i+1] = this->nameArray[i];
			}
			delete []this->nameArray;
			this->nameArray = n;
		} else {
			int i;
			for (i=this->name_count;i>before;i--) {
				this->nameArray[i] = this->nameArray[i-1];
			}
		}
		this->nameArray[before].name = name;
		this->nameArray[before].handle = handle;
		this->name_count ++;
	}

	//在skynet中,此函数并不是放在这里的
	char * skynet_strdup(const char *str) {
		auto sz = strlen(str);
		auto ret = new char[sz + 1];
		memcpy(ret, str, sz + 1);
		return ret;
	}
};