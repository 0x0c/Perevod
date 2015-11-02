#pragma once
#include "../Frame.h"

namespace Perevod
{
	class ImageFrame : public Frame <unsigned char>
	{
	public:
		std::vector<std::shared_ptr<Perevod::ImageFrame>> sub_frame;

		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data);
		ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, size_t image_data_size);

		std::vector<unsigned char> frame_data();
		uint32_t frame_size();
		uint32_t data_size();
		void read_raw_byte(unsigned char *frame_data);
		void append_raw_byte(unsigned char *frame_data, size_t *offset);
	};
}
