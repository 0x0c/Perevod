#pragma once

#include <iostream>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#ifdef __PEREVOD_DEBUG_MODE__
#define PEREVOD_DEBUG_LOG(x) std::cout << "[Perevod] " << x << std::endl;
#define PEREVOD_DEBUG_PRETTY_LOG(x) std::cout << "[Perevod] " << __PRETTY_FUNCTION__ << " " << x << std::endl;
#else
#define PEREVOD_DEBUG_LOG(x)
#define PEREVOD_DEBUG_PRETTY_LOG(x)
#endif

namespace Perevod
{
	class ImageFrame
	{
	protected:
		uint32_t x, y, width, height;
		std::vector<unsigned char> data;
	public:
		
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data);
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, int image_data_size);

		uint32_t position_x();
		uint32_t position_y();
		std::vector<unsigned char> image_data();
		unsigned char* image_raw_data();
		int image_width();
		int image_height();
		void read_raw_byte(unsigned char *frame_data);
		int size();
		int frame_size();
	};

	template <typename T> class Queue
	{
		std::queue<T> queue;
		mutable std::mutex mutex;
		std::condition_variable	cond;
	public:
		int limit;

		Queue() {
			this->limit = 0;
		};
	
		bool push(T data) {
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

		T pop() {
			std::unique_lock<std::mutex> lock(this->mutex);
			while (this->queue.empty()) {
				this->cond.wait(lock);
		    }
			auto i = this->queue.front();
			this->queue.pop();

			return i;
		}

		T try_pop() {
			std::unique_lock<std::mutex> lock(this->mutex);
			if (this->queue.empty()) {
				return nullptr;
			}
			auto i = this->queue.front();
			this->queue.pop();
			return i;
		}

		bool empty() const {
			std::unique_lock<std::mutex> lock(this->mutex);
			return this->queue.empty();
		}
	};

	class ImageSocketImpl
	{
	public:
		std::string ip_address;
		int send_port;
		int receive_port;
		Queue<std::shared_ptr<Perevod::ImageFrame>> send_queue;
		Queue<std::shared_ptr<Perevod::ImageFrame>> received_queue;
		boost::asio::streambuf receive_buffer;

		ImageSocketImpl(std::string ip_address, int send_port, int receive_port);

		virtual void send_data(unsigned char *data, int size) = 0;
		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame);
	};

	class ImageSocketTCPImpl : public ImageSocketImpl
	{
		boost::asio::io_service send_io_service;
		boost::asio::ip::tcp::socket send_socket;
		
		boost::asio::io_service receive_io_service;
		boost::asio::ip::tcp::socket receive_socket;
		boost::asio::ip::tcp::acceptor acceptor;
	public:
		ImageSocketTCPImpl(std::string ip_address, int send_port, int receive_port);

		void send_data(unsigned char *data, int size);
		std::shared_ptr<Perevod::ImageFrame> read_frame();
	};

	template <typename T> class ImageSocket
	{
		T *impl;
	public:
		bool suspend_cast_loop;
		bool suspend_receive_loop;
		std::function<void(std::shared_ptr<Perevod::ImageFrame>)> receive_handler;
		long long cast_interval;

		void set_cast_queue_limit(int limit) { this->impl->send_queue.limit = limit; };
		void set_receive_queue_limit(int limit) { this->impl->receive_queue.limit = limit; };

		ImageSocket(std::string ip_address, int send_port, int receive_port) {
			this->impl = new T(ip_address, send_port, receive_port);
			this->receive_handler = nullptr;
			this->suspend_cast_loop = true;
			this->suspend_receive_loop = true;
		}

		~ImageSocket() {
			delete this->impl;
		}

		void push_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
			std::function<void()> push = [this, frame] {
				PEREVOD_DEBUG_LOG("push_frame");
				bool limit = this->impl->send_queue.push(frame);
				if (limit) {
					PEREVOD_DEBUG_LOG("send queue limit.");
				}
			};
			if (this->cast_interval > 0) {
				std::async(std::launch::async, [this, &push] {
					std::this_thread::sleep_for(std::chrono::milliseconds(this->cast_interval));
					push();
				});
			}
			else {
				push();
			}
		}

		std::shared_ptr<Perevod::ImageFrame> pop_frame() {
			PEREVOD_DEBUG_LOG("pop_frame");
			return this->impl->received_queue.try_pop();
		}

		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
			PEREVOD_DEBUG_LOG("send_frame");
			this->impl->send_frame(frame);
		}

		std::shared_ptr<Perevod::ImageFrame> read_frame() {
			PEREVOD_DEBUG_LOG("read_frame");
			std::shared_ptr<Perevod::ImageFrame>frame = this->impl->read_frame();
			return frame;
		}

		void run_cast_loop() {
			PEREVOD_DEBUG_LOG("run_cast_loop");
			this->suspend_cast_loop = false;
			while (!this->suspend_cast_loop) {
				std::shared_ptr<Perevod::ImageFrame>frame = this->impl->send_queue.pop();
				this->send_frame(frame);
			}
		}

		void run_receive_loop() {
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
	};
}
