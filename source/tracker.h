

struct internal_tracker_note_t {
    int note;
    int program;
    int velocity;
    int cc_command;
    int cc_data;
};

/*

CC OP
 0 No operation
 1 volume
 2 pan
 3 pitch bend up
 4 pitch bend down
 5 chord

*/


struct internal_tracker_channel_t {
    struct internal_tracker_note_t notes[ 100 ];
};


#define INTERNAL_TRACKER_MAX_CHANNEL_COUNT 4

struct internal_tracker_pattern_t {
    struct internal_tracker_channel_t channels[ INTERNAL_TRACKER_MAX_CHANNEL_COUNT ];
};

struct internal_tracker_midi_chan_info_t {
    int drums;
    int age;
    int channel;
    int program;
};

struct internal_tracker_spinner_t {
    int x;
    int y;
    int min;
    int max;
    int *value;
};


#define INTERNAL_TRACKER_ACTION_LOAD 1
#define INTERNAL_TRACKER_ACTION_SAVE 2

struct internal_tracker_button_t {
    int x;
    int y;
    int width;
    char label[ 32 ];
    int action;
};


struct tracker_t {
    struct b800_t* b800;
    tsf* soundfont;

    int is_playing;
    float play_time;

    int base_octave;
    int edit_mode;
    int current_program;
    int current_channel;
    int current_pattern;
    int pattern_pos;
    int channel_count;

    int curs_x;
    int curs_y;

    int mouse_x;
    int mouse_y;

    int pattern_length;
    struct internal_tracker_pattern_t patterns[ 100 ];

    struct internal_tracker_midi_chan_info_t midi_chan_info[ 16 ];


    int song_length;
    int song[ 1000 ];

    int keyboard_state[ APP_KEYCOUNT ];
    int prev_keyboard_state[ APP_KEYCOUNT ];

    struct internal_tracker_spinner_t spinners[ 32 ];
    int spinner_count;
    int spin_cooldown;

    struct internal_tracker_button_t buttons[ 32 ];
    int button_count;
};


int internal_tracker_key_pressed( struct tracker_t* tracker, app_key_t key ) {
    return tracker->keyboard_state[ key ] && !tracker->prev_keyboard_state[ key ];
}


int internal_tracker_key_released( struct tracker_t* tracker, app_key_t key ) {
    return !tracker->keyboard_state[ key ] && tracker->prev_keyboard_state[ key ];
}


int internal_tracker_key_down( struct tracker_t* tracker, app_key_t key ) {
    return tracker->keyboard_state[ key ];
}


int internal_tracker_add_spinner( struct tracker_t* tracker, int x, int y, int min, int max, int* value ) {
    if( tracker->spinner_count >= sizeof( tracker->spinners ) / sizeof( *tracker->spinners ) ) {
        return -1;
    }

    int index = tracker->spinner_count++;
    struct internal_tracker_spinner_t* spinner = &tracker->spinners[ index ];
    spinner->x = x;
    spinner->y = y;
    spinner->value = value;
    spinner->min = min;
    spinner->max = max;
    return index;
}


void internal_tracker_draw_spinners( struct tracker_t* tracker ) {
    setbg( tracker->b800, 2 );
    for( int i = 0; i < tracker->spinner_count; ++i ) {
        struct internal_tracker_spinner_t* spinner = &tracker->spinners[ i ];
        setfg( tracker->b800, tracker->mouse_x == spinner->x && tracker->mouse_y == spinner->y ? 14 : 0 );
        setpos( tracker->b800, spinner->x, spinner->y );
        outtext( tracker->b800, "\x18" );
        setfg( tracker->b800, tracker->mouse_x == spinner->x + 1 && tracker->mouse_y == spinner->y ? 14 : 0 );
        setpos( tracker->b800, spinner->x +1, spinner->y );
        outtext( tracker->b800, "\x19"  );

    }
}


void internal_tracker_update_spinners( struct tracker_t* tracker ) {
    if( tracker->spin_cooldown > 0 ) {
        --tracker->spin_cooldown;
    }
    if( ( internal_tracker_key_down( tracker, APP_KEY_LBUTTON ) && tracker->spin_cooldown <= 0 ) ||
        internal_tracker_key_pressed( tracker, APP_KEY_LBUTTON ) ) {

        for( int i = 0; i < tracker->spinner_count; ++i ) {
            struct internal_tracker_spinner_t* spinner = &tracker->spinners[ i ];
            if( tracker->mouse_x == spinner->x && tracker->mouse_y == spinner->y ) {
                ++(*spinner->value);
                if( *spinner->value >= spinner->max ) {
                    *spinner->value = spinner->min;
                }
                tracker->spin_cooldown = 8;
            }
            else if( tracker->mouse_x == spinner->x + 1 && tracker->mouse_y == spinner->y ) {
                --(*spinner->value);
                if( *spinner->value < spinner->min ) {
                    *spinner->value = spinner->max;
                }
                tracker->spin_cooldown = 8;
            }
        }
    }
}


int internal_tracker_add_button( struct tracker_t* tracker, int x, int y, int width, char const* label, int action ) {
    if( tracker->button_count >= sizeof( tracker->buttons ) / sizeof( *tracker->buttons ) ) {
        return -1;
    }

    int index = tracker->button_count++;
    struct internal_tracker_button_t* button = &tracker->buttons[ index ];
    button->x = x;
    button->y = y;
    button->width = width;
    strcpy( button->label, label );
    button->action = action;
    return index;
}


void internal_tracker_draw_buttons( struct tracker_t* tracker ) {    
    for( int i = 0; i < tracker->button_count; ++i ) {
        struct internal_tracker_button_t* button = &tracker->buttons[ i ];
        int hover = tracker->mouse_x >= button->x && tracker->mouse_x < button->x + button->width && tracker->mouse_y == button->y;
        int pressed = hover && internal_tracker_key_down( tracker, APP_KEY_LBUTTON );
        setfg( tracker->b800, hover ? 14 : 0 );
        setbg( tracker->b800, 2 );
        setpos( tracker->b800, button->x + pressed, button->y );
        clear( tracker->b800, button->x + pressed, button->y, button->width, 1 );
        setpos( tracker->b800, button->x + pressed + ( button->width - (int) strlen( button->label ) ) / 2, button->y );
        outtext( tracker->b800, button->label );

        if( !pressed ) {
            setfg( tracker->b800, 0 );
            setbg( tracker->b800, 7 );
            setpos( tracker->b800, button->x + button->width, button->y );
            outtext( tracker->b800, "\xdc" );
            setpos( tracker->b800, button->x + 1, button->y + 1 );
            for( int j = 0; j < button->width; ++j ) {
                outtext( tracker->b800, "\xdf" );
            }
        }
    }
}


int internal_tracker_update_buttons( struct tracker_t* tracker ) {
    if( !internal_tracker_key_released( tracker, APP_KEY_LBUTTON ) ) {
        return 0;
    }

    for( int i = 0; i < tracker->button_count; ++i ) {
        struct internal_tracker_button_t* button = &tracker->buttons[ i ];
        int hover = tracker->mouse_x >= button->x && tracker->mouse_x < button->x + button->width && tracker->mouse_y == button->y;
        if( hover ) {
            return button->action;
        }
    }

    return 0;
}


struct tracker_t* tracker_create( struct b800_t* b800, tsf* soundfont ) {
    struct tracker_t* tracker = (struct tracker_t*) malloc( sizeof( struct tracker_t ) );
    memset( tracker, 0, sizeof( *tracker ) );

    tracker->b800 = b800;
    tracker->soundfont = soundfont;
    tracker->pattern_length = 32;
    tracker->song_length = 1;
    tracker->base_octave = 2;
    tracker->current_program = 0;
    tracker->channel_count = INTERNAL_TRACKER_MAX_CHANNEL_COUNT;

    tracker->curs_x = 6;
    tracker->curs_y = 17;

    tracker->midi_chan_info[ 9 ].drums = 1;
    tsf_channel_set_presetnumber( tracker->soundfont, 9, 0, 1 );

    internal_tracker_add_spinner( tracker, 15, 7, 0, 128, &tracker->current_program );
    internal_tracker_add_spinner( tracker, 76, 7, 0, 8, &tracker->base_octave );

    internal_tracker_add_button( tracker, 15, 2, 10, "Load", INTERNAL_TRACKER_ACTION_LOAD );
    internal_tracker_add_button( tracker, 15, 4, 10, "Save", INTERNAL_TRACKER_ACTION_SAVE );

    return tracker;
}


void tracker_destroy( struct tracker_t* tracker ) {
    free( tracker );
}


int internal_tracker_set_program( struct tracker_t* tracker, int channel, int program ) {    
    // for drums, use drum channel only
    if( program == 128 ) {
        for( int i = 0; i < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++i ) {
            if( tracker->midi_chan_info[ i ].drums ) {
                return i;
            }
        }
        return 9; 
    }

    // find channel already allocated
    int oldest_index = 0;
    int oldest_age = tracker->midi_chan_info[ 0 ].age;
    for( int i = 0; i < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++i ) {
        if( tracker->midi_chan_info[ i ].channel == channel && tracker->midi_chan_info[ i ].program == program ) {
            tracker->midi_chan_info[ i ].age = 0;
            return i;
        } else if( tracker->midi_chan_info[ i ].age > oldest_age ) {
            oldest_index = i;
            oldest_age = tracker->midi_chan_info[ i ].age;
        }
    }

    // allocate oldest channel
    tracker->midi_chan_info[ oldest_index ].age = 0;
    tracker->midi_chan_info[ oldest_index ].channel = channel;
    tracker->midi_chan_info[ oldest_index ].program = program;
    tsf_channel_set_presetnumber( tracker->soundfont, oldest_index, program, 0 );
    return oldest_index;
}


void internal_tracker_play_pattern_pos( struct tracker_t* tracker) {

    struct chords_t {
        int note;
        int chord[ 7 ];
    } chords[ 2 ][ 12 ] = {
        /* Major */ { 
            {  0 /* C  */, {  0,  4,  7, -1, -1, -1, -1 } },
            {  1 /* C# */, {  1,  5,  8, -1, -1, -1, -1 } },
            {  2 /* D  */, {  2,  6,  9, -1, -1, -1, -1 } },
            {  3 /* D# */, {  3,  7, 10, -1, -1, -1, -1 } },
            {  4 /* E  */, {  4,  8, 11, -1, -1, -1, -1 } },
            {  5 /* F  */, {  5,  9, 12, -1, -1, -1, -1 } },
            {  6 /* F# */, {  6, 10, 13, -1, -1, -1, -1 } },
            {  7 /* G  */, {  7, 11, 14, -1, -1, -1, -1 } },
            {  8 /* G# */, {  8, 12, 15, -1, -1, -1, -1 } },
            {  9 /* A  */, {  9, 13, 16, -1, -1, -1, -1 } },
            { 10 /* A# */, { 10, 14, 17, -1, -1, -1, -1 } },
            { 11 /* B  */, { 11, 15, 18, -1, -1, -1, -1 } },
        },                      
        /* Minor */ { 
            {  0 /* C  */, {  0,  3,  7, -1, -1, -1, -1 } },
            {  1 /* C# */, {  1,  4,  8, -1, -1, -1, -1 } },
            {  2 /* D  */, {  2,  5,  9, -1, -1, -1, -1 } },
            {  3 /* D# */, {  3,  6, 10, -1, -1, -1, -1 } },
            {  4 /* E  */, {  4,  7, 11, -1, -1, -1, -1 } },
            {  5 /* F  */, {  5,  8, 12, -1, -1, -1, -1 } },
            {  6 /* F# */, {  6,  9, 13, -1, -1, -1, -1 } },
            {  7 /* G  */, {  7, 10, 14, -1, -1, -1, -1 } },
            {  8 /* G# */, {  8, 11, 15, -1, -1, -1, -1 } },
            {  9 /* A  */, {  9, 12, 16, -1, -1, -1, -1 } },
            { 10 /* A# */, { 10, 13, 17, -1, -1, -1, -1 } },
            { 11 /* B  */, { 11, 14, 18, -1, -1, -1, -1 } },
        },      
                
    };          

    for( int i = 0; i < tracker->channel_count; ++i ) {
        struct internal_tracker_note_t* note = &tracker->patterns[ tracker->current_pattern ].channels[ i ].notes[ tracker->pattern_pos ];
        if( note->note ) {
            for( int j = 0; j < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++j ) {
                if( tracker->midi_chan_info[ j ].channel == i ) {
                    tsf_channel_note_off_all( tracker->soundfont, j );
                }
            }
            if( note->note < 128 ) {
                int midi_channel = internal_tracker_set_program( tracker, i, note->program );
                if( note->cc_command == 5 && note->cc_data >= 0 && note->cc_data < sizeof( chords ) / sizeof( *chords ) ) {
                    struct chords_t* chord = &chords[ note->cc_data ][ note->note % 12 ];
                    for( int k = 0; k < sizeof( chord->chord ) / sizeof( *chord->chord ); ++k ) {
                        if( chord->chord[ k ] >= 0 ) {
                            int n = note->note - ( note->note % 12 ) + chord->chord[ k ];
                            tsf_channel_note_on( tracker->soundfont, midi_channel, n, note->velocity / 127.0f );
                        }
                    }
                } else {
                    tsf_channel_note_on( tracker->soundfont, midi_channel, note->note, note->velocity / 127.0f );
                }
            }
        }
    }
}


    //int base_octave;
    //int edit_mode;
    //int current_program;
    //int current_channel;
    //int current_pattern;
    //int pattern_pos;
    //int channel_count;

    //int curs_x;
    //int curs_y;

    //int mouse_x;
    //int mouse_y;

    //int pattern_length;
    //struct internal_tracker_pattern_t patterns[ 100 ];

    //struct internal_tracker_midi_chan_info_t midi_chan_info[ 16 ];


    //int song_length;
    //int song[ 1000 ];

    //int keyboard_state[ APP_KEYCOUNT ];
    //int prev_keyboard_state[ APP_KEYCOUNT ];

    //struct internal_tracker_spinner_t spinners[ 32 ];
    //int spinner_count;
    //int spin_cooldown;

    //struct internal_tracker_button_t buttons[ 32 ];
    //int button_count;



void internal_tracker_save_song( struct tracker_t* tracker, char const* filename ) {
    FILE* fp = fopen( filename, "wb" );
    if( !fp ) {
        // TODO: error message
        return;
    }

    char const* header = "BLASTTRACKERSONG";
    int version = 0;
    fwrite( header, strlen( header ), 1, fp );
    fwrite( &version, sizeof( version ), 1, fp );
    fwrite( &tracker->pattern_length, sizeof( tracker->pattern_length ), 1, fp );
    int pattern_count = 0;
    int pattern_pos = ftell( fp );
    fwrite( &pattern_count, sizeof( pattern_count ), 1, fp );
    for( int i = 0; i < sizeof( tracker->patterns ) / sizeof( *tracker->patterns ); ++i ) {
        struct internal_tracker_pattern_t* pattern = &tracker->patterns[ i ];
        int first_write = 1;
        for( int j = 0; j < tracker->channel_count; ++j ) {
            struct internal_tracker_channel_t* channel = &pattern->channels[ j ];
            for( int k = 0; k < tracker->pattern_length; ++k ) {                
                struct internal_tracker_note_t* note = &channel->notes[ k ];
                if( first_write ) { 
                    if( note->note || note->program || note->velocity || note->cc_command || note->cc_data ) {
                        ++pattern_count;
                        fwrite( &i, sizeof( i ), 1, fp );
                        first_write = 0;
                    }
                }
                if( !first_write ) {
                    fwrite( &note->note, sizeof( note->note ), 1, fp );
                    fwrite( &note->program, sizeof( note->program ), 1, fp );
                    fwrite( &note->velocity, sizeof( note->velocity ), 1, fp );
                    fwrite( &note->cc_command, sizeof( note->cc_command ), 1, fp );
                    fwrite( &note->cc_data, sizeof( note->cc_data ), 1, fp );
                }
            }
        }
    }
    int current_pos = ftell( fp );
    fseek( fp, pattern_pos, SEEK_SET );
    fwrite( &pattern_count, sizeof( pattern_count ), 1, fp );
    fseek( fp, current_pos, SEEK_SET );
    
    fwrite( &tracker->song_length, sizeof( tracker->song_length ), 1, fp );
    for( int i = 0; i < tracker->song_length; ++i ) {
        fwrite( &tracker->song[ i ], sizeof( tracker->song[ i ] ), 1, fp );
    }

    fclose( fp );
}


void internal_tracker_load_song( struct tracker_t* tracker, char const* filename ) {
    (void) tracker;
    FILE* fp = fopen( filename, "rb" );
    if( !fp ) {
        // TODO: error message
        return;
    }

    char header[ sizeof( "BLASTTRACKERSONG" ) ] = "";
    int version = 0;
    fread( header, sizeof( header ) - 1, 1, fp );
    fread( &version, sizeof( version ), 1, fp );
    fread( &tracker->pattern_length, sizeof( tracker->pattern_length ), 1, fp );
    int pattern_count = 0;
    memset( tracker->patterns, 0, sizeof( tracker->patterns ) );
    fread( &pattern_count, sizeof( pattern_count ), 1, fp );
    for( int i = 0; i < pattern_count; ++i ) {
        int index = 0;
        fread( &index, sizeof( index ), 1, fp );       
        struct internal_tracker_pattern_t* pattern = &tracker->patterns[ index ];
        for( int j = 0; j < tracker->channel_count; ++j ) {
            struct internal_tracker_channel_t* channel = &pattern->channels[ j ];
            for( int k = 0; k < tracker->pattern_length; ++k ) {                
                struct internal_tracker_note_t* note = &channel->notes[ k ];
                fread( &note->note, sizeof( note->note ), 1, fp );
                fread( &note->program, sizeof( note->program ), 1, fp );
                fread( &note->velocity, sizeof( note->velocity ), 1, fp );
                fread( &note->cc_command, sizeof( note->cc_command ), 1, fp );
                fread( &note->cc_data, sizeof( note->cc_data ), 1, fp );
            }
        }
    }
    
    fread( &tracker->song_length, sizeof( tracker->song_length ), 1, fp );
    for( int i = 0; i < tracker->song_length; ++i ) {
        fread( &tracker->song[ i ], sizeof( tracker->song[ i ] ), 1, fp );
    }

    fclose( fp );
}


void internal_tracker_load( struct tracker_t* tracker ) {
    #ifdef _WIN32 
        (void) tracker;
        OPENFILENAMEA ofn = { sizeof( OPENFILENAMEA ) };
        CHAR szFile[ 260 ] = { 0 };

        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "BlastTracker Song (*.bts)\0*.BTS\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if( GetOpenFileNameA( &ofn ) ) {
            internal_tracker_load_song( tracker, szFile );
        }
    #else
        #error "Unsupported platform"
    #endif
}


void internal_tracker_save( struct tracker_t* tracker ) {
    #ifdef _WIN32 
        OPENFILENAMEA ofn = { sizeof( OPENFILENAMEA ) };
        CHAR szFile[ 260 ] = { 0 };

        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "BlastTracker Song (*.bts)\0*.BTS\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.lpstrDefExt = "bts";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if( GetSaveFileNameA( &ofn ) ) {
            internal_tracker_save_song( tracker, szFile );
        }
    #else
        #error "Unsupported platform"
    #endif
}


void tracker_update( struct tracker_t* tracker, app_input_t input, float delta_time ) {

    struct keyboard_note_map_t {
        app_key_t key;
        int note;
    } keyboard_note_map[] = {
        { APP_KEY_Z,           0 }, // C
        { APP_KEY_S,           1 }, // C#
        { APP_KEY_X,           2 }, // D
        { APP_KEY_D,           3 }, // D#
        { APP_KEY_C,           4 }, // E
        { APP_KEY_V,           5 }, // F
        { APP_KEY_G,           6 }, // F#
        { APP_KEY_B,           7 }, // G
        { APP_KEY_H,           8 }, // G#
        { APP_KEY_N,           9 }, // A
        { APP_KEY_J,          10 }, // A#
        { APP_KEY_M,          11 }, // B
        { APP_KEY_OEM_COMMA,  12 }, // C
        { APP_KEY_L,          13 }, // C#
        { APP_KEY_OEM_PERIOD, 14 }, // D
        { APP_KEY_OEM_1,      15 }, // D#
        { APP_KEY_OEM_2,      16 }, // E

        { APP_KEY_Q,          12 }, // C
        { APP_KEY_2,          13 }, // C#
        { APP_KEY_W,          14 }, // D
        { APP_KEY_3,          15 }, // D#
        { APP_KEY_E,          16 }, // E
        { APP_KEY_R,          17 }, // F
        { APP_KEY_5,          18 }, // F#
        { APP_KEY_T,          19 }, // G
        { APP_KEY_6,          20 }, // G#
        { APP_KEY_Y,          21 }, // A
        { APP_KEY_7,          22 }, // A#
        { APP_KEY_U,          23 }, // B
        { APP_KEY_I,          24 }, // C
        { APP_KEY_9,          25 }, // C#
        { APP_KEY_O,          26 }, // D
        { APP_KEY_0,          27 }, // D#
        { APP_KEY_P,          28 }, // E
        { APP_KEY_OEM_4,      29 }, // F
        { APP_KEY_OEM_PLUS,   30 }, // F#
    };

    memcpy( tracker->prev_keyboard_state, tracker->keyboard_state, sizeof( tracker->prev_keyboard_state ) );
    for( int i = 0; i < input.count; ++i ) {
        app_input_event_t const* event = &input.events[ i ];
        if( event->type == APP_INPUT_KEY_DOWN ) {
            tracker->keyboard_state[ event->data.key ] = 1;
            int in_key_map = 0;
            for( int j = 0; j < sizeof( keyboard_note_map ) / sizeof( *keyboard_note_map ); ++j ) {
                if( keyboard_note_map[ j ].key == event->data.key ) {
                    in_key_map = 1;
                    break;
                }
            }
            if( !in_key_map ) {
                tracker->prev_keyboard_state[ event->data.key ] = 0;
            }
        } else if( event->type == APP_INPUT_KEY_UP ) {
            tracker->keyboard_state[ event->data.key ] = 0;
        } else if( event->type == APP_INPUT_MOUSE_MOVE ) {
            tracker->mouse_x = event->data.mouse_pos.x;
            tracker->mouse_y = event->data.mouse_pos.y;
        }
    }
         
    for( int i = 0; i < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++i ) {
        ++tracker->midi_chan_info[ i ].age;
    }

    internal_tracker_update_spinners( tracker );

    int action = internal_tracker_update_buttons( tracker );
    if( action == INTERNAL_TRACKER_ACTION_LOAD ) {
        internal_tracker_load( tracker );
    } else if( action == INTERNAL_TRACKER_ACTION_SAVE ) {
        internal_tracker_save( tracker );
    }

    if( tracker->is_playing ) {
        float const step_time = 0.15f;
        tracker->play_time += delta_time;
        int new_pos = ( (int)( tracker->play_time / step_time ) ) % tracker->pattern_length;
        if( tracker->pattern_pos != new_pos ) {
            tracker->pattern_pos = new_pos;
            internal_tracker_play_pattern_pos( tracker );
        }

        if( internal_tracker_key_pressed( tracker, APP_KEY_SPACE ) ) {
            tracker->is_playing = 0;
            for( int i = 0; i < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++i ) {
                tsf_channel_note_off_all( tracker->soundfont, i );
            }
            return;
        }
    }


    int min_x = 6 + 18 * tracker->current_channel;
    int max_x = min_x + 16;
    int offs = tracker->curs_x - min_x;


    for( int i = 0; i < sizeof( keyboard_note_map ) / sizeof( *keyboard_note_map ); ++i ) {
        int note_num = tracker->base_octave * 12 + keyboard_note_map[ i ].note;

        if( internal_tracker_key_pressed( tracker, keyboard_note_map[ i ].key ) && 
            ( tracker->current_program < 128 || ( note_num >= 35 && note_num <= 81 ) ) ) {
            int midi_channel = internal_tracker_set_program( tracker, tracker->current_channel, tracker->current_program );
            tsf_channel_note_on( tracker->soundfont, midi_channel, note_num, 1.0f );
            if( tracker->edit_mode && !tracker->is_playing && offs < 3 ) {
                struct internal_tracker_note_t* note = &tracker->patterns[ tracker->current_pattern ].channels[ tracker->current_channel ].notes[ tracker->pattern_pos ];
                
                note->note = note_num;
                note->program = tracker->current_program;
                note->velocity = 127;
                tracker->pattern_pos = ( tracker->pattern_pos + 1 ) % ( tracker->pattern_length );
            }
        } else if( internal_tracker_key_released( tracker, keyboard_note_map[ i ].key ) ) {
            for( int j = 0; j < sizeof( tracker->midi_chan_info ) / sizeof( *tracker->midi_chan_info ); ++j ) {
                if( tracker->midi_chan_info[ j ].channel == tracker->current_channel ) {
                    tsf_channel_note_off( tracker->soundfont, j, note_num );
                }
            }
        } 

    }

    if( tracker->is_playing ) return;

    if( internal_tracker_key_pressed( tracker, APP_KEY_SPACE ) ) {
        tracker->is_playing = 1;
        tracker->pattern_pos = 0;
        tracker->play_time = 0.0f;
        internal_tracker_play_pattern_pos( tracker );
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_RETURN ) ) {
        tracker->edit_mode = !tracker->edit_mode;
    }

    if( !internal_tracker_key_down( tracker, APP_KEY_SHIFT ) && internal_tracker_key_pressed( tracker, APP_KEY_TAB ) && tracker->edit_mode ) {
        tracker->current_channel = ( tracker->current_channel + 1 ) % tracker->channel_count;
        tracker->curs_x = 6 + 18 * tracker->current_channel;
    }

    if( internal_tracker_key_down( tracker, APP_KEY_SHIFT ) && internal_tracker_key_pressed( tracker, APP_KEY_TAB ) && tracker->edit_mode ) {
        tracker->current_channel = ( tracker->current_channel - 1 + tracker->channel_count ) % tracker->channel_count;
        tracker->curs_x = 6 + 18 * tracker->current_channel;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_UP ) ) {
        tracker->pattern_pos = ( tracker->pattern_pos - 1 + tracker->pattern_length ) % tracker->pattern_length;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_DOWN ) ) {
        tracker->pattern_pos = ( tracker->pattern_pos + 1 ) % tracker->pattern_length;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_PRIOR ) ) {
        tracker->current_program = ( tracker->current_program - 1 + 129 ) % 129;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_NEXT ) ) {
        tracker->current_program = ( tracker->current_program + 1 ) % 129;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_SUBTRACT ) ) {
        tracker->base_octave = ( tracker->base_octave - 1 + 9 ) % 9;
    }

    if( internal_tracker_key_pressed( tracker, APP_KEY_ADD ) ) {
        tracker->base_octave = ( tracker->base_octave + 1 ) % 9;
    }

    if( tracker->edit_mode ) {
        if( internal_tracker_key_pressed( tracker, APP_KEY_LEFT ) ) {
            --tracker->curs_x;           
            if( offs == 4 || offs == 8 || offs == 12 || offs == 14 ) {
                --tracker->curs_x;
            }
            if( tracker->curs_x < min_x ) {
                tracker->current_channel = ( tracker->current_channel - 1 + tracker->channel_count ) % tracker->channel_count;
                tracker->curs_x = 6 + 18 * tracker->current_channel + 16;
            }
        }

        if( internal_tracker_key_pressed( tracker, APP_KEY_RIGHT ) ) {
            ++tracker->curs_x;
            if( offs == 2 || offs == 6 || offs == 10 || offs == 12 ) {
                ++tracker->curs_x;
            }
            if( tracker->curs_x > max_x ) {
                tracker->current_channel = ( tracker->current_channel  + 1 ) % tracker->channel_count;
                tracker->curs_x = 6 + 18 * tracker->current_channel;
            }
        }

        if( internal_tracker_key_pressed( tracker, APP_KEY_DELETE ) ) {
            struct internal_tracker_note_t* note = &tracker->patterns[ tracker->current_pattern ].channels[ tracker->current_channel ].notes[ tracker->pattern_pos ];
            note->note = 0;
            note->program = 0;
            note->velocity = 0;
            note->cc_command = 0;
            note->cc_data = 0;
        }

        if( internal_tracker_key_pressed( tracker, APP_KEY_INSERT ) ) {
            struct internal_tracker_note_t* note = &tracker->patterns[ tracker->current_pattern ].channels[ tracker->current_channel ].notes[ tracker->pattern_pos ];
            note->note = 128;
        }

        int num = -1;
        if( internal_tracker_key_pressed( tracker, APP_KEY_0 ) ) num = 0;
        if( internal_tracker_key_pressed( tracker, APP_KEY_1 ) ) num = 1;
        if( internal_tracker_key_pressed( tracker, APP_KEY_2 ) ) num = 2;
        if( internal_tracker_key_pressed( tracker, APP_KEY_3 ) ) num = 3;
        if( internal_tracker_key_pressed( tracker, APP_KEY_4 ) ) num = 4;
        if( internal_tracker_key_pressed( tracker, APP_KEY_5 ) ) num = 5;
        if( internal_tracker_key_pressed( tracker, APP_KEY_6 ) ) num = 6;
        if( internal_tracker_key_pressed( tracker, APP_KEY_7 ) ) num = 7;
        if( internal_tracker_key_pressed( tracker, APP_KEY_8 ) ) num = 8;
        if( internal_tracker_key_pressed( tracker, APP_KEY_9 ) ) num = 9;

        if( num >=0 && offs >= 4 ) {
            struct internal_tracker_note_t* note = &tracker->patterns[ tracker->current_pattern ].channels[ tracker->current_channel ].notes[ tracker->pattern_pos ];
            if( offs >= 4 && offs <= 6 && note->note > 0 ) {
                int val = note->program + 1;
                int a = offs == 4 ? num : ( val / 100 ) % 10;
                int b = offs == 5 ? num : ( val /  10 ) % 10;
                int c = offs == 6 ? num : ( val /   1 ) % 10;
                val = 100 * a + 10 * b + c;
                if( val >= 1 && val <= 129 ) {
                    note->program = val - 1;
                }
            }
            else if( offs >= 8 && offs <= 10 && note->note > 0  ) {
                int val = note->velocity;
                int a = offs == 8 ? num : ( val / 100 ) % 10;
                int b = offs == 9 ? num : ( val /  10 ) % 10;
                int c = offs == 10 ? num : ( val /   1 ) % 10;
                val = 100 * a + 10 * b + c;
                if( val >= 0 && val < 128 ) {
                    note->velocity = val;
                }
            }
            else if( offs == 12 ) {
                int val = note->cc_command;
                val = num;
                if( val >= 0 && val <= 9 ) {
                    note->cc_command = val;
                }
            }
            else if( offs >= 14 && offs <= 16 ) {
                int val = note->cc_data;
                int a = offs == 14 ? num : ( val / 100 ) % 10;
                int b = offs == 15 ? num : ( val /  10 ) % 10;
                int c = offs == 16 ? num : ( val /   1 ) % 10;
                val = 100 * a + 10 * b + c;
                if( val >= 0 && val <= 999 ) {
                    note->cc_data = val;
                }
            }
            ++tracker->curs_x;
            if( offs == 2 || offs == 6 || offs == 10 || offs == 12 ) {
                ++tracker->curs_x;
            }
            if( tracker->curs_x > max_x ) {
                tracker->current_channel = ( tracker->current_channel  + 1 ) % tracker->channel_count;
                tracker->curs_x = 6 + 18 * tracker->current_channel;
            }
        }

        curson( tracker->b800 );
        curs( tracker->b800, tracker->curs_x, tracker->curs_y );
    } else {
        cursoff( tracker->b800 );
    }
    
}


void tracker_draw( struct tracker_t* tracker ) {
    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 7 );
    cls( tracker->b800 );


    char const* programs[129] = {
        "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honky-tonk Piano", 
        "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavinet", "Celesta", "Glockenspiel", "Music Box",
        "Vibraphone", "Marimba", "Xylophone", "Tubular Bells", "Dulcimer", "Drawbar Organ", "Percussive Organ",
        "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion", 
        "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)", "Electric Guitar (clean)",
        "Electric Guitar (muted)", "Overdriven Guitar", "Distortion Guitar", "Guitar harmonics", "Acoustic Bass",
        "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", 
        "Synth Bass 1", "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", 
        "Pizzicato Strings", "Orchestral Harp", "Timpani", "String Ensemble 1", "String Ensemble 2", 
        "Synth Strings 1", "Synth Strings 2", "Choir Aahs", "Voice Oohs", "Synth Voice", "Orchestra Hit", "Trumpet",
        "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
        "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet",
        "Piccolo", "Flute", "Recorder", "Pan Flute", "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina", 
        "Synth Lead 1 (square)", "Synth Lead 2 (sawtooth)", "Synth Lead 3 (calliope)", "Synth Lead 4 (chiff)",
        "Synth Lead 5 (charang)", "Synth Lead 6 (voice)", "Synth Lead 7 (fifths)", "Synth Lead 8 (bass + lead)",
        "Synth Pad 1 (new age)", "Synth Pad 2 (warm)", "Synth Pad 3 (polysynth)", "Synth Pad 4 (choir)",
        "Synth Pad 5 (bowed)", "Synth Pad 6 (metallic)", "Synth Pad 7 (halo)", "Synth Pad 8 (sweep)",
        "Synth FX 1 (rain)", "Synth FX 2 (soundtrack)", "Synth FX 3 (crystal)", "Synth FX 4 (atmosphere)",
        "Synth FX 5 (brightness)", "Synth FX 6 (goblins)", "Synth FX 7 (echoes)", "Synth FX 8 (sci-fi)", "Sitar",
        "Banjo", "Shamisen", "Koto", "Kalimba", "Bag pipe", "Fiddle", "Shanai", "Tinkle Bell", "Agogo", 
        "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal", 
        "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause",
        "Gunshot", "DRUMS",
    };

    /*
    char const* drums[128] = {
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
        "", "", "", "", "", "", "", "", "Acoustic Bass Drum", "Bass Drum 1", "Side Stick/Rimshot", "Acoustic Snare", 
        "Hand Clap", "Electric Snare", "Low Floor Tom", "Closed Hi-hat", "High Floor Tom", "Pedal Hi-hat", "Low Tom", 
        "Open Hi-hat", "Low-Mid Tom", "Hi-Mid Tom", "Crash Cymbal 1", "High Tom", "Ride Cymbal 1", "Chinese Cymbal", 
        "Ride Bell", "Tambourine", "Splash Cymbal", "Cowbell", "Crash Cymbal 2", "Vibra Slap", "Ride Cymbal 2", 
        "High Bongo", "Low Bongo", "Mute High Conga", "Open High Conga", "Low Conga", "High Timbale", "Low Timbale", 
        "High Agogo", "Low Agogo", "Cabasa", "Maracas", "Short Whistle", "Long Whistle", "Short Guiro", "Long Guiro", 
        "Claves", "High Wood Block", "Low Wood Block", "Mute Cuica", "Open Cuica", "Mute Triangle", "Open Triangle", 
    };
    (void) drums;
    */

    char const* drums_short[128] = {
        "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
        "", "", "", "", "", "", "", "", "AcBassD", "BassDr1", "SideStk", "AcSnare", "HndClap", "ElSnare", "LoFlTom", 
        "ClHiHat", "HiFlTom", "PdHihat", "Low Tom", "OpHiHat", "LoMdTom", "HiMdTom", "CrCymb1", "HighTom", 
        "RdCymb1", "ChiCymb", "RideBel", "Tambour", "SplCymb", "Cowbell", "CrCymb2", "VibrSlp", "RdCymb2", 
        "HiBongo", "LoBongo", "MuHiCon", "OpHiCon", "LoConga", "HiTimba", "LoTimba", "HiAgogo", "LoAgogo", 
        "Cabasa ", "Maracas", "ShWhist", "LnWhist", "ShGuiro", "LnGuiro", "Claves ", "HiWBlck", "LoWBlck", 
        "MuCuica", "OpCuica", "MuteTri", "OpenTri", 
    };

    char const* notes[129] = { "---", "---", "---", "---", "---", "---", "---", "---", "---", "---", "---", "---",
        "C-0", "C#0", "D-0", "D#0",	"E-0","F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0", "C-1", "C#1", "D-1", 
        "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1", "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", 
        "F#2", "G-2", "G#2", "A-2", "A#2", "B-2", "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", 
        "A-3", "A#3", "B-3", "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4", 
        "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5", "C-6", "C#6", "D-6", 
        "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6", "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", 
        "F#7", "G-7", "G#7", "A-7", "A#7", "B-7", "C-8", "C#8", "D-8", "D#8", "E-8", "F-8", "F#8", "G-8", "G#8", 
        "A-8", "A#8", "B-8", "C-9", "C#9", "D-9", "D#9", "E-9", "F-9", "F#9", "G-9", "off"
    };

    char str[ 32 ];

    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 7 );
    setpos( tracker->b800, 2, 7 );
    outtext( tracker->b800, "Program" );

    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 1 );
    setpos( tracker->b800, 10, 7 );
    sprintf( str, " %3d ", tracker->current_program + 1 );
    outtext( tracker->b800, tracker->current_program < 128 ? str : " DRM " );

    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 7 );
    setpos( tracker->b800, 66, 7 );
    outtext( tracker->b800, "Octave" );
    sprintf( str, "%2d ", tracker->base_octave );
    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 1 );
    setpos( tracker->b800, 73, 7 );
    outtext( tracker->b800, str );


    setfg( tracker->b800, 0 );
    setbg( tracker->b800, 3 );
    
    clear( tracker->b800, 17, 7, 28, 1 );
    setpos( tracker->b800, 18, 7 );
    outtext( tracker->b800, programs[ tracker->current_program ] );


    internal_tracker_draw_buttons( tracker );
    internal_tracker_draw_spinners( tracker );

    setfg( tracker->b800, 15 );
    setbg( tracker->b800, 7 );

    frame( tracker->b800, FRAME_DOUBLE, 1, 8, 77, 17 );
    hdivider( tracker->b800, FRAME_VDOUBLE, 1, 10, 77 );
    vdivider( tracker->b800, FRAME_HDOUBLE, 5, 8, 17 );
    divcross( tracker->b800, FRAME_SINGLE, 5, 10 );

    vdivider( tracker->b800, FRAME_HDOUBLE, 5 + 18 * 1, 8, 17 );
    divcross( tracker->b800, FRAME_SINGLE, 5 + 18 * 1, 10 );

    vdivider( tracker->b800, FRAME_HDOUBLE, 5 + 18 * 2, 8, 17 );
    divcross( tracker->b800, FRAME_SINGLE, 5 + 18 * 2, 10 );

    vdivider( tracker->b800, FRAME_HDOUBLE, 5 + 18 * 3, 8, 17 );
    divcross( tracker->b800, FRAME_SINGLE, 5 + 18 * 3, 10 );

    

    setfg( tracker->b800, 0 );
    setpos( tracker->b800, 2, 9 );
    sprintf( str, "%03d", tracker->current_pattern );
    outtext( tracker->b800, str );

    setfg( tracker->b800, 8 );
    for( int j = 0; j < tracker->channel_count; ++j ) {
        setbg( tracker->b800, 7 );
        setfg( tracker->b800, 0 );
        setpos( tracker->b800, 6 + 18 * j, 9 );
        outtext( tracker->b800, "NOT PRG VEL CC" );
    
        setfg( tracker->b800, 10 );
        setpos( tracker->b800, 6 + 15 + 18 * j, 9 );
        outtext( tracker->b800, "\x0E" );

        setbg( tracker->b800, 2 );
        setfg( tracker->b800, 0 );
        setpos( tracker->b800, 6 + 16 + 18 * j, 9 );
        outtext( tracker->b800, "\x1F" );
    }

    
    struct internal_tracker_pattern_t* pattern = &tracker->patterns[ tracker->current_pattern ];

    for( int i = 0; i < 13; ++i ) {
        int pos = tracker->pattern_pos - 6 + i;
        if( pos < 0 || pos >= tracker->pattern_length ) {
            continue;
        }

        setbg( tracker->b800, i == 6 ? ( tracker->edit_mode ? 0 : 8 )  : 7 );

        setfg( tracker->b800, i == 6 ? ( tracker->edit_mode ? 14 : 7 ) : 8 );
        setpos( tracker->b800, 2, 11 + i );
        sprintf( str, "%03d", pos );
        outtext( tracker->b800, str );
        
        struct internal_tracker_note_t* note;

        for( int j = 0; j < tracker->channel_count; ++j ) {
            setfg( tracker->b800, i == 6 ? ( tracker->edit_mode ? ( tracker->current_channel == j ? 12 : 14 ) : 7 ) : 0 );
            note = &pattern->channels[ j ].notes[ pos ];
            if( note->note == 128 ) {
                sprintf( str, "%s         %01d %03d", notes[ note->note ], note->cc_command, note->cc_data );
            } else if( note->note == 0 || note->program < 128 ) {
                sprintf( str, "%s %03d %03d %01d %03d", notes[ note->note ], note->note ? note->program + 1 : 0, note->velocity, note->cc_command, note->cc_data );
            } else {
                sprintf( str, "%s %03d %01d %03d", drums_short[ note->note ], note->velocity, note->cc_command, note->cc_data );
            }

            setpos( tracker->b800, 6 + 18 * j, 11 + i );
            outtext( tracker->b800, str );
        }
    }
}
