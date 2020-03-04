#include <string>
#include <fstream>
#include <iostream>

class fileReader
{
public:
	struct colour
	{
		float r, g, b, alpha;
	};

	unsigned char* readPPM(char const* fileName, int *x, int *y);

private:
	colour image[1024][1024];
};

unsigned char* fileReader::readPPM(char const* fileName, int *x, int *y)
{
	std::ifstream file;
	file.open(fileName, std::ios::in);

	std::string s;
	for (int i = 1; i <= 4; i++)
		std::getline(file, s);

	for (int y = 1024; y >= 0; y--)
	{
		for (int x = 0; x < 1024; x++)
		{
			file >> image[x][y].r >> image[x][y].g >> image[x][y].b;
			image[x][y].alpha = 1.0f;
		}
	}

	*x = 1024;
	*y = 1024;

	//return (unsigned char*)(image);
}