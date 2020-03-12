#define  APP_IMPLEMENTATION
#define  APP_WINDOWS
#include "app.h"

#define SYSFONT_IMPLEMENTATION
#pragma warning( push )
#pragma warning( disable: 4456 )
#pragma warning( disable: 4457 )
#include "sysfont.h"
#pragma warning( pop )

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define FRAMETIMER_IMPLEMENTATION
#include "frametimer.h"


#include <stdlib.h>
#include <string.h>


enum frame_t {
    FRAME_SINGLE,
    FRAME_HDOUBLE,
    FRAME_VDOUBLE, 
    FRAME_DOUBLE, 
    FRAME_SOLID, 
};

struct b800_t;

void outtext( struct b800_t* b800, char const* string );
void setfg( struct b800_t* b800, int fg );
void setbg( struct b800_t* b800, int bg );
void setpos( struct b800_t* b800, int x, int y );
void setcell( struct b800_t* b800, int x, int y, char ansi, int fg, int bg );
void clear( struct b800_t* b800, int x, int y, int w, int h );
void frame( struct b800_t* b800, enum frame_t type, int x, int y, int w, int h );
void hdivider( struct b800_t* b800, enum frame_t type, int x, int y, int w );
void vdivider( struct b800_t* b800, enum frame_t type, int x, int y, int h );
void divcross( struct b800_t* b800, enum frame_t type, int x, int y );
void cls( struct b800_t* b800 );
void curson( struct b800_t* b800 );
void cursoff( struct b800_t* b800 );
void curs( struct b800_t* b800, int x, int y );


#include "tracker.h"


typedef unsigned short u16;

struct b800_t {
    u16* buffer;
    int width;
    int height;
    int x;
    int y;
    int fg;
    int bg;
    int curs_x;
    int curs_y;
    int curs;
};


void outtext( struct b800_t* b800, char const* string ) {
    while( *string ) {
        if( b800->y >= b800->height ) break;
        
        u16 ch = (u16) (unsigned char) *string;
        ch |= ( b800->fg & 0xf ) << 8;
        ch |= ( b800->bg & 0xf ) << 12;

        b800->buffer[ b800->x + b800->y * b800->width ] = ch;

        ++b800->x;
        if( b800->x >= b800->width ) {
            if( b800->y < b800->height - 1 ) {
                ++b800->y;
            } else {
                --b800->x;
                break;
            }
            b800->x = 0;
        }

        ++string;
    }
}


void curson( struct b800_t* b800 ) {
    b800->curs = 1;
}


void cursoff( struct b800_t* b800 ) {
    b800->curs = 0;
}


void curs( struct b800_t* b800, int x, int y ) {
    b800->curs_x = x;
    b800->curs_y = y;
}


void setpos( struct b800_t* b800, int x, int y ) {
    b800->x = x;
    b800->y = y;
}


void setfg( struct b800_t* b800, int fg ) {
    b800->fg = fg < 0 ? 0 : fg > 15 ? 15 : fg;
}


void setbg( struct b800_t* b800, int bg ) {
    b800->bg = bg < 0 ? 0 : bg > 15 ? 15 : bg;
}


void setcell( struct b800_t* b800, int x, int y, char ansi, int fg, int bg ) {
    if( x < 0 || x >= b800->width || y < 0 || y >= b800->height ) {
        return;
    }
    u16 ch = (u16) (unsigned char) ansi;
    ch |= ( fg & 0xf ) << 8;
    ch |= ( bg & 0xf ) << 12;
        
    b800->buffer[ x + y * b800->width ] = ch;
}


void clear( struct b800_t* b800, int x, int y, int w, int h ) {
    if( x < 0 || x >= b800->width || y < 0 || y >= b800->height ) {
        return;
    }
    
    for( int iy = 0; iy < h; ++iy ) {
        for( int ix = 0; ix < w; ++ix ) {
            setcell( b800, x + ix, y + iy, ' ', b800->fg, b800->bg );
        }
    }
}


void frame( struct b800_t* b800, enum frame_t type, int x, int y, int w, int h ) {
    char tl[] = { '\xDA', '\xD5', '\xD6', '\xC9', '\xDB', };
    char t [] = { '\xC4', '\xCD', '\xC4', '\xCD', '\xDB', };
    char tr[] = { '\xBF', '\xB8', '\xB7', '\xBB', '\xDB', };
    char l [] = { '\xB3', '\xB3', '\xBA', '\xBA', '\xDB', };
    char r [] = { '\xB3', '\xB3', '\xBA', '\xBA', '\xDB', };
    char bl[] = { '\xC0', '\xD4', '\xD3', '\xC8', '\xDB', };
    char b [] = { '\xC4', '\xCD', '\xC4', '\xCD', '\xDB', };
    char br[] = { '\xD9', '\xBE', '\xBD', '\xBC', '\xDB', };
    
    if( type < 0 || type >= sizeof( tl ) / sizeof( *tl ) ) {
        return;
    }
    --w; --h;

    setcell( b800, x, y, tl[ type ], b800->fg, b800->bg );
    for( int i = 1; i < w; ++i ) setcell( b800, x + i, y, t[ type ], b800->fg, b800->bg );
    setcell( b800, x + w, y, tr[ type ], b800->fg, b800->bg );

    for( int i = 1; i < h; ++i ) {
        setcell( b800, x, y + i, l[ type ], b800->fg, b800->bg );
        setcell( b800, x + w, y + i, r[ type ], b800->fg, b800->bg );
    }

    setcell( b800, x, y + h, bl[ type ], b800->fg, b800->bg );
    for( int i = 1; i < w; ++i ) setcell( b800, x + i, y + h, b[ type ], b800->fg, b800->bg );
    setcell( b800, x + w, y + h, br[ type ], b800->fg, b800->bg );
}



void hdivider( struct b800_t* b800, enum frame_t type, int x, int y, int w ) {
    char s[] = { '\xC3', '\xC6', '\xC7', '\xCC', '\xDB', };
    char m[] = { '\xC4', '\xCD', '\xC4', '\xCD', '\xDB', };
    char e[] = { '\xB4', '\xB5', '\xB6', '\xB9', '\xDB', };

    if( type < 0 || type >= sizeof( s ) / sizeof( *s ) ) {
        return;
    }
    --w;

    setcell( b800, x, y, s[ type ], b800->fg, b800->bg );
    for( int i = 1; i < w; ++i ) setcell( b800, x + i, y, m[ type ], b800->fg, b800->bg );
    setcell( b800, x + w, y, e[ type ], b800->fg, b800->bg );
}


void vdivider( struct b800_t* b800, enum frame_t type, int x, int y, int h ) {
    char s[] = { '\xC2', '\xD1', '\xD2', '\xCB', '\xDB', };
    char m[] = { '\xB3', '\xB3', '\xBA', '\xBA', '\xDB', };
    char e[] = { '\xC1', '\xCF', '\xD0', '\xCA', '\xDB', };

    if( type < 0 || type >= sizeof( s ) / sizeof( *s ) ) {
        return;
    }
    --h;

    setcell( b800, x, y, s[ type ], b800->fg, b800->bg );
    for( int i = 1; i < h; ++i ) setcell( b800, x, y + i, m[ type ], b800->fg, b800->bg );
    setcell( b800, x, y + h, e[ type ], b800->fg, b800->bg );
}


void divcross( struct b800_t* b800, enum frame_t type, int x, int y ) {
    char c[] = { '\xC5', '\xD8', '\xD7', '\xCE', '\xDB', };

    if( type < 0 || type >= sizeof( c ) / sizeof( *c ) ) {
        return;
    }

    setcell( b800, x, y, c[ type ], b800->fg, b800->bg );
}


void cls( struct b800_t* b800 ) {
    u16* p = b800->buffer;
    u16 c = (u16) ' ';
    c |= ( b800->fg & 0xf ) << 8;
    c |= ( b800->bg & 0xf ) << 12;
    for( int y = 0; y < 25; ++y ) {
        for( int x = 0; x < 80; ++x ) {
            *p++ = c;
        }
    }
}


void sound_callback( APP_S16* sample_pairs, int sample_pairs_count, void* user_data ) {
    tsf* soundfont = (tsf*) user_data;
    tsf_render_short( soundfont, sample_pairs, sample_pairs_count, 0 );
}



int app_proc( app_t* app, void* user_data ) {
    (void) user_data;
    static APP_U32 screen[ 640 * 400 ] = { 0 };
    app_window_size( app, app_window_width( app ), (int) ( app_window_width( app ) * ( 400.0f / 640.0f ) ) );
    app_screenmode( app, APP_SCREENMODE_WINDOW );
    app_interpolation( app, APP_INTERPOLATION_NONE );
    app_title( app, "BlastTracker" );

	APP_U32 palette[ 16 ] = { 
		0x000000, 0xaa0000, 0x00aa00, 0xaaaa00, 0x0000aa, 0xaa00aa, 0x0055aa, 0xaaaaaa,
		0x555555, 0xff5555, 0x55ff55, 0xffff55, 0x5555ff, 0xff55ff, 0x55ffff, 0xffffff,
		};

    u16 ansi[ 80 * 25 ] = { 0 };
    struct b800_t b800 = { NULL, 80, 25, 0, 0, 7, 0 };
    b800.buffer = ansi;

    tsf* soundfont = tsf_load_filename( "OPL-3_FM_128M.sf2" );
//    tsf* soundfont = tsf_load_filename( "1MGM.sf2" );

    app_sound( app, 735 * 6, sound_callback, soundfont );

    frametimer_t* frametimer = frametimer_create( NULL );
    frametimer_lock_rate( frametimer, 60 );

    struct tracker_t* tracker = tracker_create( &b800, soundfont );
   
    int curs_x = tracker->curs_x;
    int curs_y = tracker->curs_y;
    int curs_vis = 0;
    while( app_yield( app ) != APP_STATE_EXIT_REQUESTED ) {
        frametimer_update( frametimer );
        app_input_t input = app_input( app );
        for( int i = 0; i < input.count; ++i ) {
            app_input_event_t* event = &input.events[ i ];
            if( event->type == APP_INPUT_MOUSE_MOVE ) {
                int x = event->data.mouse_pos.x;
                int y = event->data.mouse_pos.y;
                app_coordinates_window_to_bitmap( app, 640, 400, &x, &y );
                x /= 8;
                y /= 16;
                event->data.mouse_pos.x = x;
                event->data.mouse_pos.y = y;
            }
        }
        tracker_update( tracker, input, frametimer_delta_time( frametimer ) );

        tracker_draw( tracker );

        for( int y = 0; y < 25; ++y ) {
            for( int x = 0; x < 80; ++x ) {
                u16 cell = ansi[ x + y * 80 ];
                char str[] = { ( (unsigned char) cell ) & 0xff, '\0' };
                APP_U32 fg = palette[ ( cell >> 8 ) & 0xf ];
                APP_U32 bg = palette[ ( cell >> 12 ) & 0xf ];
                sysfont_8x16_u32( screen, 640, 400, x * 8, y * 16, "\xdb", bg );
                sysfont_8x16_u32( screen, 640, 400, x * 8, y * 16, str, fg );
            }
        }

        ++curs_vis;
        if( b800.curs && b800.curs_x >= 0 && b800.curs_x < 80 && b800.curs_y >= 0 && b800.curs_y < 25 ) {
            if( b800.curs_x != curs_x || b800.curs_y != curs_y ) {
                curs_x = b800.curs_x;
                curs_y = b800.curs_y;
                curs_vis = 0;
            }
            int vis = ( curs_vis % 50 ) < 25;
            if( vis ) {
                int xp = b800.curs_x * 8;
                int yp = b800.curs_y * 16;
                APP_U32 col = palette[ 7 ];
                for( int y = 12; y < 14; ++y ) {
                    for( int x = 0; x < 8; ++x ) {
                        screen[ ( x + xp ) + ( y + yp ) * 640 ] = col;
                    }
                }
            }
        }
        app_present( app, screen, 640, 400, 0xffffff, 0x000000 );
    }

    app_sound( app, 0, NULL, NULL );
    tracker_destroy( tracker );
    tsf_close( soundfont );
    frametimer_destroy( frametimer );

    return 0;
}


int main( int argc, char** argv ) {
    (void) argc, argv;
    SetProcessDPIAware();
    return app_run( app_proc, NULL, NULL, NULL, NULL );
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
#ifdef __cplusplus
extern "C" {
#endif
int __stdcall WinMain( struct HINSTANCE__* a, struct HINSTANCE__* b, char* c, int d ) { 
    (void) a, (void) b, (void) c, (void) d;
    return main( __argc, __argv ); 
}
#ifdef __cplusplus
{
#endif

