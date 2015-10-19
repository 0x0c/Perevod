#include <string>
#include <functional>
#include <thread>
#include <cv.h>
#include <highgui.h>
#include "Perevod.h"

namespace asio = boost::asio;
using asio::ip::tcp;

cv::Mat bytes_to_mat(unsigned char *bytes, uint32_t width, uint32_t height)
{
	cv::Mat image = cv::Mat(height, width, CV_8UC3);
	std::memcpy(image.data, bytes, width * height * 3);
	return image.clone();
}

int main(int argc, char **argv)
{
	Perevod::ImageSocket socket(std::string(argv[2]), atoi(argv[3]), atoi(argv[4]));
	if (strcmp(argv[1], "-c") == 0) {
		std::cout << "client" << std::endl;
		auto t = std::thread([&socket] {
			socket.run_cast_loop();
		});
		int index = 0;
		while (1) {
			std::string filename = "img/" + std::to_string(index % 30 + 1) + ".jpeg";
			cv::Mat image = cv::imread(filename);
			cv::imshow("send image", image);
			Perevod::ImageFrame *frame = new Perevod::ImageFrame(0, 0, image.cols, image.rows, image.data);
			std::cout << "send frame : " << frame->size() << std::endl;
			socket.push_frame(frame);
			index++;
			if (cv::waitKey(60) > 0) {
				break;
			}
		}
		t.join();
	}
	else if (strcmp(argv[1], "-s") == 0){
		std::cout << "server" << std::endl;
		// socket.receive_handler = [&](Perevod::ImageFrame *frame) {
		// 	if (frame) {
		// 		std::cout << "read frame : " << frame->size() << std::endl;
		// 		cv::Mat image = bytes_to_mat(frame->image_data(), frame->image_width(), frame->image_height());
		// 		cv::imshow("received image", image);
		// 		if (cv::waitKey(60) > 0) {
		// 			socket.suspend_receive_loop = true;
		// 		}
		// 	}
		// };
		auto t = std::thread([&socket] {
			socket.run_receive_loop();
		});
		while (1) {
			Perevod::ImageFrame *frame = socket.pop_frame();
			if (frame) {
				cv::Mat image = bytes_to_mat(frame->image_data(), frame->image_width(), frame->image_height());
				cv::imshow("received image", image);
				delete frame;
				if (cv::waitKey(60) > 0) {
					break;
				}
			}
		}
		t.join();
	}
}
