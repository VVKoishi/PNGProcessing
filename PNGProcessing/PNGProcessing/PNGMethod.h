#pragma once
#include <algorithm>
#include "PNGBuffer.h"

struct DownSample2x {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		for (unsigned i = begin; i < end; i += 4) {
			unsigned xs = (i >> 2) % result.width << 1;
			unsigned ys = (i >> 2) / result.width << 1;
			unsigned j0 = (ys * source.width + xs) << 2;
			unsigned j1 = j0 + 4;
			unsigned j2 = j0 + (source.width << 2);
			unsigned j3 = j2 + 4;
			result.image[i + 0] = (source.image[j0 + 0] + source.image[j1 + 0] + source.image[j2 + 0] + source.image[j3 + 0]) >> 2;
			result.image[i + 1] = (source.image[j0 + 1] + source.image[j1 + 1] + source.image[j2 + 1] + source.image[j3 + 1]) >> 2;
			result.image[i + 2] = (source.image[j0 + 2] + source.image[j1 + 2] + source.image[j2 + 2] + source.image[j3 + 2]) >> 2;
			result.image[i + 3] = (source.image[j0 + 3] + source.image[j1 + 3] + source.image[j2 + 3] + source.image[j3 + 3]) >> 2;
		}
	}
};

struct UpSample2x {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		for (unsigned i = begin; i < end; i += 4) {
			unsigned xs = (i >> 2) % result.width >> 1;
			unsigned ys = (i >> 2) / result.width >> 1;
			unsigned j0 = (ys * source.width + xs) << 2;
			for (unsigned offset = 0; offset < 4; offset++) {
				result.image[i + offset] = source.image[j0 + offset];
			}
		}
	}
};

struct GrayScale {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		for (unsigned i = begin; i < end; i += 4) {
			float gray = source.image[i + 0] * 0.299f + source.image[i + 1] * 0.587f + source.image[i + 2] * 0.114f; // https://en.wikipedia.org/wiki/Grayscale
			unsigned char val = static_cast<unsigned char>(std::clamp(gray, 0.f, 255.f));
			result.image[i + 0] = val;
			result.image[i + 1] = val;
			result.image[i + 2] = val;
			result.image[i + 3] = source.image[i + 3]; // do nothing on alpha
		}
	}
};

// from (x,y) to offset*4
inline unsigned loc(const PNGBuffer& source, unsigned x, unsigned y) {
	x = std::clamp(x, 0u, source.width - 1);
	y = std::clamp(y, 0u, source.height - 1);
	return (y * source.width + x) << 2;
}

// 7x7 Gaussion Kernel
static const float gauss_kernel[] = { 0.003f, 0.048f, 0.262f, 0.415f }; // 0.262, 0.048, 0.003

struct GaussionBlur1st {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step1.Horizontal
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;
			unsigned j0 = loc(source, x - 3, y);
			unsigned j1 = loc(source, x - 2, y);
			unsigned j2 = loc(source, x - 1, y);
			unsigned j3 = i;
			unsigned j4 = loc(source, x + 1, y);
			unsigned j5 = loc(source, x + 2, y);
			unsigned j6 = loc(source, x + 3, y);
			// 下面这种方式获取一个j0的计算操作更少，一个loc是4次比较，3次运算，下面是1次比较，2次运算
			// 因为在向上取坐标时，可知一定不会超过下界；在向下取坐标时，一定不会超过上界；只shift x，不需要判断y是否越界
			// 但是差别感受不明显，并且上面的形式可读性更强
			//unsigned pix_band = (i >> 2) % width;
			//unsigned pix_padd = width - pix_band - 1;
			//unsigned j0 = (3 <= pix_band) ? i - (3 << 2) : i - (pix_band << 2);
			//unsigned j1 = (2 <= pix_band) ? i - (2 << 2) : i - (pix_band << 2);
			//unsigned j2 = (1 <= pix_band) ? i - (1 << 2) : i - (pix_band << 2);
			//unsigned j3 = i;
			//unsigned j4 = (1 <= pix_padd) ? i + (1 << 2) : i + (pix_padd << 2);
			//unsigned j5 = (2 <= pix_padd) ? i + (2 << 2) : i + (pix_padd << 2);
			//unsigned j6 = (3 <= pix_padd) ? i + (3 << 2) : i + (pix_padd << 2);
			for (unsigned offset = 0; offset < 3; offset++) {
				float f	= source.image[j0 + offset] * gauss_kernel[0] + source.image[j1 + offset] * gauss_kernel[1] + source.image[j2 + offset] * gauss_kernel[2] + source.image[j3 + offset] * gauss_kernel[3]
						+ source.image[j4 + offset] * gauss_kernel[2] + source.image[j5 + offset] * gauss_kernel[1] + source.image[j6 + offset] * gauss_kernel[0];
				unsigned char val = static_cast<unsigned char>(std::clamp(f, 0.f, 255.f));
				result.image[i + offset] = val;
			}
			result.image[i + 3] = source.image[i + 3]; // do nothing on alpha
		}
	}
};

struct GaussionBlur2nd {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step2.Vertical
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;
			unsigned j0 = loc(source, x, y - 3);
			unsigned j1 = loc(source, x, y - 2);
			unsigned j2 = loc(source, x, y - 1);
			unsigned j3 = i;
			unsigned j4 = loc(source, x, y + 1);
			unsigned j5 = loc(source, x, y + 2);
			unsigned j6 = loc(source, x, y + 3);
			//unsigned pix_band = (i >> 2) / width;
			//unsigned pix_padd = height - pix_band - 1;
			//unsigned j0 = (3 <= pix_band) ? i - (3 * width << 2) : i - (pix_band * width << 2);
			//unsigned j1 = (2 <= pix_band) ? i - (2 * width << 2) : i - (pix_band * width << 2);
			//unsigned j2 = (1 <= pix_band) ? i - (1 * width << 2) : i - (pix_band * width << 2);
			//unsigned j3 = i;
			//unsigned j4 = (1 <= pix_padd) ? i + (1 * width << 2) : i + (pix_padd * width << 2);
			//unsigned j5 = (2 <= pix_padd) ? i + (2 * width << 2) : i + (pix_padd * width << 2);
			//unsigned j6 = (3 <= pix_padd) ? i + (3 * width << 2) : i + (pix_padd * width << 2);
			for (unsigned offset = 0; offset < 3; offset++) {
				float f = source.image[j0 + offset] * gauss_kernel[0] + source.image[j1 + offset] * gauss_kernel[1] + source.image[j2 + offset] * gauss_kernel[2] + source.image[j3 + offset] * gauss_kernel[3]
						+ source.image[j4 + offset] * gauss_kernel[2] + source.image[j5 + offset] * gauss_kernel[1] + source.image[j6 + offset] * gauss_kernel[0];
				unsigned char val = static_cast<unsigned char>(std::clamp(f, 0.f, 255.f));
				result.image[i + offset] = val;
			}
			result.image[i + 3] = source.image[i + 3]; // do nothing on alpha
		}
	}
};

// 3x3 sobel kernel https://en.wikipedia.org/wiki/Sobel_operator
static const int sobel_kernel_x_horizontal[] = { 1, 0, -1 };
static const int sobel_kernel_x_vertical[] = { 1, 2, 1 };
static const int sobel_kernel_y_horizontal[] = { 1, 2, 1 };
static const int sobel_kernel_y_vertical[] = { 1, 0, -1 };

struct EdgeDetection1st {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end, PNGBuffer& GyH) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step1.Horizontal
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;
			// 在灰度图上做，只计算r
			unsigned char mat[] = {
				source.image[loc(source, x - 1, y)], source.image[i], source.image[loc(source, x + 1, y)]
			};
			// 一次读取输出两张贴图 GxH GyH
			int Gx = mat[0] * sobel_kernel_x_horizontal[0] + mat[1] * sobel_kernel_x_horizontal[1] + mat[2] * sobel_kernel_x_horizontal[2];
			int Gy = mat[0] * sobel_kernel_y_horizontal[0] + mat[1] * sobel_kernel_y_horizontal[1] + mat[2] * sobel_kernel_y_horizontal[2];
			unsigned char valx = static_cast<unsigned char>(std::clamp(static_cast<float>(Gx), 0.f, 255.f));
			unsigned char valy = static_cast<unsigned char>(std::clamp(static_cast<float>(Gy), 0.f, 255.f));
			result.image[i] = valx;
			GyH.image[i] = valy;
		}
	}
};

struct EdgeDetection2ndX {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step2.Vertical Gx
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;
			// only r
			unsigned char mat[] = {
				source.image[loc(source, x, y - 1)], source.image[i], source.image[loc(source, x, y + 1)]
			};
			int Gx = mat[0] * sobel_kernel_x_vertical[0] + mat[1] * sobel_kernel_x_vertical[1] + mat[2] * sobel_kernel_x_vertical[2];
			unsigned char valx = static_cast<unsigned char>(std::clamp(static_cast<float>(Gx), 0.f, 255.f));
			result.image[i] = valx;
		}
	}
};

struct EdgeDetection2ndY {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step2.Vertical Gy
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;
			// only r
			unsigned char mat[] = {
					source.image[loc(source, x, y - 1)], source.image[i], source.image[loc(source, x, y + 1)]
			};
			int Gy = mat[0] * sobel_kernel_y_vertical[0] + mat[1] * sobel_kernel_y_vertical[1] + mat[2] * sobel_kernel_y_vertical[2];
			unsigned char valy = static_cast<unsigned char>(std::clamp(static_cast<float>(Gy), 0.f, 255.f));
			result.image[i] = valy;
		}
	}
};

struct EdgeDetection3rd {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end, const PNGBuffer& sourceGy) {
		unsigned width = source.width;
		unsigned height = source.height;
		// Step3.merge Gx Gy
		for (unsigned i = begin; i < end; i += 4) {
			unsigned x = (i >> 2) % width;
			unsigned y = (i >> 2) / width;

			// only r
			float r_Gx = static_cast<float>(source.image[i + 0]);
			float r_Gy = static_cast<float>(sourceGy.image[i + 0]);
			float r_G = sqrtf(r_Gx * r_Gx + r_Gy * r_Gy);
			unsigned char r = static_cast<unsigned char>(std::clamp(r_G, 0.f, 255.f));
			unsigned char val = r >= 70 ? 255 : 0;

			result.image[i + 0] = val;
			result.image[i + 1] = val;
			result.image[i + 2] = val;
			result.image[i + 3] = 255; // 最后再set一下alpha就行
		}
	}
};

struct BrightnessExtraction {
	void operator()(const PNGBuffer& source, PNGBuffer& result, const unsigned begin, const unsigned end, unsigned char thresold) {
		for (unsigned i = begin; i < end; i += 4) {
			// only r
			unsigned char val = source.image[i] >= thresold ? 255 : 0;
			result.image[i + 0] = val;
			result.image[i + 1] = val;
			result.image[i + 2] = val;
			result.image[i + 3] = 255;
		}
	}
};