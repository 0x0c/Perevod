#include "ImageFrame.h"
#include "../../util/macro.h"

namespace Perevod
{
	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, std::vector<unsigned char> data) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->data = data;
	}

	ImageFrame::ImageFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, unsigned char *data, size_t image_data_size) {
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->data.resize(image_data_size);
		std::copy(data, data + image_data_size, this->data.begin());
	}

	std::vector<unsigned char> ImageFrame::frame_data() {
		return this->data;
	}

	uint32_t ImageFrame::frame_size() {
		// frame count + x, y, width, height + data size + data
		int frame_size =  sizeof(uint8_t) + sizeof(uint32_t) * 4 + sizeof(uint32_t) + this->data_size();
		for (int i = 0; i < this->sub_frame.size(); ++i) {
			std::shared_ptr<Perevod::ImageFrame> f = this->sub_frame.at(i);
			frame_size += sizeof(uint32_t) * 4 + sizeof(uint32_t) + f->data_size();
		}

		return frame_size;
	}
	
	uint32_t ImageFrame::data_size() {
		return this->data.size() * sizeof(unsigned char);
	}

	void ImageFrame::append_raw_byte(unsigned char *frame_data, size_t *offset) {
		std::memcpy(frame_data + *offset, &this->x, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->y, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->width, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, &this->height, sizeof(uint32_t));
		*offset += sizeof(uint32_t);

		uint32_t data_size = this->data_size();
		std::memcpy(frame_data + *offset, &data_size, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		std::memcpy(frame_data + *offset, this->data.data(), this->data_size());
		*offset += this->data_size();
	}

	void ImageFrame::read_raw_byte(unsigned char *frame_data) {
		size_t offset = 0;
		uint8_t frame_count = 1 + this->sub_frame.size();
		PEREVOD_DEBUG_LOG("send frame count " << unsigned(frame_count));
		std::memcpy(frame_data, &frame_count, sizeof(uint8_t));
		offset += sizeof(uint8_t);
		this->append_raw_byte(frame_data, &offset);

		for (int i = 0; i < this->sub_frame.size(); ++i) {
			PEREVOD_DEBUG_LOG("append sub frame");
			std::shared_ptr<Perevod::ImageFrame> f = this->sub_frame.at(i);
			f->append_raw_byte(frame_data, &offset);
		}
	}
}
