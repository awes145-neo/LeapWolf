// sdlmixer for the leapster2

#ifndef L2_SDL_MIXER_H
#define L2_SDL_MIXER_H

#include "SDL.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIX_CHANNELS 2  // use voice channels 5 and 6, we don't use 7 unless we wanna hear someone choking while gurgling water
#define AUDIO_U8 0x0008
#define AUDIO_S16 0x8010

typedef struct {
    int allocated;
    Uint8 *abuf;
    Uint32 alen;
    Uint8 volume;
    Uint32 rate;
} Mix_Chunk;

int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
void Mix_CloseAudio(void);
const char *Mix_GetError(void);
int Mix_ReserveChannels(int num);
int Mix_GroupChannels(int from, int to, int tag);
int Mix_GroupAvailable(int tag);
int Mix_GroupOldest(int tag);
int Mix_SetPanning(int channel, Uint8 left, Uint8 right);
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
void Mix_FreeChunk(Mix_Chunk *chunk);
int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
int Mix_HaltChannel(int channel);
void Mix_HookMusic(void (*fn)(void *udata, Uint8 *stream, int len), void *arg);
void Mix_ChannelFinished(void (*fn)(int channel));

Mix_Chunk *l2_chunk_pcm8(const Uint8 *pcm, Uint32 len, Uint32 rate);
Mix_Chunk *l2_chunk_alaw(const Uint8 *alaw, Uint32 len);

#ifdef __cplusplus
}
#endif

#endif
