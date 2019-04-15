#ifndef _sound_h_
#define _sound_h_ 

#include <string>

namespace Sound
{
	void Init();
	void PlaySfx(char* sound);
	void PlayMusic(std::string musicName);
	void ProcessMusic();
	void OnMusicFinished();
	void PauseMusic();
	void ResumeMusic();
	void StopMusic();
	void SetSfxVolume(int volume);
	int GetSfxVolume();
	void SetMusicVolume(int volume);
	int GetMusicVolume();
	void Cleanup();
}

#endif
