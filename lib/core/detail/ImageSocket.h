#include "../../util/macro.h"
#include "../ImageSocket.h"
#include <thread>
#include <future>
#ifndef _WIN32
#include <chrono>
#endif

namespace Perevod
{
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
		return this->impl->receive_queue.try_pop();
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
				bool limit = this->impl->receive_queue.push(frame);
				if (limit) {
					PEREVOD_DEBUG_LOG("received queue limit.");
				}
			}
		}
	}
}
