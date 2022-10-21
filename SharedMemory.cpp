#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/wait.h>

using namespace std;

struct battleInf	//those are communicabled information,not included map of player
{
	int printPosOrd;	//make parent put ship first
	int nowP;	//who fired
	int nowC;	//check damage
	int step;	//record phase(check or fire)
	int player;	//how many player 
	int position;
	int win;
	int use;
	int Ord[3];
	bool wreck[3];
};
struct ship		//ship must have position and attack order
{
	int part1;
	int part2;
	int use = 0;
	int *order;
	bool hurt[2]{};
};

string coor(int input);
void Game(int p);						//main gaming function
void setting(int size, ship& it);		//set the ship to randon position
void printID(char* str);				//print pid and name(parent or child)
void printPos(char* str, ship& it);		//print the ship's potition with (x,y)
void Bomb(battleInf* inf, char*str, ship& sh, int x);			//fire
void check(battleInf* inf, char* str, ship&sh, int x, int n);	//check damage
void finish(battleInf* inf);			//output who win amd how many shell it used


int main(int argc, char *argv[])
{
	if (argv[1][0] == '0')	//choose mode
		Game(2);
	else if (argv[1][0] == '1')
		Game(3);
	else
		return 1;
	return 0;
}

void Game(int p)
{
	int mapSize = 16;	//set the mapsize
	int mark = 0;
	const char *memname = "battleX";
	const size_t memsize = sizeof(battleInf);
	int fd = shm_open(memname, O_CREAT | O_TRUNC | O_RDWR, 0660);   //open shared memory can read and write
	int r = ftruncate(fd, memsize);									//set size of shared memory
	void *ptr = mmap(0, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);	//get shared memory address
	close(fd);
	battleInf* set = (battleInf*)ptr;
	(*set).player = p;		//initialize
	(*set).nowP = 0;
	(*set).printPosOrd = 0;
	(*set).wreck[0] = (*set).wreck[1] = (*set).wreck[2] = false;
	(*set).win = -1;
	(*set).step = 0;
	battleInf* inf = (battleInf*)ptr;
	pid_t pid = fork();		//fork to two different process
	if (pid == 0)
	{
		srand(time(NULL));
		mark = 1;
		pid_t Cpid = 0;
		if ((*set).player == 3)
		{
			Cpid = fork();
			if (Cpid == 0)
			{
				mark++;
				srand(time(NULL) + 2);
			}
		}
		else
			(*set).wreck[2] = true;
		char* name = "Child";
		ship sh;
		setting(mapSize, sh);
		while (true)		//wait parent
			if ((*set).printPosOrd == mark)
			{
				printPos(name, sh);
				(*inf).Ord[mark] = getpid();
				(*inf).printPosOrd = ((*inf).printPosOrd + 1) % (*set).player;
				break;
			}
		while (true)		//fire and check
		{
			if ((*inf).step == 1 && (*inf).Ord[(*inf).nowP] == getpid() && !(*inf).wreck[mark])
				Bomb(inf, name, sh, (*set).player);
			if ((*inf).step == 2 && (*inf).Ord[(*inf).nowC] == getpid() && !(*inf).wreck[mark])
				check(inf, name, sh, (*set).player, mark);
			if ((*inf).player == 1)
			{
				if ((*inf).wreck[mark] == false)
				{
					(*inf).win = getpid();
					(*inf).use = sh.use;
					(*inf).player = 0;
				}
				exit(0);
			}
		}
	}
	else
	{
		char* name = "Parent";
		ship sh;
		srand(time(NULL) + 1);
		setting(mapSize, sh);
		printPos(name, sh);
		(*inf).Ord[0] = getpid();
		(*inf).printPosOrd = ((*inf).printPosOrd + 1) % (*set).player;
		while (true)		//set all of ship
		{
			if ((*inf).printPosOrd == 0)
			{
				(*inf).step = 1;
				break;
			}
		}
		while (true)		//fire and check
		{
			if ((*inf).step == 1 && (*inf).Ord[(*inf).nowP] == getpid() && !(*inf).wreck[mark])
				Bomb(inf, name, sh, (*set).player);
			if ((*inf).step == 2 && (*inf).Ord[(*inf).nowC] == getpid() && !(*inf).wreck[mark])
				check(inf, name, sh, (*set).player, mark);
			if ((*inf).player == 1)
				if ((*inf).wreck[mark] == false)
				{

					(*inf).win = getpid();
					(*inf).use = sh.use;
					(*inf).player = 0;
				}
			if ((*inf).player == 0)
			{
				printID("Parent");
				finish(inf);
				break;
			}
		}
	}
	r = munmap(ptr, memsize);
	if (r != 0)
		cout << "munmap";
	r = shm_unlink(memname);
	if (r != 0)
		cout << "shm_unlink";
}

void setting(int size, ship& it)
{
	int direct = rand() % 4;
	it.part1 = rand() % 16;
	switch (direct)			//set the position of ship
	{
	case 0:
		if (it.part1 % 4 == 3)
			it.part2 = it.part1 - 1;
		else
			it.part2 = it.part1 + 1;
		break;
	case 1:
		if (it.part1 % 4 == 0)
			it.part2 = it.part1 + 1;
		else
			it.part2 = it.part1 - 1;
		break;
	case 2:
		it.part2 = it.part1 + 4;
		if (it.part2 > 15)
			it.part2 -= 8;
		break;
	case 3:
		it.part2 = it.part1 - 4;
		if (it.part2 < 0)
			it.part2 += 8;
		break;
	default:
		break;
	}
	if (it.part1 > it.part2)
		swap(it.part1, it.part2);
	it.order = new int[size]();		//make attack order random and not repeating
	for (int i = 0; i < size; i++)
		it.order[i] = i;
	for (int i = size; i > 0; i--)
	{
		if (it.order[i] == i)
		{
			int buffer = rand() % i;
			swap(it.order[i], it.order[buffer]);
		}
	}
}

string coor(int input)
{
	string buffer;
	buffer = "(0,0)";
	buffer[1] = input / 4 + '0';
	buffer[3] = input % 4 + '0';
	return buffer;
}

void printID(char* str)
{
	cout << "[" << getpid() << " " << str << "]: ";
}

void printPos(char* str, ship& it)
{
	printID(str);
	cout << "The gunboat: " << coor(it.part1) << coor(it.part2) << endl;
}

void Bomb(battleInf* inf, char* str, ship& sh, int p)
{
	printID(str);
	(*inf).position = sh.order[sh.use];
	cout << "bombing " << coor(sh.order[sh.use++]) << endl;
	(*inf).nowC = ((*inf).nowP + 1) % p;
	(*inf).step = 2;
}

void check(battleInf* inf, char* str, ship&sh, int p, int n)
{
	if ((*inf).nowC == (*inf).nowP)	//need to next fire
	{
		int count = 0;
		for (int i = 0; i < 3; i++)
			if ((*inf).wreck[i] == false)
				count++;
		if (count == 1)				//count = how many surcivor -> count == 1 mean someone win 
		{
			(*inf).win = getpid();
			(*inf).use = sh.use;
			return;
		}
		(*inf).nowP = ((*inf).nowP + 1) % (*inf).player;
		(*inf).nowC = ((*inf).nowP + 1) % (*inf).player;
		(*inf).step = 1;
	}
	else
	{
		printID(str);
		bool flag = false;
		if ((*inf).position == sh.part1)
		{
			flag = true;
			sh.hurt[0] = true;
		}
		else if ((*inf).position == sh.part2)
		{
			flag = true;
			sh.hurt[1] = true;
		}
		else
			cout << "missed\n";
		if (flag)
			if (sh.hurt[1] && sh.hurt[0])
			{
				(*inf).wreck[n] = true;
				cout << "hit and sinked\n";
				(*inf).player--;
				p = (*inf).player;
				if ((*inf).player > 1)	//3 player case,shared memory use std::queue have some problem so i use 
				{								//this way to  make queue for 3player
					if ((*inf).wreck[0])
					{
						swap((*inf).Ord[0], (*inf).Ord[1]);
						swap((*inf).Ord[1], (*inf).Ord[2]);
					}
					if ((*inf).wreck[1])
						swap((*inf).Ord[1], (*inf).Ord[2]);
					if ((*inf).nowP == 0 && (*inf).nowC == 1)
						(*inf).nowP = 0, (*inf).nowC = 1;
					else if ((*inf).nowP == 0 && (*inf).nowC == 2)
						(*inf).nowP = 0, (*inf).nowC = 0;
					else if ((*inf).nowP == 1 && (*inf).nowC == 2)
						(*inf).nowP = 1, (*inf).nowC = 0;
					else if ((*inf).nowP == 1 && (*inf).nowC == 0)
						(*inf).nowP = 0, (*inf).nowC = 1;
					else if ((*inf).nowP == 2 && (*inf).nowC == 0)
						(*inf).nowP = 1, (*inf).nowC = 0;
					else if ((*inf).nowP == 2 && (*inf).nowC == 1)
						(*inf).nowP = 1, (*inf).nowC = 1;
					return;
				}
			}
			else
				cout << "hit\n";
		if ((*inf).player == 1)
			(*inf).nowC = 2;
		else
			(*inf).nowC = ((*inf).nowC + 1) % p;
	}

}

void finish(battleInf* inf)
{
	cout << (*inf).win << " wins with " << (*inf).use << " bombs\n";
}