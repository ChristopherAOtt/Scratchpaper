#include "Image.hpp"

Image::Image(Image::Settings settings){
	assert(settings.type >= MIN_VALID_PIXELTYPE && settings.type <= MAX_VALID_PIXELTYPE);
	Int64 num_pixels = settings.dimensions.x * settings.dimensions.y;
	Int64 num_bytes = num_pixels * PIXEL_TYPE_SIZES[settings.type];
	
	if(!(num_bytes < ARBITRARY_MAX_IMAGE_SIZE && num_bytes > 0)){
		printf("NumBytes %li\n", num_bytes);
		assert(false);
	}
	
	m_type = settings.type;
	m_dimensions = settings.dimensions;
	m_num_bytes = num_bytes;
	m_data_ptr = malloc(num_bytes);
	if(settings.initial_clear){
		bzero(m_data_ptr, num_bytes);
	}
}

Image::~Image(){
	free(m_data_ptr);
}

Image::PixelType Image::type() const{
	return m_type;
}

IVec2 Image::dimensions() const{
	return m_dimensions;
}

Int32 Image::maxSaturation() const{
	/*
	Saturation is defined on a per-chanel basis.
	*/

	assert(m_type >= MIN_VALID_PIXELTYPE && m_type <= MAX_VALID_PIXELTYPE);
	return MAX_SATURATION_VALUES[m_type];
}

Image::PixelMonochrome& Image::pixelMonochrome(Int64 pixel_index){
	assert(m_type == PIXELTYPE_MONOCHROME);
	assertPixelIndexInBounds(pixel_index, PIXELTYPE_MONOCHROME);
	PixelMonochrome* cast_ptr = (PixelMonochrome*) m_data_ptr;
	return cast_ptr[pixel_index];
}

Image::PixelRGB& Image::pixelRGB(Int64 pixel_index){
	assert(m_type == PIXELTYPE_RGB);
	assertPixelIndexInBounds(pixel_index, PIXELTYPE_RGB);
	PixelRGB* cast_ptr = (PixelRGB*) m_data_ptr;
	return cast_ptr[pixel_index];
}

Image::PixelRGBA& Image::pixelRGBA(Int64 pixel_index){
	assert(m_type == PIXELTYPE_RGBA);
	assertPixelIndexInBounds(pixel_index, PIXELTYPE_RGBA);
	PixelRGBA* cast_ptr = (PixelRGBA*) m_data_ptr;
	return cast_ptr[pixel_index];
}

void Image::assertPixelIndexInBounds(Int64 pixel_index, PixelType type, 
	Int64 num_pixels){
	/*

	*/

	assert(num_pixels > 0);
	assert(pixel_index >= 0);
	assert(type >= MIN_VALID_PIXELTYPE && type <= MAX_VALID_PIXELTYPE);
	
	Int64 pixel_size = PIXEL_TYPE_SIZES[type];
	Int64 index_start_byte = pixel_size * pixel_index;
	Int64 num_bytes_covered = pixel_size * num_pixels;
	Int64 index_end_byte = index_start_byte + num_bytes_covered;
	assert(index_end_byte <= m_num_bytes);
}
