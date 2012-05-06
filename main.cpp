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
	
	dd = now - e.lastTime;
	
	daysSince = dd.days();
	
	dd = (now - e.startDate);
	average = dd.days();
	average = average / e.numTimes;
	
	printf("%20s: %10d %10.1f %10d\n", e.name.cstr(), daysSince, average, e.intervalDays);
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
	
	Warning("Reading numEntries...\n");
	numEntries = p.readInt();
	Warning("numEntries = %d\n", numEntries);
	p.eatWhitespace();
	mainData.entries.resize(numEntries);
	
	for (int x = 0; x < numEntries; x++) {
		mainData.entries[x] = readEntry(p);
	}
	
	printf("%20s: %10s %10s %10s\n", "Name", "Days", "Avg", "Target");
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
		
		d.writeInt(numEntries);
		d.endLine();
		
		for (int x = 0; x < numEntries; x++) {
			dumpEntry(d, mainData.entries[x]);
		}
		
		d.flush();
		fclose(mainData.dbFile);
		mainData.dbFile = NULL;
	}
	
	printf("Done.\n");
	return 0;
}
