#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <cv.h>
#include <highgui.h>
#include "Perevod.h"

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

void run_as_tcp(std::string foldername, std::string ip_address, int send_port, int receive_port)
{
	Perevod::ImageSocket<Perevod::ImageSocketTCPImpl> socket(ip_address, send_port, receive_port);
	auto t = std::thread([&socket] {
		socket.run_cast_loop();
	});
	auto t2 = std::thread([&socket] {
		socket.run_receive_loop();
	});
	int index = 0;
	while (1) {
		std::string filename = "img/" + foldername + "/" + std::to_string(index % 30 + 1) + ".jpeg";
		cv::Mat image = cv::imread(filename);

		//(1) jpeg compression
		std::vector<unsigned char> buff;
		std::vector<int> param = std::vector<int>(2);
		param[0] = CV_IMWRITE_JPEG_QUALITY;
		param[1] = 50;
	 	cv::imencode(".jpg", image, buff, param);

		auto frame = std::make_shared<Perevod::ImageFrame>(Perevod::ImageFrame(200, 400, image.cols, image.rows, buff));
		std::cout << "send image" << std::endl;
		socket.push_frame(frame);
		index++;

		cv::imshow("send image" + foldername, image);

		auto frame2 = socket.pop_frame();
		if (frame2) {
			std::cout << "receied image" << std::endl;
			cv::Mat jpegimage = imdecode(cv::Mat(frame2->image_data()), CV_LOAD_IMAGE_COLOR);
			std::cout << "show image" << std::endl;
			cv::imshow("received image" + foldername, image);
		}
		if (cv::waitKey(60) > 0) {
			break;
		}
	}
}

void run_as_udp(std::string foldername, std::string ip_address, int send_port, int receive_port)
{
	
}

int main(int argc, char **argv)
{
	if (argc == 6) {
		if (strcmp(argv[1], "-t") == 0) {
			run_as_tcp(std::string(argv[2]), std::string(argv[3]), atoi(argv[4]), atoi(argv[5]));
		}
		// else if (strcmp(argv[1], "-u") == 0) {
		// 	run_as_udp(std::string(argv[2]), std::string(argv[3]), atoi(argv[4]), atoi(argv[5]));
		// }
	}
}
