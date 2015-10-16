#pragma once

#include <iostream>
#include <functional>
#include <queue>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

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

		bool empty() const {
			std::unique_lock<std::mutex> lock(this->mutex);
			return this->queue.empty();
		}
	};

	class ImageSocket
	{
		asio::io_service send_io_service;
		tcp::socket send_socket;

		asio::io_service receive_io_service;
		tcp::socket receive_socket;
		asio::streambuf receive_buffer;
		tcp::acceptor acceptor;
		Queue<ImageFrame*> queue;

	public:
		std::string ip_address;
		int send_port;
		int receive_port;
		bool suspend_cast_loop;
		bool suspend_receive_loop;
		std::function<void(ImageFrame *frame)> receive_handler;

		ImageSocket(std::string ip_address, int send_port, int receive_port) : send_socket(this->send_io_service), receive_socket(this->receive_io_service), acceptor(this->receive_io_service, tcp::endpoint(tcp::v4(), receive_port)) {
			this->ip_address = ip_address;
			this->send_port = send_port;
			this->receive_port = receive_port;

			// asio::socket_base::reuse_address option(true);
			// this->acceptor.set_option(option);
		}

		~ImageSocket() {
		};

		void push_frame(ImageFrame *frame) {
			this->queue.push(frame);
		}

		void send_data(unsigned char *data, int size) {
			this->send_socket.connect(tcp::endpoint(asio::ip::address::from_string(this->ip_address), this->send_port));
			this->send_io_service.run();

			boost::system::error_code error;
			asio::write(this->send_socket, asio::buffer(data, size), error);
		}

		void send_frame(Perevod::ImageFrame *frame) {
			unsigned char *frame_data = new unsigned char[frame->image_width() * frame->image_height() * 3 + sizeof(uint32_t) * 4];
			frame->read_raw_byte(frame_data);
			this->send_data(frame_data, frame->frame_size());
			delete frame_data;
			this->send_socket.close();
		}

		ImageFrame* read_frame() {
			this->acceptor.accept(this->receive_socket);

			asio::socket_base::keep_alive option(true);
			this->receive_socket.set_option(option);

			ImageFrame *frame = nullptr;
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
				frame = new ImageFrame(x, y, width, height, image_data);
			}
			this->receive_buffer.consume(this->receive_buffer.size());
			this->receive_socket.close();

			return frame;
		}

		void run_cast_loop() {
			this->suspend_cast_loop = false;
			while (!this->suspend_cast_loop) {
				ImageFrame *frame = this->queue.pop();
				this->send_frame(frame);
			}
		}

		void run_receive_loop() {
			this->suspend_receive_loop = false;
			while (!this->suspend_receive_loop) {
				ImageFrame *frame = this->read_frame();
				this->receive_handler(frame);
				delete frame;
			}
		}
	};
}
