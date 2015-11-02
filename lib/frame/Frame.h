#pragma once
#include <vector>

namespace Perevod
{
	template <typename T> class Frame
	{
	protected:
		std::vector<T> data;
	public:
		uint32_t x, y, width, height;

		virtual std::vector<T> frame_data() = 0;
		virtual uint32_t frame_size() = 0;
		virtual uint32_t data_size() = 0;
		virtual void read_raw_byte(T *frame_data) = 0;
	};
}