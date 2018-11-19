#ifndef _sound_h_
#define _sound_h_ 

void PlaySound(char* sound);
void PlayMusic(char* musicName);
void ProcessMusic();
void OnMusicFinished();
void PauseMusic();
void ResumeMusic();
void StopMusic();
void SoundCleanup();

#endif
