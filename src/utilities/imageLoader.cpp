#include "imageLoader.hpp"
#include <iostream>

// Original source: https://raw.githubusercontent.com/lvandeve/lodepng/master/examples/example_decode.cpp
PNGImage loadPNGFile(std::string fileName)
{
	std::vector<unsigned char> png;
	std::vector<unsigned char> pixels; //the raw pixels
	unsigned int width, height;

	//load and decode
	unsigned error = lodepng::load_file(png, fileName);
	if(!error) error = lodepng::decode(pixels, width, height, png);

	//if there's an error, display it
	if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

	PNGImage image;
	image.width = width;
	image.height = height;
	image.pixels = pixels;

	return image;

}