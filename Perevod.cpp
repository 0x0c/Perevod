#include "Perevod.h"

namespace asio = boost::asio;
using asio::ip::tcp;
using asio::ip::udp;

namespace Perevod
{
	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->data = data;
	}

	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, int image_data_size) {
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

	uint32_t ImageFrame::frame_size() {
		// frame count + x, y, width, height + data size + data
		int frame_size =  sizeof(uint8_t) + sizeof(uint32_t) * 4 + sizeof(uint32_t) + this->data_size();
		for (int i = 0; i < this->sub_frame.size(); ++i) {
			std::shared_ptr<Perevod::ImageFrame> f = this->sub_frame.at(i);
			frame_size += sizeof(uint32_t) * 4 + sizeof(uint32_t) + f->data_size();
		}

		return frame_size;
	}
	
	uint32_t ImageFrame::data_size() {
		return this->data.size() * sizeof(unsigned char);
	}

	void ImageFrame::append_raw_byte(unsigned char *frame_data, int *offset) {
		std::memcpy(frame_data + *offset, &this->x, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->y, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->width, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->height, sizeof(uint32_t));
		*offset += sizeof(uint32_t);

		uint32_t data_size = this->data_size();
		std::memcpy(frame_data + *offset, &data_size, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, this->data.data(), this->data_size());
		*offset += this->data_size();
	}

	void ImageFrame::read_raw_byte(unsigned char *frame_data) {
		int offset = 0;
		uint8_t frame_count = 1 + this->sub_frame.size();
		PEREVOD_DEBUG_LOG("send frame count " + unsigned(frame_count));
		std::memcpy(frame_data, &frame_count, sizeof(uint8_t));
		offset += sizeof(uint8_t);
		this->append_raw_byte(frame_data, &offset);

		std::cout << "sub frame size : " << this->sub_frame.size() << std::endl;
		for (int i = 0; i < this->sub_frame.size(); ++i) {
			PEREVOD_DEBUG_LOG("append sub frame");
			std::shared_ptr<Perevod::ImageFrame> f = this->sub_frame.at(i);
			f->append_raw_byte(frame_data, &offset);
		}
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
		asio::socket_base::reuse_address reuse_address(true);
		this->acceptor.set_option(reuse_address);
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

	std::shared_ptr<Perevod::ImageFrame> ImageSocketTCPImpl::parse_frame(const unsigned char *data, int *offset) {	
		uint32_t x, y;
		std::memcpy(&x, data + *offset, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(&y, data + *offset, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		
		uint32_t width, height;
		std::memcpy(&width, data + *offset, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(&height, data + *offset, sizeof(uint32_t));
		*offset += sizeof(uint32_t);

		PEREVOD_DEBUG_LOG("received : " + x + ", " + y + ", " + width + ", " + height);

		uint32_t data_size;
		std::memcpy(&data_size, data + *offset, sizeof(uint32_t));
		PEREVOD_DEBUG_LOG("received data size : " + data_size);
		*offset += sizeof(uint32_t);

		unsigned char *image_data = (unsigned char *)data + *offset;
		*offset += data_size;
		
		return std::make_shared<Perevod::ImageFrame>(ImageFrame(x, y, width, height, image_data, data_size));
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
			int offset = 0;
			const unsigned char *data = asio::buffer_cast<const unsigned char *>(this->receive_buffer.data());

			uint8_t frame_count = 0;
			std::memcpy(&frame_count, data, sizeof(uint8_t));
			PEREVOD_DEBUG_LOG("receive frame count : " + unsigned(frame_count));
			offset += sizeof(uint8_t);
			frame = this->parse_frame(data, &offset);

			for (int i = 0; i < frame_count - 1; ++i) {
				std::shared_ptr<Perevod::ImageFrame> sub_frame = this->parse_frame(data, &offset);
				frame->sub_frame.push_back(sub_frame);
			}
		}
		this->receive_buffer.consume(this->receive_buffer.size());
		this->receive_socket.close();

		return frame;
	}
}
