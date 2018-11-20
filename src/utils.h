#ifndef _utils_h_
#define _utils_h_ 
#include <SDL.h>
#include <random>
#include <time.h>

class Timer
{
	public:
		Uint32 period;
		bool completed;
		Uint32 oldTicks;
		void Run()
		{
			Uint32 newTicks = SDL_GetTicks();
			completed = false;
			if(newTicks - oldTicks > period)
			{
				completed = true;
				oldTicks = newTicks - ((newTicks - oldTicks) % period);
			}
		};
};

class RandomGenerator
{
	private:
		int seed;
		int seedBackup;
		std::mt19937 gen;

	public:
		RandomGenerator();
		void SetSeed(int seed);
		void Reseed();
		void ExportSeed();
		void SaveSeed();
		void LoadSeed();
		void ResetSequence();
		int Generate(int from, int to);
};

template < class ContainerT >
void tokenize(const std::string& str, ContainerT& tokens,
	const std::string& delimiters = " ", bool trimEmpty = false)
{
	std::string::size_type pos, lastPos = 0;

	using value_type = typename ContainerT::value_type;
	using size_type = typename ContainerT::size_type;

	while(true)
	{
		pos = str.find_first_of(delimiters, lastPos);
		if(pos == std::string::npos)
		{
			pos = str.length();

			if(pos != lastPos || !trimEmpty)
				tokens.push_back(value_type(str.data() + lastPos,
				(size_type)pos - lastPos));

			break;
		}
		else
		{
			if(pos != lastPos || !trimEmpty)
				tokens.push_back(value_type(str.data() + lastPos,
				(size_type)pos - lastPos));
		}

		lastPos = pos + 1;
	}
}

int ConvertToTileCoord(double z, bool nearest);
SDL_Rect GetTileRect(int x, int y);
bool IsOnlyOneBitSet(int b);
int GetDistanceFromPointToLine(SDL_Point line1, SDL_Point line2, SDL_Point point);
std::string IntToString(int var);
int StringToInt(std::string val);
const char* AddLeadingZeroes(int var, int length);
const char* StringToChars(std::string temp);
const char* ConstCharConcat(const char* first, const char* second);
void PrintLog(int logLevel, const char *fmt, ...);
bool GetFolderFileList(std::string folder, std::vector<std::string> &fileList);

#endif
