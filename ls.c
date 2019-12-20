#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

// Take uint16_t -> convert to binary
// Take binary[0-6], [7-10], [11-15]
// Takes these integer arrays and converts them to decimal
// Prints the value out, carriage returns, labels etc. should be...
// ..done before calling this method
//15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//y  y  y  y  y  y  y  m  m  m  m  d  d  d  d  d
void toDate(uint16_t number)
{	
	int year[7];
	int month[4];
	int day[5];
	
	// Convert year to binary
	for (int i = 0, j = 0; i < 7; i++) {
		// Year iterator = passed in number AND with 1000000000000000 and shifted right 15 bits
		year[j] = (number & 0x8000) >> 15;
		// Shift left one bit to multiply by 2
		number <<= 1;
		j++;
	}
	
	// Convert month to binary
	for (int i = 7, j = 0; i < 11; i++) {
		month[j] = (number & 0x8000) >> 15;
		number <<= 1;
		j++;
	}
	
	// Convert day to binary
	for (int i = 11, j = 0; i < 16; i++) {
		day[j] = (number & 0x8000) >> 15;
		number <<= 1;
		j++;
	}
	
	// Initialise variables for converting binary to real numbers (starting at 00/00/1980)
	int printYear = 1980;
	int printMonth = 0;
	int printDay = 0;

	// Loop through the year array, calculating the number as we go	
	for(int i = 6, j = 1; i >= 0; i--)
	{
		if(year[i] == 1)
		{
			printYear += j;
		}
		// Shift left one bit to multiply by 2
		j <<= 1;
	}
	
	
	// Loop through the month array, calculating the number as we go
	for(int i = 3, j = 1; i >= 0; i--)
	{
		if(month[i] == 1)
		{
			printMonth += j;
		}
		j <<= 1;
	}
	
	// Loop through the day array, calculating the number as we go
	for(int i = 4, j = 1; i >= 0; i--)
	{
		if(day[i] == 1)
		{
			printDay += j;
		}
		j <<= 1;
	}
	
	// Print final String
	printf("%d/%d/%d", printDay, printMonth, printYear);
}

// Takes a uint16_t number and converts it to a time in the FAT12 format
//15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
//h  h  h  h  h  m  m  m  m  m  m  x  x  x  x  x
void toTime(uint16_t number)
{	
	int hour[5];
	int minute[6];
	
	// Convert hour to binary
	for (int i = 0, j = 0; i < 5; i++) {
		hour[j] = (number & 0x8000) >> 15;
		number <<= 1;
		j++;
	}

	// Convert minute to binary
	for (int i = 7, j = 0; i < 11; i++) {
		minute[j] = (number & 0x8000) >> 15;
		number <<= 1;
		j++;
	}
	
	// Initialise variables for converting binary to real numbers
	int printHour = 0;
	int printMin = 0;
	
	// Loop through the hour array, calculating the number as we go
	for(int i = 4, j = 1; i >= 0; i--)
	{
		if(hour[i] == 1)
		{
			printHour += j;
		}
		j <<= 1;
	}
	
	for(int i = 5, j = 1; i >= 0; i--)
	{
		if(minute[i] == 1)
		{
			printMin += j;
		}
		j <<= 1;
	}

	// Print final String (checking for 0-9 issue)
	if(printMin >= 0 && printMin < 10)
		printf("%d:0%d", printHour, printMin);	
	else
		printf("%d:%d", printHour, printMin);
	
}

// Method to take in the attributes
// Converts the number to binary
// Converts the binary to a sentence and prints it out
void convertAttributes(uint8_t attributes)
{
	// Only care about first 6 bits, but do it all for completeness
	int attributeBinary[8];
	
	// 8 bit integer, shift left 7 times and AND with 10000000
	for (int i = 0, j = 0; i < 7; i++) {
		attributeBinary[j] = (attributes & 0x80) >> 7;
		attributes <<= 1;
		j++;
	}	
	
	// 0x01 - Read Only | 0x02 - Hidden | 0x04 - System | 0x08 - Special Entry | 0x10 - Subdirectory | 0x20 - Archive | 6&7 Unused
	if(attributeBinary[7] == 1)
		//printf(" Read Only");
		printf(" RO");
	if(attributeBinary[6] == 1)
		//printf(" Hidden");
		printf(" H");
	if(attributeBinary[5] == 1)
		//printf(" System File");
		printf(" SF");
	if(attributeBinary[4] == 1)
		//printf(" Special Entry");
		printf(" SE");
	if(attributeBinary[3] == 1)
		//printf(" Subdirectory");
		printf(" Sub");
	if(attributeBinary[2] == 1)
		//printf(" Archived");
		printf(" A");
}

void printListing(struct _DirectoryEntry dirEntry, int option)
{				
	if(option == 1)
	{
		// The size of the file.
		printf("%d ", dirEntry.FileSize);

		// The creation date and time for the file.	
		printf(" Cr. ");
		toDate(dirEntry.DateCreated);
		printf(" @ ");
		toTime(dirEntry.TimeCreated);		
		
		// The date and time the file was last modified.
		printf(" Mod. ");
		toDate(dirEntry.LastModDate);
		printf(" @ ");
		toTime(dirEntry.LastModTime);		
		
		// The attributes of the file.
		printf(" Attr:");
		convertAttributes(dirEntry.Attrib);
		printf(" ");
	}

	// Loop through 8 spaces for the file name
	for(int i = 0; i < 8; i++)
		printf("%c", dirEntry.Filename[i]);

	// Loop through the 3 spaces for the extension
	for(int i = 0; i < 3; i++)
		printf("%c", dirEntry.Ext[i]);
	
	if(option == 2)
	{
		// Files with a size of 0 seem to be directories
		if(dirEntry.FileSize == 0)
		{
			printf("Directory - recursive");
		}
	}
	
	// Clear a line
	printf("\n");
}

int main(int argc, char *argv[])
{	
	int uniqueDescriptor = 0;
	int option = 0;
	int readDirectoryResult = 1;
	struct _DirectoryEntry dirEntry;
	
	switch(argc) 
	{
		case 1:
			// No directory or option, just ls
			uniqueDescriptor = opendir("");
			break;
		case 2:
			// Could be a -l/-r or a <dir>
			if(strcmp(argv[1], "-l") == 0)
			{
				option = 1;
				uniqueDescriptor = opendir("");
			}
			else if(strcmp(argv[1], "-r") == 0)
			{
				option = 2;
				uniqueDescriptor = opendir("");
			}
			else
			{
				uniqueDescriptor = opendir(argv[1]);
			}
			break;
		case 3:
			if(strcmp(argv[1], "-l") == 0)
			{
				option = 1;
				uniqueDescriptor = opendir(argv[2]);	
			}
			else if(strcmp(argv[1], "-r") == 0)
			{
				option = 2;
				uniqueDescriptor = opendir(argv[2]);
			}
			else if(strcmp(argv[2], "-l") == 0)
			{
				option = 1;
				uniqueDescriptor = opendir(argv[1]);	
			}
			else if(strcmp(argv[2], "-r") == 0)
			{
				option = 2;
				uniqueDescriptor = opendir(argv[1]);
			}
			else 
			{
				// Prompt to use correct layout
				printf("Usage: ls -<opt> <dir>\n");	
				exit();
			}
			break;
		default:
			printf("Usage: ls -<opt> <dir>\n");
			exit();
			break;
	}

	// Cater for a directory not existing since 0 is now root
	if(uniqueDescriptor == -1)
	{
		exit();
	}
	
	while(readDirectoryResult != -1)
	{
		// Fetch the current chunk of file
		readDirectoryResult = readdir(uniqueDescriptor, &dirEntry);
		
		// Check before printing so I don't print a blank line
		if(readDirectoryResult != -1)
		{			
			// Send to print listing
			printListing(dirEntry, option);
		}
	}	
	
	// Close the directory now
	closedir(uniqueDescriptor);
	
	// Leave LS
	exit();
}
