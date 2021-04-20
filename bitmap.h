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
	uint32_t pitch;
	uint32_t width, height;
	union {
		void* data;
		Color* color;
	};
	uint16_t bits_per_color;
	uint32_t line_size;
} Bitmap3B;

Color* bmp_get_pixel(Bitmap3B bmp, uint32_t x, uint32_t y) {
	return bmp.data + (x*3) + (bmp.pitch * (bmp.height - y));
}

Bitmap3B bitmap3b_new(uint32_t width, uint32_t height) {
	uint32_t pitch = (width*3);
	if (pitch % 4) pitch++;
	uint32_t bmp_size = pitch * height;

	void* bmp = malloc(bmp_size * sizeof(Color));


	return (Bitmap3B){.width=width, .height=height, .pitch=pitch, .data=bmp};
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

	

	fwrite(&bmp_header, 1, sizeof(bmp_header), fp);
	fwrite(&dib_header, 1, sizeof(DIB_Header), fp);
}

void bitmap3b_write(Bitmap3B bmp, const char* path) {
	FILE* fp;

	
	fp = fopen(path, "w");

	uint32_t bmp_size = bmp.pitch * bmp.height;

	write_bitmap_header(fp, bmp.width, bmp.height, 24, bmp_size);


	fwrite(bmp.data, 1, bmp_size, fp);

}

void bitmap1b_write(Bitmap1B bmp, const char* path) {
	FILE* fp;
	
	fp = fopen(path, "w");

	uint32_t bmp_size = bmp.width * bmp.height;

	write_bitmap_header(fp, bmp.width, bmp.height, 8, bmp_size);

	fwrite(bmp.data, 1, bmp_size, fp);
}


void bitmap1b_flip(Bitmap1B bmp) {

	uint8_t* flipped_data = malloc(bmp.width * bmp.height);

	for(uint32_t y = 0; y < bmp.height; y++) {
		uint32_t read_offset = y * bmp.width;
		uint32_t write_offset = bmp.height - read_offset;
		memcpy(flipped_data + write_offset, bmp.data+read_offset, bmp.width);
	}

	free(bmp.data);
	bmp.data = flipped_data;
}