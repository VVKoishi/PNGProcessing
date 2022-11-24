#pragma once
#include <iostream>
#include <vector>
#include <filesystem>
#include "lodepng\lodepng.h"

class PNGBuffer
{
	using path = std::filesystem::path;
public:
	PNGBuffer(const path p) : p(p) {
		//decode
		unsigned error = lodepng::decode(image, width, height, p.string());

		//if there's an error, display it
		if (error) {
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			state = false;
		}

		//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
	}

	PNGBuffer(unsigned width, unsigned height) : width(width), height(height) {
		//create an empty buffer
		image.resize(width * height * 4);
	}

	void WriteBufferToFile(const path p) {
		std::cout << "Writing file " << p.string() << "...\n";

		//Encode the image
		unsigned error = lodepng::encode(p.string(), image, width, height);

		//if there's an error, display it
		if (error) std::cout << "encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
	}

	const path p;
	std::vector<unsigned char> image; //the raw pixels
	unsigned width, height;
	bool state = true;
};

