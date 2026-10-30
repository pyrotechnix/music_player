## Music Player
A simple music player made using the miniaudio library and C. To use, create an albums directory, which contains folders with your audio files.

## Building/dependencies
Requires ncurses. You can build with
```
gcc main.c music_player.c music_player.h miniaudio/miniaudio.h -lncursesw -lm -o ex
```
(note: this isn't very good so it might not work very well)

