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
