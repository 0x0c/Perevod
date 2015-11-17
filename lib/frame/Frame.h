#pragma once
#include <vector>

namespace Perevod
{
	template <typename T> class Frame
	{
	protected:
		std::vector<T> data;
	public:
		Frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<T> data);
		Frame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, T *data, size_t image_data_size);

		std::vector<std::shared_ptr<Frame>> sub_frame;
		uint32_t x, y, width, height;
		size_t element_size();
		std::vector<T> frame_data();
		uint32_t frame_size();
		uint32_t data_size();
		void append_raw_byte(T *frame_data, size_t *offset);
		void read_raw_byte(T *frame_data);
	};
}

#include "detail/Frame.h"
