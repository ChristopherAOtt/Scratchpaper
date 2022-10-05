#pragma once

#include "Primitives.hpp"
#include "Types.hpp"

#include <cassert>

class Image{
	/*
	NOTE: At the moment, all channels are 1 byte.
	TODO: Add support for more color depth.
	*/

	public:
		enum PixelType{
			PIXELTYPE_INVALID    = 0,

			PIXELTYPE_MONOCHROME = 1,
			PIXELTYPE_RGB        = 2,
			PIXELTYPE_RGBA       = 3,
		};
		constexpr static Int32 MIN_VALID_PIXELTYPE = 1;
		constexpr static Int32 MAX_VALID_PIXELTYPE = 3;

		struct PixelMonochrome{
			Uint8 colors[1];
		};

		struct PixelRGB{
			Uint8 colors[3];
		};

		struct PixelRGBA{
			Uint8 colors[4];
		};

		constexpr static Int64 ARBITRARY_MAX_IMAGE_SIZE = 2 << 25;
		constexpr static Int32 PIXEL_TYPE_SIZES[] = {
			0,  // PIXELTYPE_INVALID

			1,
			3,
			4,
		};

		constexpr static Int32 MAX_SATURATION_VALUES[] = {
			0,    // PIXELTYPE_INVALID

			255,  // PIXELTYPE_MONOCHROME
			255,  // PIXELTYPE_RGB
			255,  // PIXELTYPE_RGBA
		};

		struct Settings{
			PixelType type;
			IVec2 dimensions;
			bool initial_clear{true};
		};

	public:
		Image();
		Image(Settings settings);
		~Image();

		// Image info
		PixelType type() const;
		IVec2 dimensions() const;
		Int32 maxSaturation() const;
		void resize(IVec2 new_dims);

		// Pixel manipulation
		PixelMonochrome& pixelMonochrome(Int64 pixel_index);
		PixelRGB&        pixelRGB(Int64 pixel_index);
		PixelRGBA&       pixelRGBA(Int64 pixel_index);
		void* dataPtr();

	private:
		void init(Settings settings);
		void assertPixelIndexInBounds(Int64 pixel_index, PixelType type, 
			Int64 num_pixels=1);

	private:
		PixelType m_type;
		IVec2 m_dimensions;
		
		Int64 m_num_bytes;
		void* m_data_ptr;
};
