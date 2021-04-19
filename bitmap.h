#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct Color {
	uint8_t a, r, g, b;
} Color;

typedef struct Bitmap {
	uint32_t width, height;
	union {
		void* data;
		Color* color;
	};
	uint16_t bits_per_color;
	uint32_t line_size;
} Bitmap;

Color* bmp_get_pixel(Bitmap bmp, uint32_t x, uint32_t y) {
	return bmp.color + x + (bmp.width * (bmp.height - y));
}

Bitmap new_bitmap(uint32_t width, uint32_t height) {

	uint32_t bmp_size = width * height;

	void* bmp = malloc(bmp_size * sizeof(Color));


	return (Bitmap){.width=width, .height=height, .data=bmp};
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


void write_bitmap(Bitmap* bmp, const char* path) {
	FILE* fp;

	uint32_t bmp_size = bmp->width * bmp->height;
	fp = fopen(path, "w");

	uint32_t header_bytes = sizeof(BMP_Header) + sizeof(DIB_Header);

	BMP_Header bmp_header = (BMP_Header) {
		.id=0x4D42,
		.size= header_bytes + bmp_size,
		.unused = 0,
		.offset = header_bytes
	};

	DIB_Header dib_header = (DIB_Header){
		.bytes=sizeof(DIB_Header),
		.width=bmp->width,
		.height=bmp->height,
		.planes=1,
		.bits=32,
		.compression=0,
		.raw_size=bmp_size,
		.res_width=2835,
		.res_height=2835,
		.colors=0,
		.imp_colors=0
	};


	fwrite(&bmp_header, 1, sizeof(bmp_header), fp);
	fwrite(&dib_header, 1, sizeof(DIB_Header), fp);

	fwrite(bmp->data, 4, bmp_size, fp);

}