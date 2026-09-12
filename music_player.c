#define NCURSES_WIDECHAR 1

// My glorious king
// https://stackoverflow.com/questions/79891188/bold-box-style-in-ncurses

#include <stdio.h>
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
    init_color(COLOR_GREY, 1000, 0, 1000);
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
    endwin();
    ma_engine_uninit(&mp->engine);
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
    }
    //display(mp);
}

struct song *init_song(char name[MAX_LEN], struct music_player *mp) {
    char info_str[MAX_LEN] = "LOADING SONG ";
    strcat(info_str, name);
    display_debug_info(mp, info_str);
    struct song *new_song = malloc(sizeof(struct song));
    new_song->next = NULL;
    // MAKE SURE TO INIT THE SOUND DIRECTLY HERE INSTEAD OF MAKING THE SOUND SOMETHING ELSE AND INITIALIZING IT ELSEWHERE
    // FOR SOME REASON THE BRAINDEAD PROGRAMMERS OF MINIAUDIO DECIDED THAT DOING IT THE WAY LITERALLY EVERY OTHER PROGRAM 
    // WORKS AND MAKING A NEW VARIABLE AND THEN ASSIGNING new_song->sound = new_sound WAS TOO EASY SO THEY JUST HAD TO 
    // MAKE IT SHITTIER LIKE THIS
    ma_sound_config soundConfig;
    soundConfig = ma_sound_config_init();
    soundConfig.pFilePath = (const char *)name;
    soundConfig.endCallback = callback_test;
    soundConfig.pEndCallbackUserData = mp;
    /*
    ma_sound_init_from_file(
        &mp->engine, 
        //name, 
        name,
        MA_SOUND_FLAG_DECODE, 
        NULL, 
        NULL, 
        &new_song->sound
    );
    */
    ma_sound_init_ex(&mp->engine, &soundConfig, &new_song->sound); 
    return new_song;
}

struct album *init_album(char name[MAX_LEN], struct music_player *mp) {
    DIR *album_dir;
    struct dirent *album_dir_ent;
    char album_dir_name[MAX_LEN] = "albums/";
    strcat(album_dir_name, name);
    album_dir = opendir(album_dir_name);
    struct song *song_list = NULL;
    //printf("VVVVVVVVVVVVVVVVVVVVVVVV\n");
    while ((album_dir_ent = readdir(album_dir)) != NULL) {
        if (strcmp(album_dir_ent->d_name, ".") != 0 && strcmp(album_dir_ent->d_name, "..") != 0) {
            char song_str[MAX_LEN];
            strncpy(song_str, album_dir_name, MAX_LEN);
            strcat(song_str, "/");
            strcat(song_str, album_dir_ent->d_name);
            //printf("%s\n", song_str);
            struct song *add_song = init_song(song_str, mp);
            strncpy(add_song->name, album_dir_ent->d_name, MAX_LEN);
            song_list = append_song(song_list, add_song);
            /*
            ma_sound song;
            
            ma_sound_init_from_file(
                engine, 
                song_str,
                MA_SOUND_FLAG_DECODE, 
                NULL, 
                NULL, 
                &song
            );
            
            ma_sound_start(&song);
            getchar();
            ma_sound_stop(&song);
            
            ma_sound_uninit(&song);
            */
        } 
    }
    struct album *new_album = malloc(sizeof(struct album));
    new_album->songs = song_list;
    new_album->next = NULL;
    strncpy(new_album->name, name, MAX_LEN);
    //printf("^^^^^^^^^^^^^^^^^^^^^^^^\n\n");
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
    display(mp);
    return mp;
}

void action_play(struct music_player *mp) {
    if (mp->playing) { 
        ma_sound_stop(&mp->queue->sound);
        mp->playing = false;
    } else {
        ma_sound_start(&mp->queue->sound);
        mp->playing = true;
    }
    update_playing_window(mp);
}

void display(struct music_player *mp) {
    /*
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
    wborder_set(mp->left_win, 
        &vb, &vb, &hb, &hb, &tl, &tr, &bl, &br 
    );
    wborder_set(mp->right_win, 
        &vb, &vb, &hb, &hb, &tl, &tr, &bl, &br 
    );
    wborder_set(mp->bot_left_win, 
        &vb, &vb, &hb, &hb, &tl, &tr, &bl, &br 
    );
    wborder_set(mp->bot_right_win, 
        &vb, &vb, &hb, &hb, &tl, &tr, &bl, &br 
    );
    wrefresh(mp->left_win);
    wrefresh(mp->right_win);
    wrefresh(mp->bot_left_win);
    wrefresh(mp->bot_right_win);
    */
    refresh_win(mp->left_win);    
    refresh_win(mp->right_win);    
    refresh_win(mp->bot_left_win);    
    refresh_win(mp->bot_right_win);    
}


// For some reason calling this function causes the program to hang
// even though the program has finished executing??
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
            char new_str[MAX_LEN]  = "albums/";
            strcat(new_str, ep->d_name);
            printf("%s\n", ep->d_name);
            
            if (strcmp(new_str, "albums/.") != 0 && strcmp(new_str, "albums/..") != 0){ 
                struct album *new_album = init_album(ep->d_name, mp);
                albums = append_album(albums, new_album);
            }
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

// Something is broken
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
    mp->queue = mp->selected_song;
    ma_sound_seek_to_pcm_frame(&mp->queue->sound, 0);
    ma_sound_start(&mp->queue->sound);
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

// Info is not working properly and getting a null pointer
// causing it to segfault. Look into how to set the info
void callback_test(void *info, ma_sound *sound) {
    struct music_player *mp = (struct music_player*)info;
    ma_sound_stop(&mp->queue->sound);
    if (mp->queue->next != NULL) {
        mp->queue = mp->queue->next;
    } else {
        mp->queue = mp->curr_album->songs;
    }
    ma_sound_start(&mp->queue->sound);
    update_playing_window(mp);
    
}


