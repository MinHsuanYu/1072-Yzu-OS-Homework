#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;
int available[5], Max[100][5], allocation[100][5], need[100][5];
vector<int*>waiting;
vector<int>gid, safe;
void input(ifstream &fin)
{
	stringstream sbuf;
	string buffer;
	int x;
	getline(fin, buffer, '\n');
	while(true)
	{
		if (buffer == "#AVAILABLE")
		{
			getline(fin, buffer, '\n');
			getline(fin, buffer, '\n');
			sbuf << buffer;
			for (int i = 0; i < 5; i++)
				sbuf >> available[i];
			sbuf.str("");
			sbuf.clear();
			getline(fin, buffer, '\n');
		}
		else if (buffer == "#MAX")
		{
			getline(fin, buffer, '\n');
			while (getline(fin, buffer, '\n'))
			{
				if (buffer[0] == '#')
					break;
				else
				{
					sbuf << buffer;
					sbuf >> x;
					gid.push_back(x);
					for (int i = 0; i < 5; i++)
						sbuf >> Max[x][i];
					sbuf.str("");
					sbuf.clear();
				}
			}
		}
		else if (buffer == "#ALLOCATION")
		{
			getline(fin, buffer, '\n');
			while (getline(fin, buffer, '\n'))
			{
				if (buffer[0] == '#')
					break;
				else
				{
					sbuf << buffer;
					sbuf >> x;
					for (int i = 0; i < 5; i++)
						sbuf >> allocation[x][i];
					sbuf.str("");
					sbuf.clear();
				}
			}
		}
		else if(buffer=="#REQUEST")
		{
			getline(fin, buffer, '\n');
			return;
		}
	}
}
void need_update()
{
	int times = gid.size();
	for (int i = 0; i < times; i++)
	{
		int row = gid[i];
		for (int j = 0; j < 5; j++)
			need[row][j] = Max[row][j] - allocation[row][j];
	}
}
bool check_safe()
{
	vector<int> check;
	int buf[5], times = gid.size();
	for (int i = 0; i < times; i++)
	{
		int row = gid[i];
		check.push_back(row);
		for (int j = 0; j < 5; j++)
			need[row][j] = Max[row][j] - allocation[row][j];
	}
	for (int i = 0; i < 5; i++)
		buf[i] = available[i];

	for (int i = 0; i < times; i++)
		for (int j = 0; j < check.size(); j++)
		{
			int row = check[j];
			bool s = true;
			for (int k = 0; k < 5; k++)
				if (need[row][k] > buf[k])
					s = false;
			if (s)
			{
				check.erase(check.begin() + j);
				safe.push_back(row);
				for (int k = 0; k < 5; k++)
					buf[k] += allocation[row][k];
				break;
			}
			else
				continue;
		}
	if (safe.size() == times)
		return true;
	else
		return false;
}
void print_safe()
{
	cout << '(' << safe[0];
	for (int i = 1; i < safe.size(); i++)
		cout << ',' << safe[i];
	cout << ')' << endl;
}
void need_print(int index,int input[])
{
	cout << '(' << index;
	for (int i = 0; i < 5; i++)
		cout << ", " << input[i];
	cout << ')';
}
void available_print(int out[])
{
	cout << '(' << out[0];
	for (int i = 1; i < 5; i++)
		cout << ", " << out[i];
	cout << ")\n";
}
bool init_safe()
{
	cout << "Initial state: ";
	if (check_safe())
	{
		cout << "safe, safe sequence = ";
		print_safe();
		return true;
	}
	else
	{
		cout << "unsafe\n";
		return false;
	}
}
void process_print(int id,int input[])
{
	int buf[5];
	for (int i = 0; i < 5; i++)
		buf[i] = available[i];
	for (int i = 0; i < safe.size(); i++)
	{
		int row = safe[i];
		need_print(id,input);
		cout << ": AVAILABLE = ";
		available_print(buf);
		for (int j = 0; j < 5; j++)
			buf[j] += allocation[row][j];
		need_print(id, input);
		cout << ": gid " << row << " finish, AVAILABLE = ";
		available_print(buf);
	}
}
void allocate(int id,int input[])
{
	bool invalid = false;
	for (int i = 0; i < 5; i++)
		if (input[i] > need[id][i])
			invalid = true;
	if (invalid)
	{
		need_print(id, input);
		cout << ": invalid request, not granted\n";
	}
	else
	{
		bool granted = true;
		for (int i = 0; i < 5; i++)
			if (input[i] > available[i])
				granted = false;
		if (granted)
		{
			for (int i = 0; i < 5; i++)
			{
				available[i] -= input[i];
				allocation[id][i] += input[i];
			}
			safe.clear();
			check_safe();
			process_print(id,input);
			need_print(id, input);
			cout << ": granted, safe sequence = ";
			print_safe();
		}
		else
		{
			int* buffer = new int[6];
			for (int i = 0; i < 5; i++)
				buffer[i] = input[i];
			buffer[5] = id;
			waiting.push_back(buffer);
			need_print(id, input);
			cout << ": not granted, put it into queue\n";
		}
	}
}
void release(int id,int input[])
{
	bool valid = true;
	for (int i = 0; i < 5; i++)
		if (input[i] > allocation[id][i])
			valid = false;
	if (valid)
	{
		for (int i = 0; i < 5; i++)
		{
			available[i] += input[i];
			allocation[id][i] -= input[i];
		}
		need_print(id, input);
		cout << ": release success\n";
		int p, pa[5];
		for (int i = 0; i < waiting.size(); i++)
		{
			p = waiting[i][5];
			for (int j = 0; j < 5; j++)
				pa[j] = waiting[i][j];
			safe.clear();
			bool granted = true;
			for (int j = 0; j < 5; j++)
				if (pa[j] > available[j])
					granted = false;
			if (granted)
			{
				for (int j = 0; j < 5;j++)
				{
					available[j] -= pa[j];
					allocation[p][j] += pa[j];
				}
				check_safe();
				process_print(p,pa);
				need_print(p, pa);
				cout << ": granted(from queue), safe sequence = ";
				print_safe();
				waiting.erase(waiting.begin() + i);
				i--;
			}
			else
				continue;
		}
	}
	else
	{
		need_print(id, input);
		cout << ": gid " << id << " doesn't have those resource\n";
	}
}
void allocate_or_release(ifstream &fin)
{
	stringstream sbuf;
	string buf;
	int id, input[5];
	char choice;
	while (getline(fin, buf, '\n'))
	{
		sbuf << buf;
		sbuf >> id;
		for (int i = 0; i < 5; i++)
			sbuf >> input[i];
		sbuf >> choice;
		need_update();
		if (choice == 'a')
			allocate(id, input);
		else if (choice == 'r')
			release(id, input);
		sbuf.str("");
		sbuf.clear();
	}
}
int main(int argc,char* argv[])
{
	ifstream infile(argv[1]);
	input(infile);
	if (!init_safe())
		return 0;
	allocate_or_release(infile);
}