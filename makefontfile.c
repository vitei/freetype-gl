/* ============================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * ----------------------------------------------------------------------------
 * Copyright 2011,2012 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ============================================================================
 */
#include "opengl.h"
#include "vec234.h"
#include "vector.h"
#include "freetype-gl.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#if defined(__APPLE__)
    #include <Glut/glut.h>
#elif defined(_WIN32) || defined(_WIN64)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

typedef struct
{
	float size;
	float height;
	float lineGap;
	float ascender;
	float descender;

	uint32_t glyphCount;
} TXFontData;

typedef struct
{
	uint16_t charCode;
	float width;
	float height;
	float offsetX;
	float offsetY;
	float advanceX;
	float advanceY;
	float texture0X;
	float texture0Y;
	float texture1X;
	float texture1Y;

	uint32_t kerningCount;
} TXFontGlyphData;

typedef struct
{
	uint16_t charCode;
	float value;
} TXFontGlyphKerningData;

typedef struct
{
	uint32_t width;
	uint32_t height;
} TXFontTextureData;

// ---------------------------------------------------------------- display ---
void display( void )
{}

// ---------------------------------------------------------------- reshape ---
void reshape(int width, int height)
{}

// --------------------------------------------------------------- keyboard ---
void keyboard( unsigned char key, int x, int y )
{}

// ------------------------------------------------------------- print help ---
void print_help()
{
    fprintf( stderr, "Usage: makefont [--help] [--verbose] --font <font file> "
             "--output <output file> --size <font size> --maxedge <max texture edge size>\n" );
}

// ------------------------------------------------------------------- main ---
int main( int argc, char **argv )
{
    FILE* test;
    size_t i, j;
    int arg;

    wchar_t * font_cache = 
        L" !\"#$%&'()*+,-./0123456789:;<=>?"
        L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
        L"`abcdefghijklmnopqrstuvwxyz{|}~";

    float  font_size   = 0.0;
    const char * font_filename   = NULL;
    const char * output_filename = NULL;
    int texture_atlus_max_edge = 2048;
    int show_help = 0;
    int verbose_output = 0;

    for ( arg = 1; arg < argc; ++arg )
    {
        if ( 0 == strcmp( "--font", argv[arg] ) || 0 == strcmp( "-f", argv[arg] ) )
        {
            ++arg;

            if ( font_filename )
            {
                fprintf( stderr, "Multiple --font parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No font file given.\n" );
                print_help();
                exit( 1 );
            }

            font_filename = argv[arg];
            continue;
        }

        if ( 0 == strcmp( "--output", argv[arg] ) || 0 == strcmp( "-o", argv[arg] )  )
        {
            ++arg;

            if ( output_filename )
            {
                fprintf( stderr, "Multiple --output parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No output file given.\n" );
                print_help();
                exit( 1 );
            }

            output_filename = argv[arg];
            continue;
        }

        if ( 0 == strcmp( "--help", argv[arg] ) || 0 == strcmp( "-h", argv[arg] ) )
        {
            show_help = 1;
            break;
        }

        if ( 0 == strcmp( "--size", argv[arg] ) || 0 == strcmp( "-s", argv[arg] ) )
        {
            ++arg;

            if ( 0.0 != font_size )
            {
                fprintf( stderr, "Multiple --size parameters.\n" );
                print_help();
                exit( 1 );
            }

            if ( arg >= argc )
            {
                fprintf( stderr, "No font size given.\n" );
                print_help();
                exit( 1 );
            }

            errno = 0;

            font_size = atof( argv[arg] );

            if ( errno )
            {
                fprintf( stderr, "No valid font size given.\n" );
                print_help();
                exit( 1 );
            }

            continue;
        }

        if ( 0 == strcmp( "--maxedge", argv[arg] ) || 0 == strcmp( "-e", argv[arg] ) )
        {
            ++arg;

            errno = 0;

            texture_atlus_max_edge = atoi( argv[arg] );

            if ( errno )
            {
                fprintf( stderr, "No valid maximum edge size given.\n" );
                print_help();
                exit( 1 );
            }

            continue;
        }

        if ( 0 == strcmp( "--verbose", argv[arg] ) || 0 == strcmp( "-v", argv[arg] )  )
        {
            ++arg;

            verbose_output = 1;

            continue;
        }

        fprintf( stderr, "Unknown parameter %s\n", argv[arg] );
        print_help();
        exit( 1 );
    }

    if ( show_help )
    {
        print_help();
        exit( 1 );
    }

    if ( !font_filename )
    {
        fprintf( stderr, "No font file given.\n" );
        print_help();
        exit( 1 );
    }

    if ( !( test = fopen( font_filename, "r" ) ) )
    {
        fprintf( stderr, "Font file \"%s\" does not exist.\n", font_filename );
    }

    fclose( test );

    if ( 4.0 > font_size )
    {
        fprintf( stderr, "Font size too small, expected at least 4 pt.\n" );
        print_help();
        exit( 1 );
    }

    if ( !output_filename )
    {
        fprintf( stderr, "No header file given.\n" );
        print_help();
        exit( 1 );
    }

    // FIXME, choose better default values for these?
    int texture_atlus_width = 128;
    int texture_atlus_height = 128;

    texture_atlas_t * atlas = 0;
    texture_font_t  * font = 0;

    glutInit( &argc, argv );
    glutInitWindowSize( texture_atlus_max_edge, texture_atlus_max_edge );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutCreateWindow( "Freetype OpenGL" );
    glutReshapeFunc( reshape );
    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );

    do
    {
        if( atlas )
        {
            texture_atlas_delete( atlas );
        }

        if( font )
        {
            texture_font_delete( font );
        }

        if( texture_atlus_height > texture_atlus_max_edge )
        {
            fprintf( stderr, "Maximum texture edge size of %d exceeded.\n", texture_atlus_max_edge );
            exit( 1 );
        }

        atlas = texture_atlas_new( texture_atlus_width, texture_atlus_height, 1 );
        font  = texture_font_new_from_file( atlas, font_size, font_filename );

        if( texture_atlus_width == texture_atlus_height )
        {
            texture_atlus_height <<= 1;
        }
        else
        {
            texture_atlus_width <<= 1;
		}
    }
    while( texture_font_load_glyphs( font, font_cache ) );

    if(verbose_output)
    {
        wprintf( L"Font filename              : %s\n", font_filename );
        wprintf( L"Font size                  : %.1f\n", font_size );
        wprintf( L"Number of glyphs           : %ld\n", wcslen(font_cache) );
        // wprintf( L"Number of missed glyphs    : %ld\n", missed );
        wprintf( L"Texture size               : %ldx%ldx%ld\n",
                 atlas->width, atlas->height, atlas->depth );
        wprintf( L"Texture occupancy          : %.2f%%\n", 
                100.0*atlas->used/(float)(atlas->width*atlas->height) );
        wprintf( L"\n" );
        wprintf( L"Output filename            : %s\n", output_filename );
    }


    size_t texture_size = atlas->width * atlas->height *atlas->depth;
    size_t glyph_count = font->glyphs->size;
    size_t max_kerning_count = 1;
    for( i=0; i < glyph_count; ++i )
    {
        texture_glyph_t *glyph = *(texture_glyph_t **) vector_get( font->glyphs, i );

        if( vector_size(glyph->kerning) > max_kerning_count )
        {
            max_kerning_count = vector_size(glyph->kerning);
        }
    }


    FILE *file = fopen( output_filename, "wb" );

    // -------------------
    // Font information
    // -------------------

    TXFontData fontData = {
    	font->size,
    	font->height,
    	font->linegap,
    	font->ascender,
    	font->descender,

    	glyph_count
	};

	fwrite( &fontData, sizeof( fontData ), 1, file );

    // --------------
    // Font glyphs
    // --------------

    for( i=0; i < glyph_count; ++i )
    {
    	texture_glyph_t * glyph = *(texture_glyph_t **) vector_get( font->glyphs, i );
    	wchar_t character_code = glyph->charcode;

		if(character_code == (wchar_t)(-1))
		{
			character_code = 0;
		}

		TXFontGlyphData glyphData = {
			character_code,
			glyph->width,
			glyph->height,
			glyph->offset_x,
			glyph->offset_y,
			glyph->advance_x,
			glyph->advance_y,
			glyph->s0,
			glyph->t0,
			glyph->s1,
			glyph->t1,

			vector_size(glyph->kerning)
		};
		
		fwrite( &glyphData, sizeof( glyphData ), 1, file );

        for( j=0; j < vector_size(glyph->kerning); ++j )
        {
            kerning_t *kerning = (kerning_t *) vector_get( glyph->kerning, j);

			TXFontGlyphKerningData kerningData = {
				kerning->charcode,
				kerning->kerning
			};

			fwrite( &kerningData, sizeof( kerningData ), 1, file );
        }
    }

    // ------------
    // Texture data
    // ------------

	TXFontTextureData textureData = {
		atlas->width,
		atlas->height
	};

	fwrite( &textureData, sizeof( textureData ), 1, file );

	fwrite( atlas->data, sizeof( atlas->data[0] ), atlas->width * atlas->height * atlas->depth, file );

	fclose( file );

    return 0;
}
