#pragma once
#include "../frame/ImageFrame/ImageFrame.h"
#include <string>
#include <functional>
#include <memory>

namespace Perevod
{
	template <typename T> class ImageSocket
	{
		T *impl;
	public:
		bool suspend_cast_loop;
		bool suspend_receive_loop;
		std::function<void(std::shared_ptr<Perevod::ImageFrame>)> receive_handler;

		#ifndef _WIN32
		long long cast_interval;
		#endif

		ImageSocket(std::string ip_address, int send_port, int receive_port);
		~ImageSocket();

		void set_cast_queue_limit(int limit);
		void set_receive_queue_limit(int limit);
		void push_frame(std::shared_ptr<Perevod::ImageFrame>frame);
		std::shared_ptr<Perevod::ImageFrame> pop_frame();
		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame);
		std::shared_ptr<Perevod::ImageFrame> read_frame();
		void run_cast_loop();
		void run_receive_loop();
	};
}

#include "detail/ImageSocket.h"