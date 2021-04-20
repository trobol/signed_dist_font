#include <ft2build.h>
#include FT_FREETYPE_H
#include "bitmap.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

FT_Library ft;

#define WIDTH   1200
#define HEIGHT  400
#define PADDING 20
Bitmap3B bmp;
Bitmap1B bmp1;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void my_draw_bitmap(FT_Bitmap* bitmap, uint8_t* unpacked, int x, int y);
Bitmap1B unpack_mono_bitmap(FT_Bitmap bitmap);
uint8_t* calc_distances(uint32_t width, uint32_t height, Bitmap1B unpacked);
void bmp1b_draw_bmp1b(Bitmap1B dst, Bitmap1B src, uint32_t x, uint32_t y);

int main(int argc, char *argv[]) {
	if ( argc == 1) {
		printf("missing file name");
		return 1;
	}

	bmp = bitmap3b_new(WIDTH, HEIGHT);
	bmp1 = bitmap1b_new(WIDTH, HEIGHT);

	memset(bmp.data, 0, bmp.width * bmp.height);
	memset(bmp1.data, 0, bmp1.width * bmp1.height);
	

	char* filename = argv[1];
	char* text = "abcdefg";
	size_t num_chars = strlen(text);
	FT_Error error;

	error = FT_Init_FreeType( &ft );
	if ( error ) {
		printf("failed to init freetype");
		return 1;
	}

	FT_Face face;
	error = FT_New_Face(ft, filename, 0, &face);
	if (error)
	{
		printf("failed to load font");
		return 1;
	}

	error = FT_Set_Pixel_Sizes(face, 0, 300);
	if (error) {
		printf("failed to set char size");
		return 1;
	}

	FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
	int           pen_x, pen_y, n;

	pen_x = 0;
	pen_y = 0;

	for ( n = 0; n < num_chars; n++ )
	{
	FT_UInt  glyph_index;


	/* retrieve glyph index from character code */
	glyph_index = FT_Get_Char_Index( face, text[n] );

	/* load glyph image into the slot (erase previous one) */
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
	if ( error )
		return 0;

	error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_MONO );
	if ( error )
		return 0;

	Bitmap1B unpacked = unpack_mono_bitmap(slot->bitmap);
	uint8_t* dist = calc_distances(slot->bitmap.width, slot->bitmap.rows, unpacked);

	bmp1b_draw_bmp1b(bmp1, unpacked, pen_x, pen_y);
	/* now, draw to our target surface */
	my_draw_bitmap( &slot->bitmap, dist, pen_x, pen_y );

		free(unpacked.data);
		pen_x += slot->bitmap.width + PADDING * 2;
	}

	bitmap3b_write(bmp, "text.bmp");
	bitmap1b_write(bmp1, "text1b.bmp");

	return 0;
}

/* convert FreeType glyph bitmap to a simple 0|1 bitmap */
Bitmap1B unpack_mono_bitmap(FT_Bitmap bitmap)
{

	int y, byte_index, num_bits_done, rowstart, bits, bit_index;
	uint8_t byte_value;
	
	Bitmap1B result = bitmap1b_new(bitmap.width, bitmap.rows);

	for (y = 0; y < bitmap.rows; y++) {
		for (byte_index = 0; byte_index*8 < bitmap.width; byte_index++) {
			
			byte_value = bitmap.buffer[y * bitmap.pitch + byte_index];
			
			num_bits_done = byte_index * 8;
			

			rowstart = y * bitmap.width + byte_index * 8;
			
			
			bits = 8;
			if ((bitmap.width - num_bits_done) < 8) {
				bits = bitmap.width - num_bits_done;
			}
			
			for (bit_index = 0; bit_index < bits; bit_index++) {
				
				uint8_t bit = byte_value >> (7-bit_index) & 1;

				result.data[rowstart + bit_index] = bit * 255;
			}
		}
	}
	
	return result;
}


void my_draw_bitmap(FT_Bitmap* bitmap, uint8_t* unpacked, int x, int y) {
	FT_Int  i, j, p, q;
	uint32_t width = bitmap->width + PADDING*2;
	uint32_t height = bitmap->rows + PADDING*2;

	FT_Int  x_max = MIN(x + width, WIDTH);
	FT_Int  y_max = MIN(y + height, HEIGHT);


	for ( j = y, q = 0; j < y_max; j++, q++ )
	{
		for ( i = x, p = 0; i < x_max; i++, p++ )
		{
		
		Color* color = bmp_get_pixel(bmp, i, j);

		uint8_t val = unpacked[q * width + p];	
		
		color->r = color->g = color->b = val;
		}
	}
}

void vert_pass(uint32_t width, uint32_t height, uint8_t* src, uint8_t* dst, uint32_t delta) {
	
	for(uint32_t x = 0; x < width; x++)
	for(uint32_t y = 0; y < height; y++) {
		uint32_t src_val = src[y * width + x];
	
				
		uint32_t a = 255;
		uint32_t b = 255;

		if (y > 0) a = src[(y-1) * width + x];
		if (y < height-1) b = src[(y+1) * width + x];

		uint32_t val = MIN(a, b) + delta;


		dst[y * width + x] = val < src_val ? val : src_val;

	}
}

void horz_pass(uint32_t width, uint32_t height, uint8_t* src, uint8_t* dst, uint32_t delta) {
	for(uint32_t y = 0; y < height; y++)
	for(uint32_t x = 0; x < width; x++) {
		uint32_t src_val = src[y * width + x];
		
		
		uint32_t a = 255;
		uint32_t b = 255;

		if (x > 0) a = src[y * width + (x-1)];
		if (x < width-1) b = src[y * width + (x+1)];

		uint32_t val = MIN(a, b)+delta;

		

		dst[y * width + x] = MIN(val, src_val);
	}
}


void bmp1b_draw_bmp1b(Bitmap1B dst, Bitmap1B src, uint32_t x, uint32_t y) {
	uint32_t i, j, p, q;


	uint32_t x_max = MIN(x + src.width, dst.width);
	uint32_t y_max = MIN(y + src.height, dst.height);


	for ( j = y, q = 0; j < y_max; j++, q++ )
	{
		for ( i = x, p = 0; i < x_max; i++, p++ )
		{
		
			uint8_t* src_px = bitmap1b_get_pixel(src, p, q);
			uint8_t* dst_px = bitmap1b_get_pixel(dst, i, j);

			*dst_px = *src_px;
		}
	}
}

uint8_t* calc_distances(uint32_t src_width, uint32_t src_height, Bitmap1B unpacked) {

	int32_t i, j;
	uint32_t x_dist, y_dist;
	uint32_t sqr_dist;


	uint32_t width = src_width + PADDING*2;
	uint32_t height = src_height + PADDING*2;

	uint32_t buffer_size = width * height;

	uint8_t* src = malloc(buffer_size);
	uint8_t* dst = malloc(buffer_size);

	
	memset(src, 255, buffer_size);

	for(uint32_t y = 0; y < src_height; y++)
	for(uint32_t x = 0; x < src_width; x++) {
		src[(y+PADDING) * width + (x+PADDING)] = unpacked.data[y * src_width + x] ? 0 : 255;
	}

	
	for(uint32_t d = 0; d < 128; d++) {
		horz_pass(width, height, src, dst, 1+d*2);
		uint8_t* tmp = dst;
		dst = src;
		src = tmp;
	}
	
	for(uint32_t d = 0; d < 128; d++) {
		vert_pass(width, height, src, dst, 1+d*2);
		uint8_t* tmp = dst;
		dst = src;
		src = tmp;
	}

	uint8_t* outside = src;

	src = malloc(buffer_size);
	
	memset(src, 0, buffer_size);

	for(uint32_t y = 0; y < src_height; y++)
	for(uint32_t x = 0; x < src_width; x++) {
		src[(y+PADDING) * width + (x+PADDING)] = unpacked.data[y * src_width + x] ? 255 : 0;
	}
	
	
	for(uint32_t d = 0; d < 128; d++) {
		horz_pass(width, height, src, dst, 1+d*2);
		uint8_t* tmp = dst;
		dst = src;
		src = tmp;
	}
	
	for(uint32_t d = 0; d < 128; d++) {
		vert_pass(width, height, src, dst, 1+d*2);
		uint8_t* tmp = dst;
		dst = src;
		src = tmp;
	}
	
	for(uint32_t i = 0; i < buffer_size; i++) {
		uint32_t total = (255 - outside[i])/2 + src[i];
		dst[i] = MIN(total, 255);
	}
	
	return src;
}