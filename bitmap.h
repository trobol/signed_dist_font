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

uint8_t* bitmap1b_get_pixel(Bitmap1B bmp, uint32_t x, uint32_t y) {
	return bmp.data + x + (bmp.width * y); 
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
	uint32_t width;
	uint32_t height;
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
		.height=height,
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
	for(uint32_t i = height; i > 0; i--) {
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
	__bitmap_write(path, pixel_data, bmp.width, bmp.height, 3);
}
