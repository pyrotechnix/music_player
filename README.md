## Music Player
A simple music player made using the miniaudio library and C. To use, create an albums directory, which contains folders with your audio files.

## Building/dependencies
Requires ncurses. You can build with
```
gcc main.c music_player.c music_player.h miniaudio/miniaudio.h -lncursesw -lm -o ex
```
(note: this isn't very good so it might not work very well. Currently, requires mp3 files to have metadata removed/be encoded in a certain way. To do this, you can use the bash script)
```
# Generates a 'clean' directory inside your current directory and does some metadata removing/encoding stuff to make them work

mkdir clean
for filename in *; do ffmpeg -i "${filename}" -map_metadata -1 -c:a libmp3lame "clean/${filename}"; done
```
