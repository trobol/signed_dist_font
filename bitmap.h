#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct __attribute__ ((packed)) Color {
	uint8_t b, g, r;
} Color;

typedef struct Bitmap1B {
	uint32_t width;
	uint32_t height;
	uint8_t* data;
} Bitmap1B;

Bitmap1B bitmap1b_new(uint32_t width, uint32_t height) {
	uint8_t* data = malloc(width * height);
	return (Bitmap1B){.width=width, .height=height, .data=data};
}

void bitmap1b_free(Bitmap1B bmp) {
	free(bmp.data);
}

#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )

uint8_t* bitmap1b_get_pixel(Bitmap1B bmp, uint32_t x, uint32_t y) {
	return bmp.data + MIN(x, bmp.width) + (bmp.width * MIN(y, bmp.height)); 
}

typedef struct Bitmap3B {
	uint32_t width, height;
	union {
		void* data;
		Color* color;
	};
	uint16_t bits_per_color;
	uint32_t line_size;
} Bitmap3B;

Color* bmp_get_pixel(Bitmap3B bmp, uint32_t x, uint32_t y) {
	return bmp.color + x + (bmp.width * y);
}

Bitmap3B bitmap3b_new(uint32_t width, uint32_t height) {
	uint32_t bmp_size = width * height;

	void* bmp = malloc(bmp_size * sizeof(Color));
	return (Bitmap3B){.width=width, .height=height, .data=bmp};
}


typedef struct __attribute__ ((packed)) BMP_Header {
	uint16_t id;
	uint32_t size;
	uint32_t unused;
	uint32_t offset;
} BMP_Header;

typedef struct __attribute__ ((packed)) DIB_Header {
	uint32_t bytes;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bits;
	uint32_t compression;
	uint32_t raw_size;
	uint32_t res_width;
	uint32_t res_height;
	uint32_t colors;
	uint32_t imp_colors;
} DIB_Header;


void write_bitmap_header(FILE* fp, uint32_t width, uint32_t height, uint16_t bits, uint32_t raw_size) {
	
	uint32_t header_bytes = sizeof(BMP_Header) + sizeof(DIB_Header);

	BMP_Header bmp_header = (BMP_Header) {
		.id=0x4D42,
		.size= header_bytes + raw_size,
		.unused = 0,
		.offset = header_bytes
	};

	DIB_Header dib_header = (DIB_Header){
		.bytes=sizeof(DIB_Header),
		.width=width,
		.height=-height,
		.planes=1,
		.bits=bits,
		.compression=0,
		.raw_size=raw_size,
		.res_width=2835,
		.res_height=2835,
		.colors=0,
		.imp_colors=0,
	};

	

	fwrite(&bmp_header, sizeof(bmp_header), 1, fp);
	fwrite(&dib_header, sizeof(DIB_Header), 1, fp);
}

void bitmap1b_shrink_pass(Bitmap1B src) {

	for (uint32_t y = 0; y < src.height; y++)
	for (uint32_t x = 0; x < src.width; x+=2) {

		uint32_t a = *bitmap1b_get_pixel(src, x, y);
		uint32_t b = *bitmap1b_get_pixel(src, x+1, y);

		*bitmap1b_get_pixel(src, x/2, y) = (a+b) / 2;
	}
	

	
	for (uint32_t y = 0; y < src.height; y+=2)
	for (uint32_t x = 0; x < src.width; x++) {

		uint32_t a = *bitmap1b_get_pixel(src, x, y);
		uint32_t b = *bitmap1b_get_pixel(src, x, y+1);

		*bitmap1b_get_pixel(src, x, y/2) = (a+b) / 2;
	}
	
}

Bitmap1B bitmap1b_crop(Bitmap1B bmp, uint32_t width, uint32_t height) {
	Bitmap1B result = bitmap1b_new(width, height);
	for(uint32_t y = 0; y < height; y++)
	for(uint32_t x = 0; x < width; x++) {
		*bitmap1b_get_pixel(result, x, y) = *bitmap1b_get_pixel(bmp, x, y);
	}

	return result; 
}


// must be a power of 2
Bitmap1B bitmap1b_linear_shrink(Bitmap1B bmp, uint32_t itr) {
	for(uint32_t i = 0; i < itr; i++) {
		bitmap1b_shrink_pass(bmp);
	}
	uint32_t width = bmp.width >> itr;
	uint32_t height = bmp.height >> itr;
	Bitmap1B cropped = bitmap1b_crop(bmp, width, height);
	bitmap1b_free(bmp);
	return cropped;
}

void __bitmap_write(const char* path, uint8_t* data, uint32_t width, uint32_t height, uint8_t bytes) {
	FILE* fp;
	fp = fopen(path, "wb");

	
	uint32_t data_line_size = width*bytes;

	uint32_t padding = (4 - (data_line_size % 4)) % 4;

	uint32_t line_size = data_line_size+padding;

	uint32_t raw_size = line_size*height;

	write_bitmap_header(fp, width, height, bytes*8, raw_size);

	const char* null = "null";
	// write data in inverse order (thats how bitmaps work)
	for(uint32_t i = 0; i < height; i++) {
		uint8_t* line_start = data + (i * width * bytes);
		fwrite(line_start, data_line_size, 1, fp);
		fwrite(null, padding, 1, fp);
	}

	fclose(fp);
}

void bitmap3b_write(Bitmap3B bmp, const char* path) {	
	__bitmap_write(path, bmp.data, bmp.width, bmp.height, 3);
}


void bitmap1b_write(Bitmap1B bmp, const char* path) {
	uint32_t bmp_size = bmp.width * bmp.height;
	Color* pixel_data = malloc(bmp_size * sizeof(Color));
	for(uint32_t i = 0; i < bmp_size; i++) {
		pixel_data[i].r = pixel_data[i].g = pixel_data[i].b = bmp.data[i];
	}
	__bitmap_write(path, (uint8_t*)pixel_data, bmp.width, bmp.height, 3);
}
