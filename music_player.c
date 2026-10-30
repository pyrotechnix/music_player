#define NCURSES_WIDECHAR 1

// My glorious king
// https://stackoverflow.com/questions/79891188/bold-box-style-in-ncurses

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ncurses.h>
#include <locale.h>
#include <panel.h>
#include <dirent.h>
#include <dirent.h>
#include <string.h>

#include "music_player.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#define COLOR_GREY 8


// NOTE:
// Need to clean the mp3 file metadatas for them to work properly
// ffmpeg -i test.mp3 -map_metadata -1 -c:a libmp3lame clean_test.mp3
// this command seems to make them work

// Just make a clean directory inside the album folder and then run the bash script

// Potential things to add: 
// Visualizer but i might do that in a different window so
// possibly not
// Some way to see the image in an album

// TODO:
// - Read in music names
// - Add status bar at the bottom
// - Change display logic to be handled on a window by window basis (reduces flickering)
 
void init_and_run() {
    // NCURSES INIT STUFF
    setlocale(LC_ALL, "");
    initscr();
    noecho();
    start_color();
    curs_set(0);
    init_color(COLOR_BLACK, 114, 122, 129);
    init_color(COLOR_WHITE, 769, 784,772);
    init_color(COLOR_GREY, 513, 523, 481);
    
    //init_color(COLOR_GREY, 1000, 0, 1000);
    init_pair(1, COLOR_GREY, COLOR_BLACK);
    refresh();

    // INIT MUSIC PLAYER
    struct music_player *mp = init_mp();
    load_music(mp);

    char ch = ' ';
    while (ch != 'Q') {
        ch = getch();
        main_loop(ch, mp);
    }
    cleanup_mp(mp);
}

void main_loop(char ch, struct music_player *mp) {
    // Refreshing windows
    wclear(mp->left_win);
    wclear(mp->right_win);
    if (ch == ' ') {
        action_play(mp);
    } else if (ch == 'j') {
        action_move_down_song(mp);
    } else if (ch == 'k') {
        action_move_up_song(mp);
    } else if (ch == 'l') {
        action_play_song_selected(mp);
    } else if (ch == '\n') {
        action_play_song_selected(mp);
    } else if (ch == 'J') {
        action_move_down_album(mp);
    } else if (ch == 'K') {
        action_move_up_album(mp);
    } else if (ch == 'c') {
        free_unused_songs(mp);
    } else if (ch == 's') {
        toggle_shuffle(mp);
    }
    //display(mp);
}

struct song *init_song(char name[MAX_LEN], struct music_player *mp) {
    struct song *new_song = malloc(sizeof(struct song));
    new_song->next = NULL;
    strcpy(new_song->path, name);
    display_debug_info(mp, new_song->path);
    // char info_str[MAX_LEN] = "LOADING SONG ";
    // strcat(info_str, name);
    // MAKE SURE TO INIT THE SOUND DIRECTLY HERE INSTEAD OF MAKING THE SOUND SOMETHING ELSE AND INITIALIZING IT ELSEWHERE
    // FOR SOME REASON THE BRAINDEAD PROGRAMMERS OF MINIAUDIO DECIDED THAT DOING IT THE WAY LITERALLY EVERY OTHER PROGRAM 
    // WORKS AND MAKING A NEW VARIABLE AND THEN ASSIGNING new_song->sound = new_sound WAS TOO EASY SO THEY JUST HAD TO 
    // MAKE IT SHITTIER LIKE THIS
    /*
    ma_sound_config soundConfig;
    soundConfig = ma_sound_config_init();
    soundConfig.pFilePath = (const char *)name;
    soundConfig.endCallback = callback_test;
    soundConfig.pEndCallbackUserData = mp;
    ma_sound_init_ex(&mp->engine, &soundConfig, &new_song->sound); 
    */
    new_song->initialized = false;
    return new_song;
}

struct album *init_album(char name[MAX_LEN], struct music_player *mp) {
    // DIR *album_dir;
    // struct dirent *album_dir_ent;
    struct dirent **name_list;
    char album_dir_name[MAX_LEN] = "albums/";
    strcat(album_dir_name, name);
    int n = scandir(album_dir_name, &name_list, NULL, alphasort);
    // album_dir = opendir(album_dir_name);
    struct song *song_list = NULL;
    // Old way of doing it that didn't do it in alphabetical order
    /*
    while ((album_dir_ent = readdir(album_dir)) != NULL) {
        if (strcmp(album_dir_ent->d_name, ".") != 0 && strcmp(album_dir_ent->d_name, "..") != 0) {
            char song_str[MAX_LEN];
            strncpy(song_str, album_dir_name, MAX_LEN);
            strcat(song_str, "/");
            strcat(song_str, album_dir_ent->d_name);
            struct song *add_song = init_song(song_str, mp);
            strncpy(add_song->name, album_dir_ent->d_name, MAX_LEN);
            song_list = append_song(song_list, add_song);
        } 
    }
    */
    if (n == -1) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        if (strcmp(name_list[i]->d_name, ".") != 0 && strcmp(name_list[i]->d_name, "..") != 0) {
            char song_str[MAX_LEN];
            strncpy(song_str, album_dir_name, MAX_LEN);
            strcat(song_str, "/");
            strcat(song_str, name_list[i]->d_name);
            struct song *add_song = init_song(song_str, mp);
            strncpy(add_song->name, name_list[i]->d_name, MAX_LEN);
            song_list = append_song(song_list, add_song);
        } 
    }
    struct album *new_album = malloc(sizeof(struct album));
    new_album->songs = song_list;
    new_album->next = NULL;
    strncpy(new_album->name, name, MAX_LEN);
    closedir(album_dir);
    return new_album;
}

struct music_player *init_mp() {
    int width;
    int height;
    getmaxyx(stdscr, height, width);
    int left_width = width / 4;
    int top_height = (height / 4) * 3;
    struct music_player *mp = malloc(sizeof(struct music_player));
    ma_result result; 
    result = ma_engine_init(NULL, &mp->engine);
    if (result != MA_SUCCESS) {
        printf("SOMETHING HAS GONE SERIOUSLY WRONG. AUDIO ENGINE FAILED TO INIT\n");
        return mp;
    }
    mp->left_win = newwin(top_height, left_width, 0, 0);
    mp->right_win = newwin(top_height, width - left_width, 0, left_width);
    mp->bot_left_win = newwin(height - top_height, left_width, top_height, 0);
    mp->bot_right_win = newwin(height - top_height, width - left_width, top_height, left_width);
    mp->playing = false;
    mp->shuffle = false;
    display(mp);
    srand(time(0));
    return mp;
}

void action_play(struct music_player *mp) {
    if (mp->playing) { 
        ma_sound_stop(&mp->queue->sound);
        mp->playing = false;
    } else {
        init_and_play_curr_song(mp);
        //ma_sound_start(&mp->queue->sound);
        mp->playing = true;
    }
    update_playing_window(mp);
}

void display(struct music_player *mp) {
    refresh_win(mp->left_win);    
    refresh_win(mp->right_win);    
    refresh_win(mp->bot_left_win);    
    refresh_win(mp->bot_right_win);    
}

struct album *init_albums(struct music_player *mp) {
    struct album *albums = NULL;
    DIR *dp;
    struct dirent *ep;
    dp = opendir("albums");
    if (dp == NULL) {
        printf("SOMETHING HAS GONE WRONG");
    } else {
        printf("FETCHING ALBUMS...\n");
        while ((ep = readdir(dp)) != NULL) {
            
            printf("HELLO\n");
            char new_str[MAX_LEN]  = "albums/";
            strcat(new_str, ep->d_name);
            printf("%s\n", new_str);
            
            if (strcmp(new_str, "albums/.") != 0 && strcmp(new_str, "albums/..") != 0 && strcmp(new_str, "albums/.cache") != 0){ 
                struct album *new_album = init_album(ep->d_name, mp);
                albums = append_album(albums, new_album);
            }
            printf("GOODBYE\n");
        }
        (void) closedir(dp);
    }
    return albums;
}

struct song *append_song(struct song *head, struct song *song_to_append) {
    if (head == NULL) {
        return song_to_append;
    } 
    struct song *curr = head;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = song_to_append;
    return head;
}

struct album *append_album(struct album *head, struct album *append) {
    if (head == NULL) {
        return append;
    } 
    struct album *curr = head;
    while (curr->next != NULL) {
        curr = curr->next;
    }
    curr->next = append;
    return head;
}

void display_debug_info(struct music_player *mp, char info[MAX_LEN]) {
    wclear(mp->bot_left_win);
    mvwprintw(mp->bot_left_win, 1, 1, info);
    refresh_win(mp->bot_left_win);    
}

void update_playing_window(struct music_player *mp) {
    wclear(mp->bot_right_win);
    char playing_str[MAX_LEN] = "Now playing: ";
    strcat(playing_str, mp->queue->name);
    mvwprintw(mp->bot_right_win, 1, 1, playing_str);
    if (mp->playing) {
        mvwprintw(mp->bot_right_win, 2, 1, "Playing");
    } else {
        mvwprintw(mp->bot_right_win, 2, 1, "Paused");
    }
    if (mp->shuffle) {
        mvwprintw(mp->bot_right_win, 3, 1, "Shuffle");
    }
    refresh_win(mp->bot_right_win);
}

void refresh_win(WINDOW *win) {
    cchar_t vb;
    setcchar(&vb, L"│", 0, 1, nullptr);
    cchar_t hb;
    setcchar(&hb, L"─", 0, 1, nullptr);
    cchar_t tl;
    setcchar(&tl, L"╭", 0, 1, nullptr);
    cchar_t tr;
    setcchar(&tr, L"╮", 0, 1, nullptr);
    cchar_t bl;
    setcchar(&bl, L"╰", 0, 1, nullptr);
    cchar_t br;
    setcchar(&br, L"╯", 0, 1, nullptr);
    wborder_set(win, 
        &vb, &vb, &hb, &hb, &tl, &tr, &bl, &br 
    );
    wrefresh(win);
    
}

void update_playlist_window(struct music_player *mp) {
    int idx = 1;
    struct album *curr = mp->album_head;
    while (curr != NULL) {
        if (curr == mp->selected_album) {
            mvwprintw(mp->left_win, idx, 1, "> ");
            mvwprintw(mp->left_win, idx, 3, curr->name);
        } else {
            mvwprintw(mp->left_win, idx, 1, curr->name);
        }
        idx++;
        curr = curr->next;
    }
    refresh_win(mp->left_win);
}

void update_song_window(struct music_player *mp) {
    int idx = 1;
    struct song *curr = mp->selected_album->songs;
    while (curr != NULL) {
        if (curr == mp->selected_song) {
            mvwprintw(mp->right_win, idx, 1, ">");
            mvwprintw(mp->right_win, idx, 3, curr->name);
        } else {
            mvwprintw(mp->right_win, idx, 1, curr->name);
        }
        idx++;
        curr = curr->next;
    }
    refresh_win(mp->right_win);
}
    
void load_music(struct music_player *mp) {
    struct album *albums;
    albums = init_albums(mp);
    mp->album_head = albums;
    mp->curr_album = albums;
    mp->selected_album = albums;
    mp->queue = albums->songs;
    mp->selected_song = albums->songs;
    update_playlist_window(mp);
    update_playing_window(mp);
    update_song_window(mp);
    display_debug_info(mp, "Albums loaded!");
}

void action_move_down_song(struct music_player *mp) {
    if (mp->selected_song->next != NULL) {
        mp->selected_song = mp->selected_song->next;
    } else {
        mp->selected_song = mp->selected_album->songs;
    }
    update_song_window(mp);
}

void action_move_up_song(struct music_player *mp) {
    struct song *target_song = mp->selected_song;
    struct song *curr_song = mp->selected_album->songs;
    if (mp->selected_album->songs == mp->selected_song) {
        target_song = NULL;
    }
    while (curr_song->next != target_song) {
        curr_song = curr_song->next;
    }
    mp->selected_song = curr_song;
    update_song_window(mp);
}

void action_play_song_selected(struct music_player *mp) {
    mp->curr_album = mp->selected_album;
    if (mp->playing) {
        ma_sound_stop(&mp->queue->sound);
    }
    uninit_curr_song(mp);
    mp->queue = mp->selected_song;
    ma_sound_seek_to_pcm_frame(&mp->queue->sound, 0);
    init_and_play_curr_song(mp);
    //ma_sound_start(&mp->queue->sound);
    mp->playing = true;
    update_playing_window(mp);
}

void action_move_down_album(struct music_player *mp) {
    if (mp->selected_album->next != NULL) {
        mp->selected_album = mp->selected_album->next;
    } else {
        mp->selected_album = mp->album_head;
    }
    mp->selected_song = mp->selected_album->songs;
    update_playlist_window(mp);
    update_song_window(mp);
}

void action_move_up_album(struct music_player *mp) {
    struct album *target_album = mp->selected_album;
    struct album *curr = mp->album_head;
    if (mp->selected_album == mp->album_head) {
        target_album = NULL;
    }
    while (curr->next != target_album) {
        curr = curr->next;
    }
    mp->selected_album = curr;
    mp->selected_song = mp->selected_album->songs;
    update_playlist_window(mp);
    update_song_window(mp);
}

void callback_test(void *info, ma_sound *sound) {
    struct music_player *mp = (struct music_player*)info;
    ma_sound_stop(&mp->queue->sound);
    display_debug_info(mp, "GOING TO NEXT SONG");
    //uninit_curr_song(mp);
    int num_songs_forward = 1;
    if (mp->shuffle) {
        num_songs_forward = gen_shuffle_num();
    }
    for (int i = 0; i < num_songs_forward; i++) {
        if (mp->queue->next != NULL) {
            mp->queue = mp->queue->next;
        } else {
            mp->queue = mp->curr_album->songs;
        }
    }
    init_and_play_curr_song(mp);
    //ma_sound_start(&mp->queue->sound);
    update_playing_window(mp);
    
}

void cleanup_mp(struct music_player *mp) {
    struct album *free_album;
    struct song *free_song;
    delwin(mp->left_win);
    delwin(mp->right_win);
    delwin(mp->bot_left_win);
    delwin(mp->bot_right_win);
    while (mp->album_head != NULL) {
        free_album = mp->album_head;
        mp->album_head = mp->album_head->next; 
        while (free_album->songs != NULL) {
            free_song = free_album->songs;
            free_album->songs = free_album->songs->next;
            if (free_song->initialized) {
                ma_sound_uninit(&free_song->sound);
            }
            free(free_song);
        }
        free(free_album);
    }
    ma_engine_uninit(&mp->engine);
    free(mp);
    endwin();
    exit_curses(0);
}

void init_and_play_curr_song(struct music_player *mp) {
    if (mp->queue->initialized == false) {
        ma_sound_config soundConfig;
        soundConfig = ma_sound_config_init();
        soundConfig.pFilePath = (const char *)mp->queue->path;
        soundConfig.endCallback = callback_test;
        soundConfig.pEndCallbackUserData = mp;
        ma_sound_init_ex(&mp->engine, &soundConfig, &mp->queue->sound); 
        mp->queue->initialized = true;
        display_debug_info(mp, "Song initialized");
    }
    ma_sound_start(&mp->queue->sound);
}

void uninit_curr_song(struct music_player *mp) {
    if (mp->queue->initialized) {
        ma_sound_uninit(&mp->queue->sound);
        mp->queue->initialized = false;
        display_debug_info(mp, "SONG FREED");
    }
}

void free_unused_songs(struct music_player *mp) {
    struct album *curr_album = mp->album_head;
    while (curr_album != NULL) {
        struct song *curr_song = curr_album->songs;
        while (curr_song != NULL) {
            if (curr_song != mp->queue) {
                if (curr_song->initialized) {
                    ma_sound_uninit(&curr_song->sound);
                    curr_song->initialized = false;
                }
            }
            curr_song = curr_song->next;
        }
        curr_album = curr_album->next;
    }
    display_debug_info(mp, "FREED ALL UNUSED SONGS");
}


void toggle_shuffle(struct music_player *mp) {
    if (mp->shuffle) {
        mp->shuffle = false;
        display_debug_info(mp, "Shuffle off");
    } else {
        mp->shuffle = true;
        display_debug_info(mp, "Shuffle on");
    }
    update_playing_window(mp);
}

int gen_shuffle_num() {
    return rand() % (MAX_SHUFFLE_DISTANCE - MIN_SHUFFLE_DISTANCE + 1) + MIN_SHUFFLE_DISTANCE;
}
