#include "Graphics.h"

namespace Texture
{
	ImageData::ImageData(std::wstring_view a_path) :
		path(a_path)
	{}

	ImageData::ImageData(std::wstring_view a_folder, std::wstring_view a_textureName)
	{
		path.append(a_folder).append(a_textureName).append(L".png");
	}

	ImageData::~ImageData()
	{
		if (srView) {
			srView.Reset();
		}
		if (image) {
			image.reset();
		}
	}

	bool ImageData::Create(bool a_resizeToScreenRes)
	{
		bool result = false;

		image = std::make_shared<DirectX::ScratchImage>();
		HRESULT hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::Renderer::GetSingleton()) {
				if (a_resizeToScreenRes) {
					auto height = renderer->data.renderWindows[0].windowHeight;
					auto width = renderer->data.renderWindows[0].windowWidth;

					if (height != image->GetMetadata().height && height != image->GetMetadata().width) {
						DirectX::ScratchImage tmpImage;
						DirectX::Resize(*image->GetImage(0, 0, 0), width, height, DirectX::TEX_FILTER_CUBIC, tmpImage);

						image.reset();	// is this needed
					    image = std::make_shared<DirectX::ScratchImage>(std::move(tmpImage));
					}
				}

				ComPtr<ID3D11Resource> pTexture{};
				hr = DirectX::CreateTexture(renderer->data.forwarder, image->GetImages(), 1, image->GetMetadata(), &pTexture);

				if (SUCCEEDED(hr)) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = image->GetMetadata().format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					hr = renderer->data.forwarder->CreateShaderResourceView(pTexture.Get(), &srvDesc, &srView);
					result = SUCCEEDED(hr);
				}

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				pTexture.Reset();
			}
		}

		return result;
	}

	std::string Sanitize(std::string& a_path)
	{
		a_path = clib_util::string::tolower(a_path);

		a_path = srell::regex_replace(a_path, srell::regex("/+|\\\\+"), "\\");
		a_path = srell::regex_replace(a_path, srell::regex("^\\\\+"), "");
		a_path = srell::regex_replace(a_path, srell::regex(R"(.*?[^\s]textures\\|^textures\\)", srell::regex::icase), "");

		return a_path;
	}

	void AlphaBlendImage(const DirectX::Image* baseImg, const DirectX::Image* overlayImg, DirectX::ScratchImage& a_outImage, float intensity)
	{
		auto hr = a_outImage.InitializeFromImage(*baseImg);
		if (FAILED(hr)) {
			return;
		}

		const auto resultImage = a_outImage.GetImages();

		const std::size_t width = baseImg->width;
		const std::size_t height = baseImg->height;
		const std::size_t pixelSize = DirectX::BitsPerPixel(baseImg->format) / 8;

		auto processRows = [&](const std::size_t startRow, const std::size_t endRow) {
			for (std::size_t y = startRow; y < endRow; y++) {
				std::uint8_t*       resultPixel = resultImage->pixels + (y * resultImage->rowPitch);
				const std::uint8_t* basePixel = baseImg->pixels + (y * baseImg->rowPitch);
				const std::uint8_t* overlayPixel = overlayImg->pixels + (y * overlayImg->rowPitch);

				for (std::size_t x = 0; x < width; x++) {
					if (const float overlayAlpha = (overlayPixel[x * pixelSize + 3] / 255.0f) * intensity; overlayAlpha > 0.0f) {
						const float baseAlpha = 1.0f - overlayAlpha;

						for (std::size_t i = 0; i < pixelSize - 1; i++) {
							float blendedValue = (overlayPixel[x * pixelSize + i] * overlayAlpha) + (basePixel[x * pixelSize + i] * baseAlpha);
							resultPixel[x * pixelSize + i] = static_cast<std::uint8_t>(std::round(std::min(blendedValue, 255.0f)));
						}
					}
				}
			}
		};

		const auto numThreads = std::thread::hardware_concurrency();

		std::vector<std::jthread> threads;
		threads.reserve(numThreads);

		const std::size_t rowsPerThread = height / numThreads;

		for (std::size_t i = 0; i < numThreads; ++i) {
			std::size_t startRow = i * rowsPerThread;
			std::size_t endRow = (i + 1) * rowsPerThread;

			threads.emplace_back(std::jthread(processRows, startRow, endRow));
		}

		for (auto& thread : threads) {
			thread.join();
		}
	}

	// https://www.codeproject.com/Articles/471994/OilPaintEffect
	// https://github.com/aarizhov/DFPerformanceMeter/blob/master/Examples/iOS%20Language%20Performance%20Example/CPU/PureC/OilPaintingC.m
	bool OilPaintingFilter(const DirectX::Image* a_srcImage, const std::int32_t a_radius, const float a_intensity, DirectX::ScratchImage& a_outImage)
	{
		auto hr = a_outImage.InitializeFromImage(*a_srcImage);
		if (FAILED(hr)) {
			return false;
		}

		const auto intensityFactor = 255.0f / std::clamp(a_intensity, 0.0f, 255.0f);

		const std::uint8_t* inPixels = a_srcImage->pixels;
		std::uint8_t*       outPixels = a_outImage.GetPixels();

		const auto& height = a_srcImage->height;
		const auto& width = a_srcImage->width;
		const auto& bytesInARow = a_srcImage->rowPitch;

		auto processRows = [&](const std::size_t startRow, const std::size_t endRow) {
			std::array<std::int32_t, 256> intensityCount{ 0 };
			std::array<std::int32_t, 256> avgR{ 0 };
			std::array<std::int32_t, 256> avgG{ 0 };
			std::array<std::int32_t, 256> avgB{ 0 };

			auto currRowOffset = (startRow * bytesInARow);
			for (auto currRow = startRow; currRow < endRow; currRow++) {
				for (auto currColumn = 0; currColumn < width; currColumn++) {
					// Reset calculations of last pixel.
					intensityCount.fill(0);
					avgR.fill(0);
					avgG.fill(0);
					avgB.fill(0);

					const std::int32_t minY = std::max(-a_radius, -static_cast<std::int32_t>(currRow));
					const std::int32_t maxY = std::min(a_radius, static_cast<std::int32_t>(height - currRow - 1));
					const std::int32_t minX = std::max(-a_radius, -currColumn);
					const std::int32_t maxX = std::min(a_radius, static_cast<std::int32_t>(width - currColumn - 1));

					// Grab luminance values of neighbouring pixels
					for (std::int32_t offsetY = minY; offsetY <= maxY; offsetY++) {
						for (std::int32_t offsetX = minX; offsetX <= maxX; offsetX++) {
							const std::int32_t offset = ((currColumn + offsetX) << 2) + (currRow + offsetY) * bytesInARow;

							const std::uint32_t R = inPixels[offset];
							const std::uint32_t G = inPixels[offset + 1];
							const std::uint32_t B = inPixels[offset + 2];

							// Find intensity of RGB value and apply intensity level.
							const auto currIntensity = static_cast<std::int32_t>(((R + G + B) / 3) / intensityFactor);

							intensityCount[currIntensity]++;
							avgR[currIntensity] += R;
							avgG[currIntensity] += G;
							avgB[currIntensity] += B;
						}
					}

					// Find max intensity
					const std::int32_t maxIntensityIndex = std::distance(intensityCount.begin(), std::ranges::max_element(intensityCount));
					const std::int32_t currMaxIntensityCount = intensityCount[maxIntensityIndex];

					const auto offset = (currColumn << 2) + currRowOffset;
					outPixels[offset] = avgR[maxIntensityIndex] / currMaxIntensityCount;
					outPixels[offset + 1] = avgG[maxIntensityIndex] / currMaxIntensityCount;
					outPixels[offset + 2] = avgB[maxIntensityIndex] / currMaxIntensityCount;
				}
				currRowOffset += bytesInARow;
			}
		};

		const auto numThreads = std::thread::hardware_concurrency();

		std::vector<std::jthread> threads;
		threads.reserve(numThreads);

		const std::size_t rowsPerThread = height / numThreads;

		for (std::size_t i = 0; i < numThreads; ++i) {
			std::size_t startRow = i * rowsPerThread;
			std::size_t endRow = (i + 1) * rowsPerThread;

			threads.emplace_back(std::jthread(processRows, startRow, endRow));
		}

		for (auto& thread : threads) {
			thread.join();
		}

		return true;
	}

	void CompressTexture(const RE::BSGraphics::Renderer* a_this, const DirectX::ScratchImage& a_inputImage, DirectX::ScratchImage& a_outputImage)
	{
		// Compress texture
		const ComPtr<ID3D11Device> device{ a_this->data.forwarder };

		auto hr = DirectX::Compress(device.Get(), a_inputImage.GetImages(), 1, a_inputImage.GetMetadata(),
			DXGI_FORMAT_BC7_UNORM,
			DirectX::TEX_COMPRESS_BC7_QUICK,
			0.0f,
			a_outputImage);
		if (FAILED(hr)) {
			logger::info("Failed to compress dds");
		}
	}

	void SaveToDDS(const DirectX::ScratchImage& a_inputImage, std::string_view a_path)
	{
		// Save texture
		const auto wPath = stl::utf8_to_utf16(a_path);
		auto       hr = DirectX::SaveToDDSFile(a_inputImage.GetImages(), 1, a_inputImage.GetMetadata(), DirectX::DDS_FLAGS_NONE, wPath->c_str());
		if (FAILED(hr)) {
			logger::info("Failed to save dds");
		}
	}

	void SaveToPNG(const DirectX::ScratchImage& a_inputImage, std::string_view a_path)
	{
		// Save texture
		const auto wPath = stl::utf8_to_utf16(a_path);
		auto       hr = DirectX::SaveToWICFile(*a_inputImage.GetImage(0, 0, 0), DirectX::WIC_FLAGS_FORCE_SRGB,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), wPath->c_str());
		if (FAILED(hr)) {
			logger::info("Failed to save png");
		}
	}
}

std::string Mesh::Sanitize(std::string& a_path)
{
	a_path = clib_util::string::tolower(a_path);

	a_path = srell::regex_replace(a_path, srell::regex("/+|\\\\+"), "\\");
	a_path = srell::regex_replace(a_path, srell::regex("^\\\\+"), "");
	a_path = srell::regex_replace(a_path, srell::regex(R"(.*?[^\s]meshes\\|^meshes\\)", srell::regex::icase), "");

	return a_path;
}
