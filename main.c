#include <ft2build.h>
#include FT_FREETYPE_H
#include "bitmap.h"

#include <stdio.h>
#include <string.h>


FT_Library ft;

#define WIDTH   640
#define HEIGHT  480
Bitmap bmp;

void my_draw_bitmap(FT_Bitmap* bitmap, uint8_t* unpacked, int x, int y);
uint8_t* unpack_mono_bitmap(FT_Bitmap bitmap);
uint8_t* calc_distances(FT_Bitmap* bitmap, uint8_t* unpacked);

int main(int argc, char *argv[]) {
	if ( argc == 1) {
		printf("missing file name");
		return 1;
	}

	bmp = new_bitmap(WIDTH, HEIGHT);

	memset(bmp.data, 0, bmp.width * bmp.height * 4);
	

	char* filename = argv[1];
	char* text = "hello";
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

	error = FT_Set_Pixel_Sizes(face, 0, 100);
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
	error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT );
	if ( error )
		return 0;

	error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_MONO );
	if ( error )
		return 0;

	uint8_t* unpacked = unpack_mono_bitmap(slot->bitmap);
	uint8_t* dist = calc_distances(&slot->bitmap, unpacked);
	/* now, draw to our target surface */
	my_draw_bitmap( &slot->bitmap, dist,
					pen_x,
					pen_y );

		free(unpacked);
		pen_x += 100;
	}
	write_bitmap(&bmp, "bitmap.bmp");

	return 0;
}

/* convert FreeType glyph bitmap to a simple 0|1 bitmap */
uint8_t* unpack_mono_bitmap(FT_Bitmap bitmap)
{
	unsigned char* result;
	int y, x, byte_index, num_bits_done, rowstart, bits, bit_index;
	unsigned char byte_value;
	
	result = malloc(bitmap.rows * bitmap.width);

	
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
				int bit;
				bit = byte_value & (1 << (7 - bit_index));
				
				result[rowstart + bit_index] = bit;
			}
		}
	}
	
	return result;
}


void my_draw_bitmap(FT_Bitmap* bitmap, uint8_t* unpacked, int x, int y) {
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;


	for ( j = y, q = 0; j < y_max; j++, q++ )
	{
		for ( i = x, p = 0; i < x_max; i++, p++ )
		{
		
		Color* color = bmp_get_pixel(bmp, i, j);

		color->r = color->g = color->b = color->a = unpacked[q * bitmap->width + p];


		}
	}
}




uint8_t* calc_distances(FT_Bitmap* bitmap, uint8_t* unpacked) {
	uint8_t* result;
	int32_t i, j;
	uint32_t x_dist, y_dist;
	uint32_t sqr_dist;
	uint32_t width = bitmap->width;
	uint32_t height = bitmap->rows;

	result = malloc(bitmap->rows * bitmap->width);
	for(uint32_t y = 0; y < bitmap->rows; y++)
	for(uint32_t x = 0; x < bitmap->width; x++) {
		if (unpacked[y * width + x]) {
			result[y * width + x] = 255;
			continue;
		}
		sqr_dist = 255*255 * 2;
		y_dist = 255;
		x_dist = 255;
		for(j = -255; j < 255; j++)
		for(i = -255; i < 255; i++) {
			int32_t x0 = x+i;
			int32_t y0 = y+j;
			if( x0 >= 0 && y0 >= 0 && x0 < width && y0 < height)
				if (unpacked[y0 * width + x0]) {
					uint32_t i0 = abs(i);
					uint32_t j0 = abs(j);
					uint32_t dist = i0*i0 + j0*j0;
					if (dist < sqr_dist) {
						x_dist = i0;
						y_dist = j0;
						sqr_dist = dist;
					}
				}

		}
		result[y * width + x] = x_dist + y_dist;

	}
	return result;
}