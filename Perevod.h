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

namespace asio = boost::asio;
using asio::ip::tcp;
using asio::ip::udp;

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
		
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data) {
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
			this->data = data;
		};

		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, int image_data_size) {
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
			this->data.resize(image_data_size);
			std::copy(data, data + image_data_size, this->data.begin());
		};

		~ImageFrame() {};

		uint32_t position_x() {
			return this->x;
		}

		uint32_t position_y() {
			return this->y;
		}

		std::vector<unsigned char> image_data() {
			return this->data;
		}

		unsigned char* image_raw_data() {
			return this->data.data();
		}

		int image_width() {
			return this->width;
		}

		int image_height() {
			return this->height;
		}
		
		void read_raw_byte(unsigned char *frame_data) {
			std::memcpy(frame_data, &this->x, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t), &this->y, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 2, &this->width, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 3, &this->height, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 4, this->image_raw_data(), this->data.size());
		}

		int size() {
			return this->data.size();
		}

		int frame_size() {
			return this->size() + sizeof(uint32_t) * 4;
		}
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
		~Queue() {};

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
		asio::streambuf receive_buffer;

		ImageSocketImpl(std::string ip_address, int send_port, int receive_port) {
			this->ip_address = ip_address;
			this->send_port = send_port;
			this->receive_port = receive_port;
		};
		~ImageSocketImpl() {};

		virtual void send_data(unsigned char *data, int size) = 0;

		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
			unsigned char *frame_data = new unsigned char[frame->frame_size()];
			frame->read_raw_byte(frame_data);
			this->send_data(frame_data, frame->frame_size());
			delete frame_data;
		}
	};

	class ImageSocketUDPImpl : public ImageSocketImpl
	{
		asio::io_service send_io_service;
		udp::socket send_socket;

		asio::io_service receive_io_service;
		udp::socket receive_socket;
		asio::streambuf receive_buffer;
		udp::endpoint destination;
	public:
		ImageSocketUDPImpl(std::string ip_address, int send_port, int receive_port) :  ImageSocketImpl(ip_address, send_port, receive_port), send_socket(send_io_service,udp::endpoint(udp::v4(), send_port)), receive_socket(receive_io_service, udp::endpoint(udp::v4(),
			receive_port)), destination(asio::ip::address::from_string(ip_address), send_port) {}

		void send_data(unsigned char *data, int size) {
			asio::socket_base::send_buffer_size buffer_size(size);
    		this->send_socket.set_option(buffer_size);
    		this->send_socket.send_to(asio::buffer(data, size), destination);
		}

		std::shared_ptr<Perevod::ImageFrame> read_frame() {
			return nullptr;
			// std::shared_ptr<Perevod::ImageFrame>frame;
			// boost::system::error_code error;

			// boost::array<char, 65537> recv_buf;
   //  		udp::endpoint sender_endpoint;
   //  		size_t len = socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);
			// asio::read(this->receive_socket, this->receive_buffer, asio::transfer_all(), error);
			// if (error && error != asio::error::eof) {
			// 	std::cout << error.message() << std::endl;
			// }
			// else {
			// 	const unsigned char *data = asio::buffer_cast<const unsigned char *>(this->receive_buffer.data());
			// 	uint32_t x, y;
			// 	std::memcpy(&x, data, sizeof(uint32_t));
			// 	std::memcpy(&y, data + sizeof(uint32_t), sizeof(uint32_t));

			// 	uint32_t width, height;
			// 	std::memcpy(&width, data + sizeof(uint32_t) * 2, sizeof(uint32_t));
			// 	std::memcpy(&height, data + sizeof(uint32_t) * 3, sizeof(uint32_t));
			// 	unsigned char *image_data = (unsigned char *)data + sizeof(uint32_t) * 4;

			// 	frame = std::make_shared<Perevod::ImageFrame>(ImageFrame(x, y, width, height, image_data));
			// }
    	}
	};

	class ImageSocketTCPImpl : public ImageSocketImpl
	{
		asio::io_service send_io_service;
		tcp::socket send_socket;

		asio::io_service receive_io_service;
		tcp::socket receive_socket;
		tcp::acceptor acceptor;
	public:
		ImageSocketTCPImpl(std::string ip_address, int send_port, int receive_port) :ImageSocketImpl(ip_address, send_port, receive_port), send_socket(this->send_io_service), receive_socket(this->receive_io_service), acceptor(this->receive_io_service, tcp::endpoint(tcp::v4(), receive_port)) {
			asio::socket_base::reuse_address option(true);
			this->acceptor.set_option(option);
			asio::socket_base::keep_alive keep_alive(true);
			this->acceptor.set_option(keep_alive);
		};
		~ImageSocketTCPImpl() {};
		
		void send_data(unsigned char *data, int size) {
			boost::system::error_code error;
			do {
				this->send_socket.connect(tcp::endpoint(asio::ip::address::from_string(this->ip_address), this->send_port), error);
			} while (error);
			PEREVOD_DEBUG_LOG("TCP connected");

			this->send_io_service.run();

			asio::write(this->send_socket, asio::buffer(data, size), error);
			this->send_socket.close();	
		}

		std::shared_ptr<Perevod::ImageFrame> read_frame() {
			this->acceptor.accept(this->receive_socket);
			
			std::shared_ptr<Perevod::ImageFrame>frame;
			boost::system::error_code error;
			asio::read(this->receive_socket, this->receive_buffer, asio::transfer_all(), error);
			if (error && error != asio::error::eof) {
				std::cout << error.message() << std::endl;
			}
			else {
				const unsigned char *data = asio::buffer_cast<const unsigned char *>(this->receive_buffer.data());
				uint32_t x, y;
				std::memcpy(&x, data, sizeof(uint32_t));
				std::memcpy(&y, data + sizeof(uint32_t), sizeof(uint32_t));

				uint32_t width, height;
				std::memcpy(&width, data + sizeof(uint32_t) * 2, sizeof(uint32_t));
				std::memcpy(&height, data + sizeof(uint32_t) * 3, sizeof(uint32_t));
				unsigned char *image_data = (unsigned char *)data + sizeof(uint32_t) * 4;

				frame = std::make_shared<Perevod::ImageFrame>(ImageFrame(x, y, width, height, image_data, this->receive_buffer.size() - sizeof(uint32_t) * 4));
			}
			this->receive_buffer.consume(this->receive_buffer.size());
			this->receive_socket.close();

			return frame;
		}
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
