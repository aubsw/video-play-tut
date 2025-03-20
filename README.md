# Video Player
My implementation of Dranger's ffmpeg/dsl tutorial http://dranger.com/ffmpeg/ffmpeg.html

Some dev environment setup for apple silicon mac:

```bash
brew install sdl2 sdl2_ttf ffmpeg pkg-config
```

To compile (mac):

```bash
gcc -g -lavcodec -lavformat -lavutil -o main.out main.c $(pkg-config --cflags --libs sdl2 sdl2_ttf)
```

And to run:

```bash
./main </path/to/video>
```

Note that I made this with SDL2 and ffmpeg6, which are notably not aligned with what the dranger doc uses. I've had to consult a variety of sources to get this working in 2025.
