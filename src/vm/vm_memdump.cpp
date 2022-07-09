/**
 * memory dump and visualisation functions
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 9-july-2022
 * @license see 'LICENSE.EUPL' file
 */

#include "vm.h"

#include <iostream>
#include <sstream>
#include <cstring>


#ifdef USE_BOOST_GIL

// see: https://github.com/boostorg/gil/tree/develop/example
#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png.hpp>


/**
 * visualises vm memory utilisation
 */
void VM::DrawMemoryImage()
{
	namespace gil = boost::gil;

	//using t_img = gil::gray8_image_t;
	using t_img = gil::rgb8_image_t;
	using t_view = typename t_img::view_t;
	using t_coord = typename t_view::coord_t;
	using t_pixel = typename gil::channel_type<t_img>::type;
	using t_format = typename gil::png_tag;

	std::size_t length = static_cast<std::size_t>(
		std::ceil(std::sqrt(static_cast<t_real>(m_memsize * 8))));

	int pixel_scale = 2;
	length *= pixel_scale;

	t_img img(length, length);
	t_view view = gil::view(img);

	for(t_coord y = 0; y < img.height(); ++y)
	{
		t_coord x = 0;
		for(auto iter = view.row_begin(y); iter != view.row_end(y); ++iter)
		{
			std::uint64_t mem_pos = y/pixel_scale*img.width()/pixel_scale + x/pixel_scale;
			std::uint64_t mem_byte = mem_pos / 8;
			std::uint8_t mem_bit = mem_pos % 8;

			t_pixel pixel[] { 0x00, 0x00, 0x00 };
			if(mem_byte < static_cast<std::uint64_t>(m_memsize))
			{
				bool thebit = (m_mem[mem_byte] & (1 << (8-mem_bit))) != 0;

				// memory bit set?
				if(thebit)
					pixel[0] = 0xff;

				// mark instruction pointer position
				if(mem_byte == static_cast<std::uint64_t>(m_ip))
					pixel[1] |= 0xff;

				// mark current stack frame
				if(mem_byte >= static_cast<std::uint64_t>(m_sp) &&
					mem_byte <= static_cast<std::uint64_t>(m_bp))
					pixel[2] |= 0xff;
			}

			(*iter)[0] = pixel[0];
			(*iter)[1] = pixel[1];
			(*iter)[2] = pixel[2];
			++x;
		}
	}

	static std::size_t ctr = 0;
	std::string file = "mem_" + std::to_string(ctr++) + ".png";
	gil::write_view(file, view, t_format{});
}


#else


/**
 * visualises vm memory utilisation (dummy function)
 */
void VM::DrawMemoryImage()
{
}

#endif
