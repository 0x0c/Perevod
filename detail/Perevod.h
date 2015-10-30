#include "../Perevod.h"

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
	
	template <typename T> ImageSocket<T>::ImageSocket(std::string ip_address, int send_port, int receive_port) {
		this->impl = new T(ip_address, send_port, receive_port);
		this->receive_handler = nullptr;
		this->suspend_cast_loop = true;
		this->suspend_receive_loop = true;
	}

	template <typename T> ImageSocket<T>::~ImageSocket() {
		delete this->impl;
	}

	template <typename T> void ImageSocket<T>::set_cast_queue_limit(int limit) {
		this->impl->send_queue.limit = limit;
	}

	template <typename T> void ImageSocket<T>::set_receive_queue_limit(int limit) {
		this->impl->receive_queue.limit = limit;
	}

	template <typename T> void ImageSocket<T>::push_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
		std::function<void()> push = [this, frame] {
			PEREVOD_DEBUG_LOG("push_frame");
			bool limit = this->impl->send_queue.push(frame);
			if (limit) {
				PEREVOD_DEBUG_LOG("send queue limit.");
			}
		};
		#ifndef _WIN32
		if (this->cast_interval > 0) {
			std::async(std::launch::async, [this, &push] {
				std::this_thread::sleep_for(std::chrono::milliseconds(this->cast_interval));
				push();
			});
		}
		else {
			push();
		}
		#else
		push();
		#endif
	}

	template <typename T> std::shared_ptr<Perevod::ImageFrame> ImageSocket<T>::pop_frame() {
		PEREVOD_DEBUG_LOG("pop_frame");
		return this->impl->received_queue.try_pop();
	}

	template <typename T> void ImageSocket<T>::send_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
		PEREVOD_DEBUG_LOG("send_frame");
		this->impl->send_frame(frame);
	}

	template <typename T> std::shared_ptr<Perevod::ImageFrame> ImageSocket<T>::read_frame() {
		PEREVOD_DEBUG_LOG("read_frame");
		std::shared_ptr<Perevod::ImageFrame>frame = this->impl->read_frame();
		return frame;
	}

	template <typename T> void ImageSocket<T>::run_cast_loop() {
		PEREVOD_DEBUG_LOG("run_cast_loop");
		this->suspend_cast_loop = false;
		while (!this->suspend_cast_loop) {
			std::shared_ptr<Perevod::ImageFrame>frame = this->impl->send_queue.pop();
			this->send_frame(frame);
		}
	}

	template <typename T> void ImageSocket<T>::run_receive_loop() {
		PEREVOD_DEBUG_LOG("run_receive_loop");
		this->suspend_receive_loop = false;
		while (!this->suspend_receive_loop) {
			std::shared_ptr<Perevod::ImageFrame>frame = this->read_frame();
			if (this->receive_handler) {
				this->receive_handler(frame);
			}
			else {
				bool limit = this->impl->received_queue.push(frame);
				if (limit) {
					PEREVOD_DEBUG_LOG("received queue limit.");
				}
			}
		}
	}
}
