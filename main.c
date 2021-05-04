#include <ft2build.h>
#include FT_FREETYPE_H
#include "bitmap.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

FT_Library ft;

#define WIDTH   2048
#define HEIGHT  2048
#define PADDING 15

Bitmap1B bmp1;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct Character {
	uint32_t width, height;
	uint32_t left, top;
	uint32_t advance;
} Character;


void draw_border(Bitmap1B bmp, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
	for(uint32_t j = 0; j < height; j++)
	for(uint32_t i = 0; i < width; i++) {
		uint8_t* c = bitmap1b_get_pixel(bmp, x+i, y+j);
		*c = 255;
	}
}

void my_draw_bitmap(FT_Bitmap* bitmap, uint8_t* unpacked, int x, int y);
Bitmap1B unpack_mono_bitmap(FT_Bitmap bitmap);
Bitmap1B calc_distances(uint32_t width, uint32_t height, Bitmap1B unpacked);
void bmp1b_draw_bmp1b(Bitmap1B dst, Bitmap1B src, uint32_t x, uint32_t y);


#define CHAR_START 33
#define CHAR_END 127
#define CHAR_COUNT (CHAR_END - CHAR_START)
int main(int argc, char *argv[]) {
	if ( argc == 1) {
		printf("missing file name");
		return 1;
	}

	bmp1 = bitmap1b_new(WIDTH, HEIGHT);

	memset(bmp1.data, 0, bmp1.width * bmp1.height);
	
	Character characters[CHAR_COUNT] = { 0 };

	char* filename = argv[1];
	char* text = "abcdefg";
	size_t num_chars = strlen(text);
	FT_Error error;

	FILE* data_fp = fopen("data.txt", "w");


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

	error = FT_Set_Pixel_Sizes(face, 0, 250);
	if (error) {
		printf("failed to set char size");
		return 1;
	}

	FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */
	int n, i;

	int pen_x = 0;
	int pen_y = 250;

	uint32_t row_height = 0;
	const int shrink_factor = 2;


	for (i = 0, n = CHAR_START; n < CHAR_END; n++, i++ )
	{
		FT_UInt  glyph_index;



		/* retrieve glyph index from character code */
		glyph_index = FT_Get_Char_Index( face, n );

		/* load glyph image into the slot (erase previous one) */
		error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
		if ( error )
			return 0;

		error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_MONO );
		if ( error )
			return 0;

		int32_t left = face->glyph->bitmap_left;
		int32_t top = face->glyph->bitmap_top;
		int32_t width = slot->bitmap.width + PADDING * 2;
		int32_t height = slot->bitmap.rows + PADDING * 2;
		uint32_t advance = (face->glyph->advance.x >> 6) + PADDING;

		if ( pen_x + advance > WIDTH) {
			pen_x = 0;
			pen_y += row_height + PADDING * 2;
			row_height = 0;
		}
		Bitmap1B unpacked = unpack_mono_bitmap(slot->bitmap);
		Bitmap1B dist = calc_distances(slot->bitmap.width, slot->bitmap.rows, unpacked);

		row_height = MAX(row_height, height);
		bmp1b_draw_bmp1b(bmp1, dist, pen_x + left, pen_y - top);


		characters[i] = (Character){
			.width= width >> shrink_factor,
			.height= height >> shrink_factor,
			.left= left >> shrink_factor,
			.top= top >> shrink_factor,
			// we need to bitshift to get width in pixels
			.advance= advance >> shrink_factor,
		};

		//fprintf(data_fp, "%i %i %i %i %i %i\n", n, width, height, left, top, advance);

	

		bitmap1b_free(unpacked);
		bitmap1b_free(dist);

		pen_x += advance;
	}
	bitmap1b_write(bmp1, "full_res.bmp");

	fclose(data_fp);

	Bitmap1B shrink = bitmap1b_linear_shrink(bmp1, shrink_factor);

	for(uint32_t i = 0; i < CHAR_COUNT; i++) {
		Character c = characters[i];
		//draw_border(shrink, c.left, c.top, c.width, c.height);
	}
	
	bitmap1b_write(shrink, "text.bmp");

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

			*dst_px = MAX(*src_px, *dst_px);
		}
	}
}


/*
calculate the distance field
basically a blur pass,
one set of horizontal and vertical passes for the outside of the character,
and one for the inside

*/

Bitmap1B calc_distances(uint32_t src_width, uint32_t src_height, Bitmap1B unpacked) {

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
		uint32_t total = (255 - outside[i])/2 + src[i]/2;
		dst[i] = MIN(total, 255);
	}
	
	return (Bitmap1B){.height=height, .width=width, .data=dst};
}