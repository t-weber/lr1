/**
 * tests if boost.gil compiles
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 10-july-2022
 * @license see 'LICENSE.EUPL' file
 *
 * @see https://github.com/boostorg/gil/tree/develop/example
 */

#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png/write.hpp>


void test_gil()
{
	using t_img = boost::gil::gray8_image_t;
	using t_view = typename t_img::view_t;
	using t_point = typename t_view::point_t;
	using t_coord = typename t_view::coord_t;
	using t_pixel = typename boost::gil::channel_type<t_img>::type;
	using t_format = typename boost::gil::png_tag;

	t_img img(64, 64);
	t_view view = boost::gil::view(img);

	for(t_coord y = 0; y < view.height(); ++y)
	{
		// pixel access option 1
		for(t_coord x = 0; x < view.width(); ++x)
			view(x, y) = 0xff;

		// pixel access option 2
		for(auto iter = view.row_begin(y); iter != view.row_end(y); ++iter)
			(*iter)[0] = 0x00;
	}

	boost::gil::write_view("test_gil.png", view, t_format{});
}


int main()
{
	test_gil();
	return 0;
}
