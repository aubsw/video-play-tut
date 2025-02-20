# Video Player
My implementation of Dranger's ffmpeg/dsl tutorial http://dranger.com/ffmpeg/ffmpeg.html

To compile rn:

```bash
gcc -lSDL2 -lSDL2_ttf  -lavcodec -lavformat -lavutil -o main.out main.c
```

And to run:

```bash
./main </path/to/video>
```

Note that I made this with SDL2 and ffmpeg6, which are notably not aligned with what the dranger doc uses. I've had to consult a variety of sources to get this working in 2025.
