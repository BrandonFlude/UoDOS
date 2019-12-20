// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5

#define MAXARGS 10

struct cmd
{
	int type;
};

struct execcmd 
{
	int type;
	char *argv[MAXARGS];
	char *eargv[MAXARGS];
};

struct redircmd 
{
	int type;
	struct cmd *cmd;
	char *file;
	char *efile;
	int mode;
	int fd;
};

struct pipecmd 
{
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct listcmd 
{
	int type;
	struct cmd *left;
	struct cmd *right;
};

struct backcmd 
{
	int type;
	struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
	int p[2];
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == 0)
	{
		exit();
	}
	switch (cmd->type) 
	{
		default:
			panic("runcmd");

		case EXEC:
			ecmd = (struct execcmd*)cmd;
			if (ecmd->argv[0] == 0)
			{
				exit();
			}
			exec(ecmd->argv[0], ecmd->argv);
			printf("exec %s failed\n", ecmd->argv[0]);
			break;

		case REDIR:
			rcmd = (struct redircmd*)cmd;
			close(rcmd->fd);
			if (open(rcmd->file, rcmd->mode) < 0) 
			{
				printf("open %s failed\n", rcmd->file);
				exit();
			}
			runcmd(rcmd->cmd);
			break;

		case LIST:
			lcmd = (struct listcmd*)cmd;
			if (fork1() == 0)
			{
				runcmd(lcmd->left);
			}
			wait();
			runcmd(lcmd->right);
			break;

		case PIPE:
			pcmd = (struct pipecmd*)cmd;
			if (pipe(p) < 0)
			{
				panic("Pipe");
			}
			if (fork1() == 0) 
			{
				close(1);
				dup(p[1]);
				close(p[0]);
				close(p[1]);
				runcmd(pcmd->left);
			}
			if (fork1() == 0) 
			{
				close(0);
				dup(p[0]);
				close(p[0]);
				close(p[1]);
				runcmd(pcmd->right);
			}
			close(p[0]);
			close(p[1]);
			wait();
			wait();
			break;

		case BACK:
			bcmd = (struct backcmd*)cmd;
			if (fork1() == 0)
			{
				runcmd(bcmd->cmd);
			}
			break;
	}
	exit();
}


//! LS Command Handling
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
	
	// 0 - Read Only | 1 - Hidden | 2 - System | 3 - Special Entry | 4 - Subdirectory | 5 - Archive | 6&7 Unused
	if(attributeBinary[0] == 1)
		//printf(" Read Only");
		printf(" RO");
	if(attributeBinary[1] == 1)
		//printf(" Hidden");
		printf(" H");
	if(attributeBinary[2] == 1)
		//printf(" System File");
		printf(" SF");
	if(attributeBinary[3] == 1)
		//printf(" Special Entry");
		printf(" SE");
	if(attributeBinary[4] == 1)
		//printf(" Subdirectory");
		printf(" Sub");
	if(attributeBinary[5] == 1)
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

//! End LS command handling

void changeDirectory(char * path)
{
	// Stop a plain CD command doing nothing
	if(path[0] == '\0')
	{
		// Go to root
	}
	else
	{
		chdir(path);
	}
}

void getCurrentDirectory(char * buffer, int bufferSize)
{
	buffer[0] = 0;
	getcwd(buffer, bufferSize);
}

// Move LS from LS.C/EXE to the shell
void listContents(int argc, char *argv[])
{	
	int uniqueDescriptor = 0;
	int option = 0;
	int readDirectoryResult = 1;
	int error = 0;
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
			}
			break;
		default:
			printf("Usage: ls -<opt> <dir>\n");
			break;
	}
	
	// Cater for a directory not existing since 0 is now root
	if(uniqueDescriptor == -1)
	{	
		// This means there's an error, and we don't want to continue, so set a bool to true
		error = 1;
	}
	
	if(error != 1)
	{
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
		
		// Only close if it's not the root
		if(uniqueDescriptor != 0)
		{
			// Close the directory now
			closedir(uniqueDescriptor);	
		}
	}
}

int getcmd(char *buf, int nbuf)
{
	char buffer[255];
	getCurrentDirectory(buffer, 255);
	printf("%s$ ", buffer);
	memset(buf, 0, nbuf);
	gets(buf, nbuf);
	if (buf[0] == 0) // EOF
	{
		return -1;
	}
	return 0;
}

int main(void)
{
	static char buf[100];

	// Read and run input commands.
	while (getcmd(buf, sizeof(buf)) >= 0) 
	{
		if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') 
		{
			// Chdir must be called by the parent, not the child.
			buf[strlen(buf) - 1] = 0;  // chop \n
			changeDirectory(buf + 3);
			continue;
		}
		
		// Support for LS
		if(buf[0] == 'l' && buf[1] == 's')
		{
			// Number of params specified and find arguments
			int argc = 1; // LS itself is 1
			char wordBuilder[20]; // Word Builder
			char *argv[3]; // Arguments
			int i = 0, j = 0, k = 0;
			
			// allocate memory for all possibilities (should only ever be ls -opt <dir> at most)
			argv[0] = malloc(20);
			argv[1] = malloc(20);
			argv[2] = malloc(20);
			
			// Count words typed in
			while(buf[i] != '\0')
			{				
				// If it's not a space, i.e, a char, add it to word builder
				if(buf[i] != ' ')
				{
					// Put buffer char into word builder
					wordBuilder[j] = buf[i];
					j++;
				}
				else
				{	
					// Word Builder now contains a word
					strcpy(argv[k], wordBuilder);
					k++;
					
					// Increment the word counter
					argc++;
					
					// Reset J and wordBuilder
					memset(wordBuilder, 0, sizeof(wordBuilder));
					j = 0;
				}
			
				// Increment buffer
				i++;
			}
			
			wordBuilder[strlen(wordBuilder)-1] = '\0';  // chop \n
			strcpy(argv[k], wordBuilder);
			
			// Call LS command code
			if(argc > 3)
			{
				printf("Usage: ls -<opt> <dir>\n");
			}
			else
			{
				listContents(argc, argv);	
			}
			continue;
		}
		
		if (fork1() == 0)
		{
			runcmd(parsecmd(buf));
		}
		wait();
	}
	exit();
}

void panic(char *s)
{
	printf("%s\n", s);
	exit();
}

int fork1(void)
{
	int pid;

	pid = fork();
	if (pid == -1)
	{
		panic("fork");
	}
	return pid;
}

struct cmd* execcmd(void)
{
	struct execcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = EXEC;
	return (struct cmd*)cmd;
}

struct cmd* redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
	struct redircmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = REDIR;
	cmd->cmd = subcmd;
	cmd->file = file;
	cmd->efile = efile;
	cmd->mode = mode;
	cmd->fd = fd;
	return (struct cmd*)cmd;
}

struct cmd*	pipecmd(struct cmd *left, struct cmd *right)
{
	struct pipecmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = PIPE;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

struct cmd*	listcmd(struct cmd *left, struct cmd *right)
{
	struct listcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = LIST;
	cmd->left = left;
	cmd->right = right;
	return (struct cmd*)cmd;
}

struct cmd* backcmd(struct cmd *subcmd)
{
	struct backcmd *cmd;

	cmd = malloc(sizeof(*cmd));
	memset(cmd, 0, sizeof(*cmd));
	cmd->type = BACK;
	cmd->cmd = subcmd;
	return (struct cmd*)cmd;
}

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char **ps, char *es, char **q, char **eq)
{
	char *s;
	int ret;

	s = *ps;
	while (s < es && strchr(whitespace, *s))
	{
		s++;
	}
	if (q)
	{
		*q = s;
	}
	ret = *s;
	switch (*s) 
	{
		case 0:
			break;
		case '|':
		case '(':
		case ')':
		case ';':
		case '&':
		case '<':
			s++;
			break;
		case '>':
			s++;
			if (*s == '>') 
			{
				ret = '+';
				s++;
			}
			break;
		default:
			ret = 'a';
			while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
			{
				s++;
			}
			break;
	}
	if (eq)
	{
		*eq = s;
	}
	while (s < es && strchr(whitespace, *s))
	{
		s++;
	}
	*ps = s;
	return ret;
}

int peek(char **ps, char *es, char *toks)
{
	char *s;

	s = *ps;
	while (s < es && strchr(whitespace, *s))
	{
		s++;
	}
	*ps = s;
	return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*	parsecmd(char *s)
{
	char *es;
	struct cmd *cmd;

	es = s + strlen(s);
	cmd = parseline(&s, es);
	peek(&s, es, "");
	if (s != es) 
	{
		printf("leftovers: %s\n", s);
		panic("syntax");
	}
	nulterminate(cmd);
	return cmd;
}

struct cmd* parseline(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parsepipe(ps, es);
	while (peek(ps, es, "&")) 
	{
		gettoken(ps, es, 0, 0);
		cmd = backcmd(cmd);
	}
	if (peek(ps, es, ";")) 
	{
		gettoken(ps, es, 0, 0);
		cmd = listcmd(cmd, parseline(ps, es));
	}
	return cmd;
}

struct cmd* parsepipe(char **ps, char *es)
{
	struct cmd *cmd;

	cmd = parseexec(ps, es);
	if (peek(ps, es, "|")) 
	{
		gettoken(ps, es, 0, 0);
		cmd = pipecmd(cmd, parsepipe(ps, es));
	}
	return cmd;
}

struct cmd*	parseredirs(struct cmd *cmd, char **ps, char *es)
{
	int tok;
	char *q, *eq;

	while (peek(ps, es, "<>")) 
	{
		tok = gettoken(ps, es, 0, 0);
		if (gettoken(ps, es, &q, &eq) != 'a')
		{
			panic("missing File for redirection");
		}
		switch (tok) 
		{
			case '<':
				cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
				break;
			case '>':
				cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
				break;
			case '+':  // >>
				cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
				break;
		}
	}
	return cmd;
}

struct cmd*	parseblock(char **ps, char *es)
{
	struct cmd *cmd;

	if (!peek(ps, es, "("))
	{
		panic("parseblock");
	}
	gettoken(ps, es, 0, 0);
	cmd = parseline(ps, es);
	if (!peek(ps, es, ")"))
	{
		panic("syntax - missing )");
	}
	gettoken(ps, es, 0, 0);
	cmd = parseredirs(cmd, ps, es);
	return cmd;
}

struct cmd*	parseexec(char **ps, char *es)
{
	char *q, *eq;
	int tok, argc;
	struct execcmd *cmd;
	struct cmd *ret;

	if (peek(ps, es, "("))
	{
		return parseblock(ps, es);
	}
	ret = execcmd();
	cmd = (struct execcmd*)ret;

	argc = 0;
	ret = parseredirs(ret, ps, es);
	while (!peek(ps, es, "|)&;")) 
	{
		if ((tok = gettoken(ps, es, &q, &eq)) == 0)
		{
			break;
		}
		if (tok != 'a')
		{
			panic("syntax");
		}
		cmd->argv[argc] = q;
		cmd->eargv[argc] = eq;
		argc++;
		if (argc >= MAXARGS)
		{
			panic("too many args");
		}
		ret = parseredirs(ret, ps, es);
	}
	cmd->argv[argc] = 0;
	cmd->eargv[argc] = 0;
	return ret;
}

// NUL-terminate all the counted strings.
struct cmd* nulterminate(struct cmd *cmd)
{
	int i;
	struct backcmd *bcmd;
	struct execcmd *ecmd;
	struct listcmd *lcmd;
	struct pipecmd *pcmd;
	struct redircmd *rcmd;

	if (cmd == 0)
	{
		return 0;
	}

	switch (cmd->type) 
	{
		case EXEC:
			ecmd = (struct execcmd*)cmd;
			for (i = 0; ecmd->argv[i]; i++)
			{
				*ecmd->eargv[i] = 0;
			}
			break;

		case REDIR:
			rcmd = (struct redircmd*)cmd;
			nulterminate(rcmd->cmd);
			*rcmd->efile = 0;
			break;

		case PIPE:
			pcmd = (struct pipecmd*)cmd;
			nulterminate(pcmd->left);
			nulterminate(pcmd->right);
			break;

		case LIST:
			lcmd = (struct listcmd*)cmd;
			nulterminate(lcmd->left);
			nulterminate(lcmd->right);
			break;

		case BACK:
			bcmd = (struct backcmd*)cmd;
			nulterminate(bcmd->cmd);
			break;
	}
	return cmd;
}