#include "sound.h"
#include <map>
#include "SDL_mixer.h"
#include "globals.h"
#include "utils.h"

std::map<std::string, Mix_Chunk*> loadedSounds;

Mix_Music *activeMusic = NULL;
bool restartMusic = false;

void PlaySound(char* soundName)
{
	char soundFile[50];

	strcpy(soundFile, "assets/sounds/");
	strcat(soundFile, soundName);
	strcat(soundFile, ".wav");

	Mix_Chunk *activeSound = NULL;
	if(loadedSounds.find(soundFile) == loadedSounds.end())
	{
		activeSound = Mix_LoadWAV(soundFile);
		loadedSounds[soundFile] = activeSound;
	}
	else
		activeSound = loadedSounds[soundFile];
	Mix_PlayChannel(-1, activeSound, 0);
}

void PlayMusic(char* musicName)
{
	char musicFile[50];

	strcpy(musicFile, "assets/music/");
	strcat(musicFile, musicName);
	strcat(musicFile, ".ogg");
	
	activeMusic = Mix_LoadMUS(musicFile);
	if (!activeMusic) {
		PrintLog(LOG_IMPORTANT, "Mix_LoadMUS: %s\n", Mix_GetError());
	}
	else
	{
		Mix_PlayMusic(activeMusic, 1);
		// use RestartMusic for when music stops
		Mix_HookMusicFinished(OnMusicFinished);
	}
}

void ProcessMusic()
{
	if (restartMusic)
	{
		Mix_PlayMusic(activeMusic, 1);
		restartMusic = false;
	}
}

// Never call SDL_Mixer functions inside this callback
void OnMusicFinished()
{
	restartMusic = true;
}

void PauseMusic()
{
	Mix_PauseMusic();
}

void ResumeMusic()
{
	Mix_ResumeMusic();
}

void StopMusic()
{
	Mix_HaltMusic();
	Mix_FreeMusic(activeMusic);
	activeMusic = NULL;
}

void SoundCleanup()
{
	for(auto &i : loadedSounds)
	{
		Mix_FreeChunk(i.second);
	}
	// Close sound mixer
	while (Mix_Init(0))
		Mix_Quit();
	Mix_CloseAudio();
}
