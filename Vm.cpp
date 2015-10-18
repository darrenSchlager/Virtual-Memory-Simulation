#include <iostream>
#include <vector>
#include <deque>
#include <map>
#include <fstream>
using namespace std;

enum opCode {
	NEW=1, READ, WRITE, END 
};

enum replacementAlgorithm {
	FIFO, LRU, OPTIMAL
};

typedef struct {
	opCode opC;
	string parameter;
} command;

typedef struct {
	int frame;
	bool dirty;
} pte; //Page Table Entry

typedef map<int, pte> pageTable;

typedef deque<int> frames; //hold the frames currently in use

typedef struct {
	bool valid;
	int size;
	pageTable pt;
} process;

const int PAGE_SIZE_FRAME_SIZE = 1000;
const int MEMORY_SIZE = 3000;
const int NUM_FRAMES = MEMORY_SIZE/PAGE_SIZE_FRAME_SIZE;

void processCommands(replacementAlgorithm algorithm, const vector<command> &c, int & totalProcesses, int &totalHits, int &totalFaults);
void processPageHit(replacementAlgorithm algorithm, frames & f, process & p, int page, string offset, int & numHits);
void processPageFault(frames & f, process & p, int page, string offset, int & numFaults);
void processPageFaultOPTIMAL(const vector<command> &c, int currentCommand, frames & f, process & p, int page, string offset, int & numFaults);
void allocateFreeFrame(frames & f, process & p, int page, string offset);
bool isDirty(process & p, int frameNumber);
void readInCommands(string filename, vector<command> & c);

int main(int argc, char *argv[]) 
{
	vector<command> c;
	readInCommands("Vm.dat", c);
	int totalProcesses, totalHits, totalFaults;
	cout << "*** First In First Out ***" << endl;
	processCommands(FIFO, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
	
	cout << "*** Least Recently Used ***" << endl;
	processCommands(LRU, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
	
	cout << "*** Optimal ***" << endl;
	processCommands(OPTIMAL, c, totalProcesses, totalHits, totalFaults);
	cout << "####For all " << totalProcesses << " processes, total page hit is " << totalHits << ", total page fault is " << totalFaults << "####" << endl;
}

/* Function:	processCommands
				int totalProcessesl
				int totalHits;
				int totalFaults;
 *    Usage:	processCommands(FIFO, c, totalProcesses, totalHits, totalFaults);
 * -------------------------------------------
 * Prints the execution of the commands. 
 * - algorithm: the replacementAlgorithm to use
 * - c: contains the commands to execute
 * - totalProcesses: set to equal the total number of processes
 * - totalHits: set to equal the total number of page hits
 * - totalFaults: set to equal the total number of page faults
 */
void processCommands(replacementAlgorithm algorithm, const vector<command> &c, int & totalProcesses, int &totalHits, int &totalFaults)
{
	process p;
	p.valid=false;
	frames f;
	totalProcesses = 0;
	totalHits = 0;
	totalFaults = 0;
	int numHits = 0; 
	int numFaults = 0;
	for(int i=0; i<c.size(); i++)
	{
		if(c[c.size()-1].opC != END)
		{
			cerr << "ERROR-- processCommands: The job is never ending. The last OpCode Must be opCode(4)."<< endl;
			exit(EXIT_FAILURE);
		}
		
		int code = c[i].opC;
		int param = stoi(c[i].parameter);
		string paramStr = c[i].parameter;
		if(code==NEW)
		{
			if(p.valid)
			{
				cerr << "ERROR-- processCommands: Only one job may be active; opCode(4) must precede an additional opCode(1)."<< endl;
				exit(EXIT_FAILURE);
			}
			else
			{
				cout << "New job, size " << paramStr << endl;
				p.size=param;
				p.valid=true;
				totalProcesses++;
			}
		}
		else if(code==READ || code==WRITE)
		{
			if(!p.valid)
			{
				cerr << "ERROR-- processCommands: There must be an active job to execute a Read/Write; opCode(1) MUST precede opCode(2) or opCode(3)."<< endl;
				exit(EXIT_FAILURE);
			}
			else
			{
				int page = param/PAGE_SIZE_FRAME_SIZE;
				string offset;
				int offsetLength = to_string(param%PAGE_SIZE_FRAME_SIZE).length();
				for(int j=to_string(PAGE_SIZE_FRAME_SIZE-1).length(); j>offsetLength; j--)
					offset += "0";
				offset += to_string(param%PAGE_SIZE_FRAME_SIZE);
				
				if(code==READ)
				{
					cout << "Read " << paramStr << endl;
					if(param>=p.size)
						cout << "  Access Violation" << endl;
					else 
					{
						if(p.pt.count(page))
							processPageHit(algorithm, f, p, page, offset, numHits);
						else 
						{
							if(algorithm==OPTIMAL) processPageFaultOPTIMAL(c, i, f, p, page, offset, numFaults);
							else processPageFault(f, p, page, offset, numFaults);
						}	
					}
				}
				else if(code==WRITE)
				{
					cout << "Write " << paramStr << endl;
					if(param>p.size)
						cout << "  Access Violation" << endl;
					else 
					{
						if(p.pt.count(page))
							processPageHit(algorithm, f, p, page, offset, numHits);
						else 
						{
							if(algorithm==OPTIMAL) processPageFaultOPTIMAL(c, i, f, p, page, offset, numFaults);
							else processPageFault(f, p, page, offset, numFaults);
						}
						if(p.pt.count(page))
							p.pt[page].dirty=true;
					}
				}
			}
		}
		else if(code==END)
		{
			if(!p.valid)
			{
				cerr << "ERROR-- processCommands: There is no active job to end; opCode(1) MUST precede opCode(4)."<< endl;
				exit(EXIT_FAILURE);
			}
			else 
			{
				cout << "End job" << endl;
				cout << "###Total page hit is " << numHits << "; total page fault is " << numFaults << "###" << endl;
				if(i<c.size()-1) cout << endl;
				totalHits+=numHits;
				totalFaults+=numFaults;
				numHits=0;
				numFaults=0;
				p.valid = false;
				p.pt.clear();
				f.clear();
			}
		}
	}
}

/* Function:	processPageHit
 *    Usage:	if(p.pt.count(page))
 *					processPageHit(p, page, offset, numHits);
 * -------------------------------------------
 * Process the page hit, prints the details.
 */
void processPageHit(replacementAlgorithm algorithm, frames & f, process & p, int page, string offset, int & numHits)
{
	cout << "  Page hit" << endl;
	numHits++;
	cout << "  Location " << p.pt[page].frame << offset << endl;
	
	if(algorithm==LRU)
	{
		for(int i=0; i<f.size(); i++)
		{
			if(f[i] == p.pt[page].frame)
			{
				int tempFrame = f[i];
				f.erase(f.begin()+i);
				f.push_back(tempFrame);
			}
		}
	}
}

/* Function:	processPageFault
 *    Usage:	processPageFault(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Process the page fault, prints the details
 */
void processPageFault(frames & f, process & p, int page, string offset, int & numFaults)
{
	cout << "  Page fault " << endl;
	numFaults++;
	if(f.size()<NUM_FRAMES)
	{
		allocateFreeFrame(f, p, page, offset);
	}
	else
	{
		cout << "    Page replacement" << endl;
		if(isDirty(p, f.front()))
			cout << "      Page out" << endl;
		for(pageTable::iterator it = p.pt.begin(); it!=p.pt.end(); ++it)
		{
			if(it->second.frame==f.front()) 
			{
				p.pt.erase(it->first);
				break;
			}
		}
		int tempFrame = f.front();
		f.erase(f.begin());
		f.push_back(tempFrame);
		pte e;
		e.frame=tempFrame;
		e.dirty=false;
		p.pt[page] = e;
		cout << "  Location " << p.pt[page].frame << offset << endl;
	}
}

/* Function:	processPageFaultOPTIMAL
 *    Usage:	processPageFaultOPTIMAL(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Process the page fault, prints the details
 */
void processPageFaultOPTIMAL(const vector<command> &c, int currentCommand, frames & f, process & p, int page, string offset, int & numFaults)
{
	cout << "  Page fault" << endl;
	numFaults++;
	if(f.size()<NUM_FRAMES)
	{
		allocateFreeFrame(f, p, page, offset);
	}
	else
	{
		cout << "    Page replacement" << endl;
		
		int frameToReplace=0;
		vector<bool> furthestOut(NUM_FRAMES,true);
		for(int i=currentCommand+1; i<c.size(); i++)
		{
			int count=0;
			for(int j=0; j<furthestOut.size(); j++)
			{
				if(furthestOut[j]==true)
				{
					count++;
					frameToReplace = j;
				}
			}
			if(count==1) break;
			int nextPage = stoi(c[i].parameter)/PAGE_SIZE_FRAME_SIZE;
			if(p.pt.count(nextPage))
				furthestOut[p.pt[nextPage].frame] = false; 
		}

		if(isDirty(p, f[frameToReplace]))
			cout << "      Page out" << endl;
		for(pageTable::iterator it = p.pt.begin(); it!=p.pt.end(); ++it)
		{
			if(it->second.frame==f[frameToReplace]) 
			{
				p.pt.erase(it->first);
				break;
			}
		}	
		int tempFrame = f[frameToReplace];
		f.erase(f.begin()+frameToReplace);
		f.push_back(tempFrame);
		pte e;
		e.frame=tempFrame;
		e.dirty=false;
		p.pt[page] = e;
		cout << "  Location " << p.pt[page].frame << offset << endl;
		
	}
}

/* Function:	allocateFreeFrame
 *    Usage:	allocateFreeFrame(f, p, page, offset, numFaults);
 * -------------------------------------------
 * Allocate a free fram, prints the details
 */
void allocateFreeFrame(frames & f, process & p, int page, string offset)
{
	cout <<"      Using free frame" << endl;
	int frame = f.size();
	f.push_back(frame);
	pte e;
	e.frame=frame;
	e.dirty=false;
	p.pt[page] = e;
	cout << "  Location " << frame << offset << endl;
}

/* Function:	isDirty
 *    Usage:	isDirty(p, frameNumber);
 * -------------------------------------------
 * Checks if the frame is dirty and needs to be paged out. Dirty bits are cleared.
 */
bool isDirty(process & p, int frameNumber)
{
	bool isDirty = false;
	for(pageTable::iterator it = p.pt.begin(); it!=p.pt.end(); ++it)
	{
		if(it->second.frame==frameNumber && it->second.dirty)
		{
			isDirty = true;
			it->second.dirty = false;
		}
	}
	return isDirty;
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
				newC.parameter = line.substr(firstDigit, end-firstDigit+1);
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
				newC.parameter="0";
			}


			c.push_back(newC);
		}
	}
	f.close();
}