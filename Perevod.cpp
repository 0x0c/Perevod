#include "Perevod.h"

namespace asio = boost::asio;
using asio::ip::tcp;
using asio::ip::udp;

namespace Perevod
{
	ImageFrame::ImageFrame() {
		this->x = 0;
		this->y = 0;
		this->width = 0;
		this->height = 0;
	}
	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data) : ImageFrame() {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->data = data;
	}

	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, int image_data_size) : ImageFrame(){
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->data.resize(image_data_size);
		std::copy(data, data + image_data_size, this->data.begin());
	}

	std::vector<unsigned char> ImageFrame::frame_data() {
		return this->data;
	}

	int ImageFrame::frame_size() {
		return this->data_size() + sizeof(uint32_t) * 4;
	}

	unsigned char* ImageFrame::raw_data() {
		return this->data.data();
	}
	
	int ImageFrame::data_size() {
		return this->data.size() * sizeof(unsigned char);
	}

	void ImageFrame::read_raw_byte(unsigned char *frame_data) {
		std::memcpy(frame_data, &this->x, sizeof(uint32_t));
		std::memcpy(frame_data + sizeof(uint32_t), &this->y, sizeof(uint32_t));
		std::memcpy(frame_data + sizeof(uint32_t) * 2, &this->width, sizeof(uint32_t));
		std::memcpy(frame_data + sizeof(uint32_t) * 3, &this->height, sizeof(uint32_t));
		std::memcpy(frame_data + sizeof(uint32_t) * 4, this->raw_data(), this->data_size());
	}

	ImageSocketImpl::ImageSocketImpl(std::string ip_address, int send_port, int receive_port) {
		this->ip_address = ip_address;
		this->send_port = send_port;
		this->receive_port = receive_port;
	}

	void ImageSocketImpl::send_frame(std::shared_ptr<Perevod::ImageFrame>frame) {
		unsigned char *frame_data = new unsigned char[frame->frame_size()];
		frame->read_raw_byte(frame_data);
		this->send_data(frame_data, frame->frame_size());
		delete frame_data;
	}

	ImageSocketTCPImpl::ImageSocketTCPImpl(std::string ip_address, int send_port, int receive_port) :ImageSocketImpl(ip_address, send_port, receive_port), send_socket(this->send_io_service), receive_socket(this->receive_io_service), acceptor(this->receive_io_service, tcp::endpoint(tcp::v4(), receive_port)) {
		asio::socket_base::reuse_address option(true);
		this->acceptor.set_option(option);
		asio::socket_base::keep_alive keep_alive(true);
		this->acceptor.set_option(keep_alive);
	}
	
	void ImageSocketTCPImpl::send_data(unsigned char *data, int size) {
		boost::system::error_code error;
		do {
			this->send_socket.connect(tcp::endpoint(asio::ip::address::from_string(this->ip_address), this->send_port), error);
		} while (error);
		PEREVOD_DEBUG_LOG("TCP connected");

		this->send_io_service.run();

		asio::write(this->send_socket, asio::buffer(data, size), error);
		this->send_socket.close();	
	}

	std::shared_ptr<Perevod::ImageFrame> ImageSocketTCPImpl::read_frame() {
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
}