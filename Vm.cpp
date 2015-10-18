#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <fstream>
using namespace std;

enum opCode {
	NEW=1, READ, WRITE, END 
};

typedef struct {
	opCode opC;
	int parameter;
} command;

typedef struct {
	int frame;
	bool dirty;
} pte; //Page Table Entry

typedef map<int, pte> pageTable;

typedef deque<int> frames;

const int PAGE_SIZE_FRAME_SIZE = 1000;
const int MEMORY_SIZE = 3000;
const int NUM_FRAMES = MEMORY_SIZE/PAGE_SIZE_FRAME_SIZE;

void readInCommands(string filename, vector<command> & c);

int main(int argc, char *argv[]) 
{
	vector<command> c;
	readInCommands("Vm.dat", c);
}

/* Function:	readInCommands
 *    Usage:	vector<command> c;
				readInCommands("Vm.dat", c);
 * -------------------------------------------
 * Saves the data in a formatted file (eg. "Vm.dat") into a vector of cpmmands (eg. cd).
 * Each line of the file must contain two numbers separated by a space:
 * 		- The first number is the Op Code (1-4).
 *		- The second number is the parameter (Process Size or Address).
 *		eg. "1 4605"
 */
void readInCommands(string filename, vector<command> & c)
{
	c.clear();
	ifstream f(filename, fstream::in);
	string line;
	while(f.good())
	{
		getline(f, line);
		if(line.length()>0 && isprint(line[0]))
		{
			/* if a number doesnt come next, error */
			if(!isdigit(line[0])) 
			{
				cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - Each line MUST contain an OpCode. 1 (New Job), 2 (Read), 3 (Write), 4 (Job End)" << endl;
				exit(EXIT_FAILURE);
			}
			/**/
			command newC;
			/* save the Op Code if it is in the range 1-4 */
			int end;
			for(end=0; end<line.length() && isdigit(line[end]); end++) {}
			int candidate = stoi(line.substr(0,end));
			if(candidate>=1 && candidate<=4)
			{
				newC.opC = (opCode)candidate;
			}
			else
			{
				cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - The OpCode must be one of the following: 1 (New Job), 2 (Read), 3 (Write), 4 (Job End)" << endl;
				exit(EXIT_FAILURE);
			} 
			/**/
			if(newC.opC!=4)
			{
				/* if a number doesnt come next, error */
				if(!isdigit(line[++end])) //skip over the space
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 1 (New Job), 2 (Read), or 3 (Write) MUST be followed by a single space and a Parameter  (numbers only)" << endl;
					exit(EXIT_FAILURE);
				}
				/**/
				/* save the Parameter */
				int firstDigit = end;
				for(; end<line.length() && isdigit(line[end]); end++) {}
				newC.parameter = stoi(line.substr(firstDigit, end-firstDigit+1));
				/**/
				/* if a something comes next, error */
				if(end<line.length() && isprint(line[end]))
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 1 (New Job), 2 (Read), or 3 (Write) MUST ONLY be followed by a single space and a Parameter (numbers only). No lagging spaces." << endl;
					exit(EXIT_FAILURE);
				}
				/**/
			}
			else
			{
				/* if a something comes next, error */
				if(end<line.length() && isprint(line[end]))
				{
					cerr << "ERROR-- readInCommands: " << filename << " '"<< line << "' - An Op Code of 4 (Job End) does not accept a parameter. No lagging spaces." << endl;
					exit(EXIT_FAILURE);
				}
				/**/
				newC.parameter=0;
			}


			c.push_back(newC);
		}
	}
	f.close();
}