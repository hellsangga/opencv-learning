#include "pch.h"

namespace luo
{
	void mean_blur(const cv::Mat& ori, cv::Mat& res, cv::Size block_size)
	{
		int rows = ori.rows;
		int cols = ori.cols * ori.channels();
		int channels = ori.channels();

		res = cv::Mat::zeros(ori.size(), ori.type());

		int rad_w = block_size.width / 2;
		int rad_h = block_size.height / 2;
		int block_total = block_size.width * block_size.height;

		cv::Mat copy;
		cv::copyMakeBorder(ori, copy, rad_h, rad_h, rad_w, rad_w, cv::BORDER_REPLICATE);

		int offet_h = rad_h;
		int offet_w = rad_w * channels;

		uchar* res_p;
		for (int y = offet_h; y < rows + offet_h; y++)
		{
			res_p = res.ptr<uchar>(y - offet_h);
			for (int x = offet_w; x < cols + offet_w; x += channels)
			{
				for (int c = 0; c < channels; c++)
				{
					float sum = 0.0;
					for (int b_y = y - offet_h; b_y <= y + offet_h; b_y++)
					{
						for (int b_x = x - offet_w; b_x <= x + offet_w; b_x += channels)
						{
							sum += static_cast<float>(copy.ptr<uchar>(b_y)[b_x + c]);
						}
					}

					res_p[x - offet_w + c] = round(sum / block_total);
				}
			}
		}
	}


	void median_blur(const cv::Mat& ori, cv::Mat& res, cv::Size block_size)
	{
		int rows = ori.rows;
		int cols = ori.cols * ori.channels();
		int channels = ori.channels();

		res = cv::Mat::zeros(ori.size(), ori.type());

		int rad_w = block_size.width / 2;
		int rad_h = block_size.height / 2;
		int block_total = block_size.width * block_size.height;

		cv::Mat copy;
		cv::copyMakeBorder(ori, copy, rad_h, rad_h, rad_w, rad_w, cv::BORDER_REPLICATE);

		int offet_h = rad_h;
		int offet_w = rad_w * channels;

		std::vector<int> block_val;
		block_val.resize(block_total);

		uchar* res_p;
		for (int y = offet_h; y < rows + offet_h; y++)
		{
			res_p = res.ptr<uchar>(y - offet_h);
			for (int x = offet_w; x < cols + offet_w; x += channels)
			{
				for (int c = 0; c < channels; c++)
				{
					int count = 0;
					for (int b_y = y - offet_h; b_y <= y + offet_h; b_y++)
					{
						for (int b_x = x - offet_w; b_x <= x + offet_w; b_x += channels)
						{
							int intensity = static_cast<int>(copy.ptr<uchar>(b_y)[b_x + c]);

							if (count == 0)
								block_val[0] = intensity;
							else
							{
								for (int i = count - 1; i >= 0; i--)
								{
									if (intensity >= block_val[i])
									{
										block_val[i + 1] = intensity;
										break;
									}

									int temp = block_val[i];
									block_val[i] = intensity;
									block_val[i + 1] = temp;
								}
							}

							count++;
 						}
					}

					res_p[x - offet_w + c] = block_val[block_total / 2];
				}
			}
		}
	}


	void gaussian_blur(const cv::Mat& ori, cv::Mat& res, cv::Size block_size, double sigma_x, double sigma_y = 0.0)
	{
		CV_Assert(ori.type() == CV_8UC1 || ori.type() == CV_8UC3);

		if (ori.channels() == 1)
			res = cv::Mat::zeros(ori.size(), CV_8UC1);
		else
			res = cv::Mat::zeros(ori.size(), CV_8UC3);

		int rows = ori.rows;
		int cols = ori.cols * ori.channels();
		int channels = ori.channels();

		if (block_size.width < 3 || block_size.height < 3)
			return;

		//block_size.width = block_size.width < ceil(6 * sigma_x) ? block_size.width : ceil(6 * sigma_x);
		//block_size.height = block_size.height < ceil(6 * sigma_y) ? block_size.height : ceil(6 * sigma_y);

		if (block_size.width % 2 == 0)
			block_size.width -= 1;
		if (block_size.height % 2 == 0)
			block_size.height -= 1;

		int radius_x = floor(block_size.height / 2);
		int radius_y = floor(block_size.width / 2);

		cv::Mat copy;
		cv::copyMakeBorder(ori, copy, radius_x, radius_x, radius_y, radius_y, cv::BORDER_REPLICATE);

		if (sigma_x <= 0.0 && sigma_y <= 0.0)
		{
			sigma_x = 0.3 * ((block_size.width - 1) * 0.5 - 1) + 0.8;
			sigma_y = 0.3 * ((block_size.height - 1) * 0.5 - 1) + 0.8;
		}
		else if (sigma_x <= 0.0 && sigma_y > 0.0)
		{
			sigma_x = sigma_y;
		}
		else if (sigma_x > 0.0 && sigma_y <= 0.0)
		{
			sigma_y = sigma_x;
		}

		const double PI = acos(-1);

		std::vector<std::vector<double>> kernel(block_size.height, std::vector<double>(block_size.width));
		std::vector<double> kernel_x, kernel_y;
		kernel_x.resize(block_size.height);
		kernel_y.resize(block_size.width);

		int dx = 0, dy = 0;
		for (int x = 0; x < kernel_x.size(); x++)
		{
			dx = x - radius_x;
			kernel_x[x] = (1 / sqrt(2 * PI * sigma_x * sigma_x)) * exp(-(dx * dx) / (2 * sigma_x * sigma_x));
		}
		for (int y = 0; y < kernel_y.size(); y++)
		{
			dy = y - radius_y;
			kernel_y[y] = (1 / sqrt(2 * PI * sigma_y * sigma_y)) * exp(-(dy * dy) / (2 * sigma_y * sigma_y));
		}

		double sum = 0.0;
		for (int x = 0; x < kernel.size(); x++)
		{
			for (int y = 0; y < kernel[x].size(); y++)
			{
				kernel[x][y] = kernel_x[x] * kernel_y[y];
				sum += kernel[x][y];
			}
		}

		for (int x = 0; x < kernel.size(); x++)
		{
			for (int y = 0; y < kernel[x].size(); y++)
			{
				kernel[x][y] /= sum;
			}
		}

		for (int x = radius_x; x < rows + radius_x; x++)
		{
			for (int y = radius_y * channels; y < cols + radius_y * channels; y += channels)
			{
				for (int c = 0; c < channels; c++)
				{
					double convolution = 0.0;
					int index_x = -1;
					for (int dx = x - radius_x; dx <= x + radius_x; dx++)
					{
						int index_y = -1;
						index_x++;
						for (int dy = (y + c) - radius_y * channels; dy <= (y + c) + radius_y * channels; dy += channels)
						{
							index_y++;
							convolution += (int)(copy.ptr<uchar>(dx)[dy]) * kernel[index_x][index_y];
						}
					}

					res.ptr<uchar>(x - radius_x)[y - radius_y * channels + c] = static_cast<int>(convolution);
				}
			}
		}
	}

	enum BlurMethod
	{
		MEAN_BLUR,
		MEDIAN_BLUR,
		GAUSSIAN_BLUR
	};

	void blur(const cv::Mat& ori, cv::Mat& res, cv::Size block_size, int method, double sigma_x = 0.0, double sigma_y = 0.0)
	{
		CV_Assert(ori.depth() == CV_8U);

		if (block_size.width < 3)
			block_size.width = 3;
		if (block_size.height < 3)
			block_size.height = 3;

		if (block_size.width % 2 == 0)
			block_size.width--;
		if (block_size.height % 2 == 0)
			block_size.height--;

		switch (method)
		{
		case MEAN_BLUR:
			mean_blur(ori, res, block_size);
			break;

		case MEDIAN_BLUR:
			median_blur(ori, res, block_size);
			break;

		case GAUSSIAN_BLUR:
			gaussian_blur(ori, res, block_size, sigma_x, sigma_y);
			break;

		default:
			break;
		}
	}
}