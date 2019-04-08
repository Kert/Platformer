#include "utils.h"
#include <sstream>
#include "dirent.h"
#include "globals.h"
#include "level.h"

SDL_RWops *logfile;
extern Level *level;

RandomGenerator::RandomGenerator()
{
#ifdef __MINGW32__
	seed = time(NULL);
#else
	std::random_device rd;
	seed = rd();
#endif
	//seed = yourlovelyseed
	//seed = 604403015;
	seed = -191274826;
	SetSeed(seed);
	SaveSeed();
	PrintLog(LOG_IMPORTANT, "seed is %d", seed);
}

void RandomGenerator::SetSeed(int seed)
{
	gen.seed(seed);
}

void RandomGenerator::ExportSeed()
{
	logfile = SDL_RWFromFile("log.txt", "w+");
	if(logfile == NULL)
	{
		PrintLog(LOG_IMPORTANT, "File saving failed wtf");
		return;
	}

	std::string seedString = "Last seed: " + IntToString(seed);
	const char *seed = StringToChars(seedString);

	SDL_RWwrite(logfile, seed, 1, strlen(seed));

	delete seed;

	SDL_RWclose(logfile);
}

void RandomGenerator::SaveSeed()
{
	seedBackup = seed;
}

void RandomGenerator::LoadSeed()
{
	seed = seedBackup;
	ResetSequence();
}

void RandomGenerator::Reseed()
{
	seed = Generate(0, INT_MAX);
	ResetSequence();
}

void RandomGenerator::ResetSequence()
{
	SetSeed(seed);
}

int RandomGenerator::Generate(int from, int to)
{
	std::uniform_int_distribution<> dis(from, to);
	return dis(gen);
}

// Converts a coordinate value into tile coordinate with optional round to nearest
// mode == 0 - floor, mode == 1 - ceil
int ConvertToTileCoord(double z, bool mode)
{
	int result;
	if(mode)
		result = (int)ceil(z / (double)TILESIZE);
	else
		result = (int)floor(z / (double)TILESIZE);

	if(result < 0) result = 0;
	if(result >= level->width_in_tiles) result = level->width_in_tiles - 1;
	return result;
}

// Returns tile rect in raw values (not divided by tilesize)
SDL_Rect GetTileRect(int x, int y)
{
	SDL_Rect rect;
	rect.w = rect.h = TILESIZE;
	rect.x = x * rect.w;
	rect.y = y * rect.h;
	return rect;
}

bool IsOnlyOneBitSet(int b)
{
	return b && !(b & (b - 1));
}

int GetDistanceFromPointToLine(SDL_Point line1, SDL_Point line2, SDL_Point point)
{
	int x, y, x1, y1, x2, y2;
	x = point.x;
	y = point.y;
	x1 = line1.x;
	y1 = line1.y;
	x2 = line2.x;
	y2 = line2.y;
	int dx, dy;
	dx = x2 - x1;
	dy = y2 - y1;
	return (int)(abs(dy*x - dx*y - x1*y2 + x2*y1) / sqrt(double(dx*dx + dy*dy)));
}

std::string IntToString(int var)
{
	std::stringstream o;
	o << var;
	return o.str().c_str();
}

const char* AddLeadingZeroes(int var, int length)
{
	std::string temp = IntToString(var);

	while((int)temp.length() < length)
	{
		temp = "0" + temp;
	}

	return StringToChars(temp);
}

// this function leaks 3 bytes every time it's called
// c needs to be deleted at some point
const char* StringToChars(std::string temp)
{
	char* c = new char[temp.size() + 1];
	std::vector<char> ch;
	for(int i = 0; i < (int)temp.size(); i++)
	{
		ch.push_back(' ');
	}
	std::copy(temp.begin(), temp.end(), ch.begin());
	int at = 0;
	for(auto f : ch)
	{
		c[at] = f;
		at++;
	}
	c[at] = '\0';
	return c;
}

int StringToInt(std::string val)
{
	int temp = -1;
	std::stringstream s;
	s << val;
	s >> temp;
	return temp;
}

const char* ConstCharConcat(const char* first, const char* second)
{
	std::stringstream o;
	o << first;
	//delete first;
	o << second;
	//delete second;
	std::string temp = o.str().c_str();

	return StringToChars(temp);
}

void PrintLog(int logLevel, const char *fmt, ...)
{
	if(logLevel <= LOGDETAIL)
	{
		char buffer[1024];
		va_list list;
		va_start(list, fmt);
		vsprintf(buffer, fmt, list);
		SDL_Log(buffer);
		va_end(list);
	}
}

bool GetFolderFileList(std::string folder, std::vector<std::string> &fileList)
{
	DIR *pDir;
	struct dirent *pDirent;
	pDir = opendir(folder.c_str());
	if(pDir == NULL)
	{
		PrintLog(LOG_DEBUG, "Cannot open directory %s", folder);
		return 0;
	}

	while((pDirent = readdir(pDir)) != NULL)
	{
		if(!strcmp(pDirent->d_name, ".") || !strcmp(pDirent->d_name, ".."))
			continue;

		std::string dir(folder + '\\' + pDirent->d_name);
		DIR *pDir2 = opendir(dir.c_str());
		if(pDir2 == NULL)
			fileList.push_back(pDirent->d_name);
	}
	closedir(pDir);

	return 1;
}

bool HasIntersection(PrecisionRect *a, PrecisionRect *b)
{
	if((a->x > (b->x + b->w)) || (b->x > (a->x + a->w)))
		return false;
	if((a->y > (b->y + b->h)) || (b->y > (a->y + a->h)))
		return false;
	return true;
}