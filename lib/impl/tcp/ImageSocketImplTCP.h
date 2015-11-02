#pragma once
#include "../ImageSocketImpl.h"

namespace Perevod
{
	class ImageSocketTCPImpl : public ImageSocketImpl
	{
		boost::asio::io_service send_io_service;
		boost::asio::ip::tcp::socket send_socket;
		
		boost::asio::io_service receive_io_service;
		boost::asio::ip::tcp::socket receive_socket;
		boost::asio::ip::tcp::acceptor acceptor;
	public:
		ImageSocketTCPImpl(std::string ip_address, int send_port, int receive_port);

		void send_data(unsigned char *data, size_t size);
		std::shared_ptr<Perevod::ImageFrame> parse_frame(const unsigned char *data, size_t *offset);
		std::shared_ptr<Perevod::ImageFrame> read_frame();
	};
}