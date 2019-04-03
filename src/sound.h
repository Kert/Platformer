#ifndef _sound_h_
#define _sound_h_ 

void InitSound();
void PlaySfx(char* sound);
void PlayMusic(char* musicName);
void ProcessMusic();
void OnMusicFinished();
void PauseMusic();
void ResumeMusic();
void StopMusic();
void SetSfxVolume(int volume);
int GetSfxVolume();
void SetMusicVolume(int volume);
int GetMusicVolume();
void SoundCleanup();

#endif
