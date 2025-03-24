#pragma once
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#define FRAME_BUFFER_SIZE 60

typedef struct {
    AVFrame *frames[FRAME_BUFFER_SIZE];
    int front;
    int cnt;
    pthread_mutex_t mutex;
    pthread_cond_t buffer_is_not_full;
    pthread_cond_t buffer_has_stuff;
} FrameQ;

void push_q(FrameQ *q, AVFrame const *f) {
    pthread_mutex_lock(&q->mutex);
    if (q->cnt >= FRAME_BUFFER_SIZE) {
        pthread_cond_wait(&q->buffer_is_not_full, &q->mutex);
    }
    assert(q->cnt <= FRAME_BUFFER_SIZE);
    const int next_elt = (q->front + q->cnt) % FRAME_BUFFER_SIZE;
    av_frame_copy(q->frames[next_elt], f);
    q->cnt++;
    pthread_cond_signal(&q->buffer_has_stuff);
    pthread_mutex_unlock(&q->mutex);
}

void pop_q(FrameQ *q, AVFrame *dest_f) {
    pthread_mutex_lock(&q->mutex);
    if (q->cnt <= 0) {
        pthread_cond_wait(&q->buffer_has_stuff, &q->mutex);
    }
    av_frame_copy(dest_f, q->frames[q->front]);
    q->front = (q->front + 1) % FRAME_BUFFER_SIZE;
    q->cnt--;
    pthread_cond_signal(&q->buffer_is_not_full);
    pthread_mutex_unlock(&q->mutex);
}

