#include <ncurses.h>
#include <panel.h>
#define MINIADUIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

// CONSTANTS
#define MAX_LEN 1024

#define MIN_SHUFFLE_DISTANCE 1
#define MAX_SHUFFLE_DISTANCE 20
#define SONG_PADDING 4

// STRUCTS

// General struct for songs
//
struct music_player {
    ma_engine engine;
    WINDOW *left_win;
    WINDOW *right_win;
    WINDOW *bot_left_win;
    WINDOW *bot_right_win;
    bool playing;
    bool shuffle;
    struct album *album_head;
    struct album *curr_album;
    struct album *selected_album;
    struct song *queue;
    struct song *selected_song;
    int top_win_h;
};

struct album {
    char name[MAX_LEN];
    struct song *songs;
    struct album *next;
};

struct song {
    char name[MAX_LEN];
    char path[MAX_LEN];
    bool initialized;
    ma_sound sound;
    struct song *next;
};


// FUNCTION PROTOTYPES
//
// Main function that gets ran that handles initialising windows,
// and then runs the main loop
void init_and_run();
// Main loop
void main_loop(char ch, struct music_player *mp);
// Init song object in linked list
struct song *init_song(char name[MAX_LEN], struct music_player *mp);
// Creates an album with the songs in it
struct album *init_album(char path[MAX_LEN], struct music_player *mp);
// Initialize the music player
struct music_player *init_mp();
// Play/pause music
void action_play(struct music_player *mp);
// Displays the windows (NOTE: Will need to refresh the windows or 
// I think something bad could happen?
void display(struct music_player *mp);
// Initialize albums
struct album *init_albums(struct music_player *mp);
// Appends song to list of songs
struct song *append_song(struct song *head, struct song *song_to_append);
// Append album to list of albums
struct album *append_album(struct album *head, struct album *append);
// Displays debug info in the bottom left window
void display_debug_info(struct music_player *mp, char info[MAX_LEN]);
// Updates information in playing window
void update_playing_window(struct music_player *mp);
// Refreshes specific window
void refresh_win(WINDOW *win);
// Updates information in the playlist window
void update_playlist_window(struct music_player *mp);
// Updates information in the album tracklist window
void update_song_window(struct music_player *mp);
// Loads music into the music player struct
// Made into a seperate function for modularity
void load_music(struct music_player *mp);
// User moves down song list
void action_move_down_song(struct music_player *mp);
// Same but for moving up list
void action_move_up_song(struct music_player *mp);
// Plays the selected song and redoes the queue
void action_play_song_selected(struct music_player *mp);
// User moves down the album list
void action_move_down_album(struct music_player *mp);
// Same but for moving up
void action_move_up_album(struct music_player *mp);
// Testing if callback actually works
void callback_test(void *info, ma_sound *sound);
// Cleans up and frees memory
void cleanup_mp(struct music_player *mp);
// Plays current song and also initializes it
void init_and_play_curr_song(struct music_player *mp);
// Frees the current song in memory
void uninit_curr_song(struct music_player *mp);
// Cleans up memory
void free_unused_songs(struct music_player *mp);
// Toggles shuffle
void toggle_shuffle(struct music_player *mp);
// Generates the number of songs to move forward when shuffling
int gen_shuffle_num();
