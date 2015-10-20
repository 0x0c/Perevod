#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <cv.h>
#include <highgui.h>
#include "Perevod.h"

namespace asio = boost::asio;
using asio::ip::tcp;

#define DEBUG
#ifdef DEBUG
#define DEBUG_LOG(x) std::cout << __PRETTY_FUNCTION__ << " " <<  x << std::endl;
#endif

cv::Mat bytes_to_mat(unsigned char *bytes, uint32_t width, uint32_t height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC3);
	std::memcpy(image.data, bytes, width * height * 3);
	return image.clone();
}

int main(int argc, char **argv)
{
	Perevod::ImageSocket socket(std::string(argv[2]), atoi(argv[3]), atoi(argv[4]), Perevod::ImageSocketModeTCP);
	auto t = std::thread([&socket] {
		socket.run_cast_loop();
	});
	auto t2 = std::thread([&socket] {
		socket.run_receive_loop();
	});
	int index = 0;
	while (1) {
		std::string filename = "img/" + std::string(argv[1]) + "/" + std::to_string(index % 30 + 1) + ".jpeg";
		cv::Mat image = cv::imread(filename);

		auto frame = std::make_shared<Perevod::ImageFrame>(Perevod::ImageFrame(200, 400, image.cols, image.rows, image.data));
		std::cout << "send image" << std::endl;
		socket.push_frame(frame);
		index++;

		cv::imshow("send image" + std::string(argv[1]), image);

		auto frame2 = socket.pop_frame();
		if (frame2) {
			std::cout << "receied image" << std::endl;
			cv::Mat image = bytes_to_mat(frame2->image_data(), frame2->image_width(), frame2->image_height());
			std::cout << "show image" << std::endl;
			cv::imshow("received image" + std::string(argv[1]), image);
		}
		if (cv::waitKey(60) > 0) {
			break;
		}
	}
}
