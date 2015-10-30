#pragma once

#include <iostream>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>
#include <future>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#ifndef _WIN32
#include <chrono>
#endif

#ifdef __PEREVOD_DEBUG_MODE__
#define PEREVOD_DEBUG_LOG(x) std::cout << "[Perevod] " << x << std::endl;
#define PEREVOD_DEBUG_PRETTY_LOG(x) std::cout << "[Perevod] " << __PRETTY_FUNCTION__ << " " << x << std::endl;
#else
#define PEREVOD_DEBUG_LOG(x)
#define PEREVOD_DEBUG_PRETTY_LOG(x)
#endif

namespace Perevod
{
	template <typename T> class Frame
	{
	protected:
		std::vector<T> data;
	public:
		uint32_t x, y, width, height;

		virtual std::vector<T> frame_data() = 0;
		virtual int frame_size() = 0;
		virtual T* raw_data() = 0;
		virtual int data_size() = 0;
	};

	class ImageFrame : public Frame <unsigned char>
	{
	public:
		ImageFrame();
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data);
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, int image_data_size);

		std::vector<unsigned char> frame_data();
		int frame_size();
		unsigned char* raw_data();
		int data_size();
		void read_raw_byte(unsigned char *frame_data);
	};

	template <typename T> class Queue
	{
		std::queue<T> queue;
		mutable std::mutex mutex;
		std::condition_variable	cond;
	public:
		int limit;

		Queue();
		bool push(T data);
		T pop();
		T try_pop();
		bool empty() const;
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

		#ifndef _WIN32
		long long cast_interval;
		#endif

		ImageSocket(std::string ip_address, int send_port, int receive_port);
		~ImageSocket();

		void set_cast_queue_limit(int limit) ;
		void set_receive_queue_limit(int limit);
		void push_frame(std::shared_ptr<Perevod::ImageFrame>frame);
		std::shared_ptr<Perevod::ImageFrame> pop_frame();
		void send_frame(std::shared_ptr<Perevod::ImageFrame>frame);
		std::shared_ptr<Perevod::ImageFrame> read_frame();
		void run_cast_loop();
		void run_receive_loop();
	};
}

#include "detail/Perevod.h"