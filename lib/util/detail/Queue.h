#include "../Queue.h"

namespace Perevod
{
	template <typename T> Queue<T>::Queue() {
		this->limit = 0;
	}
	
	template <typename T> bool Queue<T>::push(T data) {
		std::unique_lock<std::mutex> lock(this->mutex);
		bool limit = false;
		if (this->limit > 0 && this->limit < this->queue.size() + 1) {
			limit = true;
			this->queue.pop();
		}
		this->queue.push(data);
		lock.unlock();
		this->cond.notify_one();

		return limit;
	}

	template <typename T> T Queue<T>::pop() {
		std::unique_lock<std::mutex> lock(this->mutex);
		while (this->queue.empty()) {
			this->cond.wait(lock);
	    }
		auto i = this->queue.front();
		this->queue.pop();

		return i;
	}

	template <typename T> T Queue<T>::try_pop() {
		std::unique_lock<std::mutex> lock(this->mutex);
		if (this->queue.empty()) {
			return nullptr;
		}
		auto i = this->queue.front();
		this->queue.pop();
		return i;
	}

	template <typename T> bool Queue<T>::empty() const {
		std::unique_lock<std::mutex> lock(this->mutex);
		return this->queue.empty();
	}
}
