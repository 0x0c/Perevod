#pragma once
#include "../util/Queue.h"
#include "../frame/ImageFrame/ImageFrame.h"
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace Perevod
{
	class ImageSocketImpl
	{
	public:
		std::string ip_address;
		int send_port;
		int receive_port;
		Queue<std::shared_ptr<Perevod::ImageFrame>> send_queue;
		Queue<std::shared_ptr<Perevod::ImageFrame>> receive_queue;
		boost::asio::streambuf receive_buffer;

		ImageSocketImpl(std::string ip_address, int send_port, int receive_port);

		virtual void send_data(unsigned char *data, int size) = 0;
		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame);
	};
}