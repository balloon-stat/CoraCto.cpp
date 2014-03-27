#include "stdafx.h"

std::ostream& operator<<(std::ostream& out, Jewel jwl)
{
	const char* view[8] = { "■", "○", "◎", "△", "◇", "☆", "▽", "　" };
	return out << view[jwl];
}
class Chunk
{
public:
	static const int SIZE = 3;
	int x = 3;
	int y = 0;
	bool handle = false;
	Jewel content[SIZE];
	Jewel nextCn[SIZE];
private:
	std::random_device rd;
	std::vector<int>* vec;
	std::mt19937* engine;
	std::uniform_int_distribution<int>* distribution;
public:
	Chunk()
	{
		vec = new std::vector<int>(10);
		std::generate(vec->begin(), vec->end(), std::ref(rd));
		engine = new std::mt19937(std::seed_seq(vec->begin(), vec->end()));
		distribution = new std::uniform_int_distribution<int>(1, 6);
		gen();
	}
	~Chunk()
	{
		delete vec;
		delete engine;
		delete distribution;
	}
	void gen()
	{
		auto rnd = [&]{ return (*distribution)(*engine); };
		for (int i = 0; i < SIZE; i++)
			nextCn[i] = (Jewel)rnd();
	}
	void next()
	{
		memcpy_s(content, sizeof(content), nextCn, sizeof(nextCn));
		gen();
		x = 3;
		y = 0;
	}
	void cycle()
	{
		Jewel swap = content[0];
		content[0] = content[1];
		content[1] = content[2];
		content[2] = swap;
	}
};
class Field
{
public:
	static const int WIDTH = 6 + 1 + 1;
	static const int HEIGHT = 13 + 3 + 1;
	int score = 0;
private:
	Jewel field[HEIGHT][WIDTH];
public:
	void clear()
	{
		for (int y = 0; y < HEIGHT; y++)
		for (int x = 0; x < WIDTH; x++)
		if (x == 0 || x == WIDTH - 1 || y == HEIGHT - 1)
			field[y][x] = Jewel::WALL;
		else
			field[y][x] = Jewel::NONE;
	}
	Field()
	{
		clear();
	}
	std::string line(int y)
	{
		std::ostringstream out;
		for (int x = 0; x < WIDTH; x++)
			out << field[y][x];
			
		return out.str();
	};
	void put(Chunk* chn)
	{
		for (int i = 0; i < Chunk::SIZE; i++)
			field[chn->y + i][chn->x] = chn->content[i];
	}
	bool drop()
	{
		bool dropped = true;
		for (int y = HEIGHT - 2; y > 0; y--)
		for (int x = WIDTH - 2; x > 0; x--)
		if (field[y][x] == Jewel::NONE && field[y - 1][x] != Jewel::NONE) {
			field[y][x] = field[y - 1][x];
			field[y - 1][x] = Jewel::NONE;
			dropped = false;
		}
		return dropped;
	}
	int opMove(int y, int x, int dir)
	{
		if (field[y + 2][x + dir] == Jewel::NONE)
		{
			for (int i = 0; i < Chunk::SIZE; i++)
			{
				field[y + i][x + dir] = field[y + i][x];
				field[y + i][x] = Jewel::NONE;
			}
			return x + dir;
		}
		return x;
	}
	void renew(Chunk* chn)
	{
		for (int i = 0; i < Chunk::SIZE; i++)
			field[chn->y + i][chn->x] = chn->content[i];
	}
	bool isOver()
	{
		for (int y = 0; y < 3; y++)
		for (int x = 1; x < 7; x++)
		if (field[y][x] != Jewel::NONE)
			return true;

		return false;
	}
	bool vanish()
	{
		bool result = false;
		const int dirs[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };

		for (int i = 0; i < 4; i++)
		{
			int dx = dirs[i][0];
			int dy = dirs[i][1];
			for (int y = 1; y < HEIGHT - 1; y++)
			for (int x = 1; x < WIDTH - 1; x++)
			if (field[y][x] != Jewel::NONE && field[y][x] == field[y + dy][x + dx])
			{
				int n = 2;
				while (field[y][x] == field[y + dy * n][x + dx * n])
					n++;
				if (n > 2)
				{
					result = true;
					score += 100 * n * n;
					for (n--; n >= 0; n--)
						field[y + dy * n][x + dx * n] = Jewel::NONE;
				}
			}
		}
		return result;
	}
};
class Window
{
private:
	static const int WIDTH = 80 / 2 * 3;
	static const int HEIGHT = 24;
	HANDLE hSrc;
	HANDLE hSrc0;
	HANDLE hSrc1;
public:
	char chars[HEIGHT][WIDTH + 1];
	Window()
	{
		for (int y = 0; y < HEIGHT; y++)
			chars[y][0] = '\0';

		hSrc0 = CreateConsoleScreenBuffer(
			GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		if (hSrc0 == INVALID_HANDLE_VALUE) throw;
		hSrc1 = CreateConsoleScreenBuffer(
			GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
		if (hSrc1 == INVALID_HANDLE_VALUE) throw;
		hSrc = hSrc0;
	}
	~Window()
	{
		CloseHandle(hSrc0);
		CloseHandle(hSrc1);
		hSrc = nullptr;
		hSrc0 = nullptr;
		hSrc1 = nullptr;
	}
	void print()
	{
		std::ostringstream out;
		for (int y = 0; y < HEIGHT; y++)
		{
			out << chars[y] << std::endl;
			chars[y][0] = '\0';
		}
		auto str = out.str();

		//１、cmd.exeのcls命令を使う方法
		//system("cls");

		//２、Win32APIでカーソル位置を操作する方法
		//COORD coord = { 0, 0 };
		//SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
		
		//std::cout << str;

		//３、Win32APIでスクリーンバッファを切り替える方法
		DWORD cell;
		WriteConsoleA(hSrc, str.c_str(), str.length(), &cell, NULL);
		SetConsoleActiveScreenBuffer(hSrc);
		hSrc = (hSrc == hSrc0) ? hSrc1 : hSrc0;
	}
	void add(int y, const std::string& content)
	{
		std::string str = chars[y] + content;
		strcpy_s(chars[y], str.c_str());
	}
	void write(Field* f, Chunk* chn)
	{
		int dy = 2;
		for (int y = 3; y < Field::HEIGHT; y++)
			add(y + dy, "　　　　　　" + f->line(y));

		writeNext(chn, dy);
		writeScore(f->score, dy);
	}
	void writeGameOver()
	{
		int y = 7;
		strcpy_s(chars[y + 0], "　　　　　┏━━━━━━━━━━━━━━━━━━━━━━┓");
		strcpy_s(chars[y + 1], "　　　　　┃　　　　　　　　　　　　　　　　　　　　　　┃");
		strcpy_s(chars[y + 2], "　　　　　┃  　　　　　　　 GAME OVER!　　　　　　　   ┃");
		strcpy_s(chars[y + 3], "　　　　　┃　　　　　　　　　　　　　　　　　　　　　　┃");
		strcpy_s(chars[y + 4], "　　　　　┗━━━━━━━━━━━━━━━━━━━━━━┛");
	}
	void writeNext(Chunk* chn, int dy)
	{
		int y = 3 + dy, d = 2;
		add(y + 0, "　　　　　┏━━━┓");
		add(y + 1, "　　　　　┃　　　┃");
		add(y + 5, "　　　　　┃　　　┃");
		add(y + 6, "　　　　　┗━━━┛");

		for (int i = 0; i < 3; i++)
		{
			std::ostringstream out;
			out << chn->nextCn[i];
			add(y + i + d, "　　　　　┃　");
			add(y + i + d, out.str());
			add(y + i + d, "　┃");
		}
	}
	void writeScore(int score, int dy)
	{
		int y = 12 + dy, d = 2;
		add(y + 0, "　　　┏━ ＳＣＯＲＥ ━━━━┓");
		add(y + 1, "　　　┃　　　　　　　　　　　┃");
		add(y + 3, "　　　┃　　　　　　　　　　　┃");
		add(y + 4, "　　　┗━━━━━━━━━━━┛");

		std::string blank;
		if (score == 0)
			blank = "                 ";
		else
		for (int i = 17, min = (int)log10(score); i > min; i--)
			blank += " ";

		std::ostringstream out;
		out << score;
		add(y + d, "　　　┃" + blank);
		add(y + d, out.str());
		add(y + d, "　　┃");
	}
};
class Director
{
private:
	int interval = 10;
	int count = 0;
	Window* win;
	Field* field;
	Chunk* chn;
	Key input()
	{
		switch (_getch())
		{
		case 0x0d: return Key::ENTER;
		case 0x1b: return Key::ESC;
		case 0xe0:
			switch (_getch())
			{
			case 0x48: return Key::UP;
			case 0x50: return Key::DOWN;
			case 0x4d: return Key::RIGHT;
			case 0x4b: return Key::LEFT;
			default:
				break;
			}
		default:
			return Key::NOP;
		}

	}
	void operate()
	{
		if (!_kbhit()) return;

		if (chn->handle)
		switch (input())
		{
		case Key::RIGHT:
			chn->x = field->opMove(chn->y, chn->x, 1);
			break;
		case Key::LEFT:
			chn->x = field->opMove(chn->y, chn->x, -1);
			break;
		case Key::UP:
			chn->cycle();
			field->renew(chn);
			break;
		case Key::DOWN:
			while (!field->drop())
				;
			break;
		case Key::ESC:
			work = false;
			break;
		default:
			break;
		}
	}
	void over()
	{
		win->write(field, chn);
		win->writeGameOver();
		win->print();
		_getch();
		field->score = 0;
		field->clear();
	}
public:
	bool work = true;
	Director()
	{
		win = new Window();
		field = new Field();
		chn = new Chunk();
	}
	~Director()
	{
		delete win;
		delete field;
		delete chn;
	}
	void proc()
	{
		count++;
		operate();
		if (count >= interval)
		{
			count = 0;
			if (field->drop())
				dropped();
			else if (chn->handle)
				chn->y += 1;
		}
		win->write(field, chn);
		win->print();
	}
	void dropped()
	{
		chn->handle = false;
		if (field->vanish()) return;
		if (field->isOver()) over();
		chn->next();
		chn->handle = true;
		field->put(chn);
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	const int time_span = 16 * 4;
	std::chrono::system_clock::time_point wake_up_time;
	Director dr;

	while (dr.work)
	{
		dr.proc();

		wake_up_time = std::chrono::system_clock::now() + std::chrono::milliseconds(time_span);
		while (std::chrono::system_clock::now() < wake_up_time)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return 0;
}


