#pragma once

#include <iostream>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

#define DEBUG
#ifdef PEREVOD_DEBUG_LOG(x) std::cout << x << std::endl;

namespace Perevod
{
	class ImageFrame
	{
	protected:
		uint32_t x, y, width, height;
		unsigned char *data;
	public:
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data) {
			this->x = x;
			this->y = y;
			this->width = width;
			this->height = height;
			int image_data_size = width * height * 3;
			this->data = new unsigned char [image_data_size];
			std::memcpy(this->data, data, image_data_size);
		};

		~ImageFrame() {
			delete this->data;
		};

		uint32_t position_x() {
			return this->x;
		}

		uint32_t position_y() {
			return this->y;
		}

		unsigned char* image_data() {
			return this->data;
		}

		int image_width() {
			return this->width;
		}

		int image_height() {
			return this->height;
		}
		
		void read_raw_byte(unsigned char *frame_data) {
			int image_data_size = width * height * 3;
			std::memcpy(frame_data, &this->x, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t), &this->y, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 2, &this->width, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 3, &this->height, sizeof(uint32_t));
			std::memcpy(frame_data + sizeof(uint32_t) * 4, this->data, image_data_size);
		}

		int size() {
			return this->width * this->height * 3;
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
		Queue() {};
		~Queue() {};

		void push(T data) {
			std::unique_lock<std::mutex> lock(this->mutex);
			this->queue.push(data);
			lock.unlock();
			this->cond.notify_one();
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

	class ImageSocketUDPImpl
	{
	public:
		ImageSocketUDPImpl() {};
		~ImageSocketUDPImpl() {};
		
		void send_frame(std::shared_ptr<Perevod::ImageFrame *>frame) {}

		std::shared_ptr<Perevod::ImageFrame *> read_frame() {
			return nullptr;
		}
	};

	class ImageSocketTCPImpl
	{
		asio::io_service send_io_service;
		tcp::socket send_socket;

		asio::io_service receive_io_service;
		tcp::socket receive_socket;
		asio::streambuf receive_buffer;
		tcp::acceptor acceptor;
		std::string ip_address;
		int send_port;
		int receive_port;
	public:
		ImageSocketTCPImpl(std::string ip_address, int send_port, int receive_port) : send_socket(this->send_io_service), receive_socket(this->receive_io_service), acceptor(this->receive_io_service, tcp::endpoint(tcp::v4(), receive_port)) {
			this->ip_address = ip_address;
			this->send_port = send_port;
			this->receive_port = receive_port;
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

			this->send_io_service.run();

			asio::write(this->send_socket, asio::buffer(data, size), error);
			this->send_socket.close();	
		}

		void send_frame(std::shared_ptr<Perevod::ImageFrame *>frame) {
			unsigned char *frame_data = new unsigned char[(*frame)->image_width() * (*frame)->image_height() * 3 + sizeof(uint32_t) * 4];
			(*frame)->read_raw_byte(frame_data);
			this->send_data(frame_data, (*frame)->frame_size());
			delete frame_data;
		}

		std::shared_ptr<Perevod::ImageFrame *> read_frame() {
			this->acceptor.accept(this->receive_socket);
			
			std::shared_ptr<Perevod::ImageFrame *>frame = nullptr;
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

				frame = std::make_shared<Perevod::ImageFrame *>(new ImageFrame(x, y, width, height, image_data));
			}
			this->receive_buffer.consume(this->receive_buffer.size());
			this->receive_socket.close();

			return frame;
		}
	};

	typedef enum {
		ImageSocketModeTCP,
		ImageSocketModeUDP
	} ImageSocketMode;

	class ImageSocket
	{
		Queue<std::shared_ptr<Perevod::ImageFrame *>> send_queue;
		Queue<std::shared_ptr<Perevod::ImageFrame *>> received_queue;

		ImageSocketMode mode;
		ImageSocketTCPImpl *tcp_impl;
		ImageSocketUDPImpl *upd_impl;
	public:
		bool suspend_cast_loop;
		bool suspend_receive_loop;
		std::function<void(std::shared_ptr<Perevod::ImageFrame *>)> receive_handler;

		ImageSocket(std::string ip_address, int send_port, int receive_port, ImageSocketMode mode) : mode(mode) {
			this->tcp_impl = nullptr;
			this->upd_impl = nullptr;

			if (mode == ImageSocketModeTCP) {
				PEREVOD_DEBUG_LOG("TCP");
				this->tcp_impl = new ImageSocketTCPImpl(ip_address, send_port, receive_port);
			}
			else {
				PEREVOD_DEBUG_LOG("UDP");
				this->upd_impl = new ImageSocketUDPImpl();
			}
			
			this->receive_handler = nullptr;
			this->suspend_cast_loop = true;
			this->suspend_receive_loop = true;
		}

		~ImageSocket() {
			delete this->tcp_impl;
			delete this->upd_impl;
		};

		void push_frame(std::shared_ptr<Perevod::ImageFrame *>frame) {
			this->send_queue.push(frame);
		}

		std::shared_ptr<Perevod::ImageFrame *> pop_frame() {
			return this->received_queue.try_pop();
		}

		void send_frame(std::shared_ptr<Perevod::ImageFrame *>frame) {
			this->mode == ImageSocketModeTCP ? this->tcp_impl->send_frame(frame) : this->upd_impl->send_frame(frame);
		}

		std::shared_ptr<Perevod::ImageFrame *> read_frame() {
			std::shared_ptr<Perevod::ImageFrame *>frame = this->mode == ImageSocketModeTCP ? this->tcp_impl->read_frame() : this->upd_impl->read_frame();

			return frame;
		}

		void run_cast_loop() {
			this->suspend_cast_loop = false;
			while (!this->suspend_cast_loop) {
				std::shared_ptr<Perevod::ImageFrame *>frame = this->send_queue.try_pop();
				if (frame) {
					this->send_frame(frame);
				}
			}
		}

		void run_receive_loop() {
			this->suspend_receive_loop = false;
			while (!this->suspend_receive_loop) {
				std::shared_ptr<Perevod::ImageFrame *>frame = this->read_frame();
				if (this->receive_handler) {
					this->receive_handler(frame);
				}
				else {
					this->received_queue.push(frame);
				}
			}
		}
	};
}
