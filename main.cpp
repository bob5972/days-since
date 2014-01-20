#include <stdio.h>

#include <boost/date_time.hpp>

using namespace boost::gregorian;

#include "MBString.h"
#include "Parser.h"
#include "MBVector.h"
#include "Dumper.h"

#define DB_FILE "/home/bob5972/bin/data/days-since.dat"
#define DB_FILE_BAK DB_FILE ".bak"

typedef struct Entry {
	int index;
	MBString name;
	date startDate;
	int numTimes;
	int intervalDays;
	date lastTime;
} Entry;

typedef struct GlobalData {
	FILE *dbFile;
	MBVector<Entry> entries;
} GlobalData;

GlobalData mainData;


char readDBChar(void *clientData)
{
	return getc(mainData.dbFile);
}

bool isDBEOF(void *clientData)
{
	bool eof;
	
	eof = feof(mainData.dbFile);	
	return eof;
}

void writeDBChar(void *clientData, const char *c, int size)
{
	fwrite(c, 1, size, mainData.dbFile);
}

void flushDB(void *clientData)
{
	fflush(mainData.dbFile);
}

Entry readEntry(Parser &p)
{
	MBString tmp;
	Entry oup;
	
	
	oup.name = p.readLine();
	
	tmp = p.readLine();
	oup.startDate = from_undelimited_string(tmp.cstr());
	
	oup.numTimes = p.readInt();
	if (oup.numTimes < 0) {
		oup.numTimes = 0;
	}
	
	oup.intervalDays = p.readInt();
	p.eatWhitespace();

	tmp = p.readLine();	
	oup.lastTime = from_undelimited_string(tmp.cstr());
	
	return oup;
}

void dumpEntry(Dumper &d, const Entry &e)
{
	d.writeLine(e.name);
	
	d.writeLine(to_iso_string(e.startDate).c_str());
	
	d.writeInt(e.numTimes);
	d.endLine();
	d.writeInt(e.intervalDays);
	d.endLine();
	
	d.writeLine(to_iso_string(e.lastTime).c_str());
}

void printEntry(const Entry &e)
{
	int daysSince;
	double average;
	date now(day_clock::local_day());
	date_duration dd;
	int numTimes;
	
	dd = now - e.lastTime;
	
	daysSince = dd.days();
	
	ASSERT(e.numTimes >= 0);
	numTimes = e.numTimes;
	if (numTimes == 0) {
		numTimes = 1;
	}
	
	dd = (e.lastTime - e.startDate);
	average = dd.days();
	average = average / numTimes;
	
	/*
	 * Count the current period in the average only if
	 * we've passed the average mark.
	 * (ie Expect the previous trend to continue.)
	 *
	 * If we're at zero times, we've already counted
	 * the current period.	 
	 */
	if (average < daysSince) {
		dd = (now - e.startDate);
		average = dd.days();
		average = average / (e.numTimes + 1);
	}
	
	printf("[%d] %20s: %10d %10.1f %10d\n",
	       e.index, e.name.cstr(), daysSince, average, numTimes);
}

int findEntry(const MBString &name)
{
	int index = -1;
	int numEntries;
	
	numEntries = mainData.entries.size();
	
	for (int x = 0; x < numEntries; x++) {
		if (mainData.entries[x].name == name) {
			index = x;
			break;
		}
	}
			
	if (index == -1) {
		index = atoi(name.cstr());
		if (index < 0 || index >= numEntries) {
			PANIC("Unable to find \"%s\"\n", name.cstr());
		}
	}
	
	return index;
}


int main(int argc, char *argv[])
{
	int numEntries;
	bool modified = FALSE;	
	
	CharReaderInterface dbFileReader = {
		NULL,
		readDBChar,
		isDBEOF,
	};
	
	Parser p(&dbFileReader);
	
	mainData.dbFile = fopen(DB_FILE, "r");
	
	if (mainData.dbFile == NULL) {
		PANIC("Unable to open db file.\n");
	}
	
	numEntries = p.readInt();
	p.eatWhitespace();
	mainData.entries.resize(numEntries);
	
	for (int x = 0; x < numEntries; x++) {
		mainData.entries[x] = readEntry(p);
		mainData.entries[x].index = x;
	}
	
	if (argc > 1) {
		MBString cmd = argv[1];
		
		if (cmd == "-h") {
		    printf("Usage: days-since add name\n");
		    printf("       days-since reset name/number\n");
		    printf("       days-since del name/number\n");
			exit(1);
		} else if (cmd == "add") {
			if (argc < 3) {
				PANIC("Bad arguments to add\n");
			}
			
			MBString name = argv[2];
			Entry e;
			
			e.name = name;
			e.startDate = day_clock::local_day();
			e.numTimes = 1;
			e.intervalDays = -1;
			e.lastTime = e.startDate;
			e.index = mainData.entries.size();
			
			mainData.entries.push(e);			
			modified = TRUE;
			
			printf("Add [%d] %s\n", e.index, name.cstr());
		} else if (cmd == "reset") {
			int index = -1;
			if (argc < 3) {
				PANIC("Bad arguments to reset\n");
			}
			
			MBString name = argv[2];
			
			index = findEntry(name);
			name = mainData.entries[index].name;
			
			mainData.entries[index].numTimes++;
			mainData.entries[index].lastTime = day_clock::local_day();
			modified = TRUE;
			
			printf("Reset [%d] %s\n", index, name.cstr());
		} else if (cmd == "del") {
			int index;
			
			if (argc < 3) {
				PANIC("Bad arguments to del\n");
			}
			
			MBString name = argv[2];
			index = findEntry(name);
			name = mainData.entries[index].name;
			
			for (int x = index; x < mainData.entries.size() - 1; x++) {
				mainData.entries[x] = mainData.entries[x+1];
				mainData.entries[x].index = x;
			}
			mainData.entries.shrink();
			modified = TRUE;
			
			printf("Removed [%d] %s\n", index, name.cstr());
		} else if (cmd == "status") {
			/*
			 * Default action: Display the state.
			 */
		} else {
			PANIC("Unknown command: %s\n", cmd.cstr());
		}			
	}		
	
	numEntries = mainData.entries.size();
	
	printf("    %20s: %10s %10s %10s\n", "Name", "Days", "Avg", "Count");
	for (int x = 0; x < numEntries; x++) {
		printEntry(mainData.entries[x]);
	}
	
	fclose(mainData.dbFile);
	mainData.dbFile = NULL;
	
	if (modified) {
		CharWriterInterface dbFileWriter = {
			NULL,
			writeDBChar,
			flushDB,
		};
		
		system("cp " DB_FILE " " DB_FILE_BAK);
		
		mainData.dbFile = fopen(DB_FILE, "w");
		
		Dumper d(&dbFileWriter);
		
		numEntries = mainData.entries.size();
		d.writeInt(numEntries);
		d.endLine();
		
		for (int x = 0; x < numEntries; x++) {
			dumpEntry(d, mainData.entries[x]);
		}
		
		d.flush();
		fclose(mainData.dbFile);
		mainData.dbFile = NULL;
	}
	
	//printf("Done.\n");
	return 0;
}
