// File-system system calls.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

// Retrieve an argument to the system call that is an FD
//
// n   = The parameter number (0 = The first parameter, 1 = second parameter, etc).
// pfd = Returns the fd number (set to 0 if not required)
// pf  = Returns the pointer to the File structure (set to 0 if not required).

static int argfd(int n, int *pfd, File **pf)
{
	int fd;
	File *f;

	if (argint(n, &fd) < 0)
	{
		return -1;
	}
	if (fd < 0 || fd >= NOFILE || (f = myProcess()->OpenFile[fd]) == 0)
	{
		return -1;
	}
	if (pfd)
	{
		*pfd = fd;
	}
	if (pf)
	{
		*pf = f;
	}
	return 0;
}

// Allocate a file descriptor for the given file and
// store it in the process table for the current process.

static int fdalloc(File *f)
{
	int fd;
	Process *curproc = myProcess();

	for (fd = 0; fd < NOFILE; fd++) 
	{
		if (curproc->OpenFile[fd] == 0) 
		{
			curproc->OpenFile[fd] = f;
			return fd;
		}
	}
	return -1;
}

// Duplicate a file descriptor

int sys_dup(void)
{
	File *f;
	int fd;

	if (argfd(0, 0, &f) < 0)
	{
		return -1;
	}
	if ((fd = fdalloc(f)) < 0)
	{
		return -1;
	}
	fileDup(f);
	return fd;
}

// Read from file.

int sys_read(void)
{
	File *f;
	int n;
	char *p;

	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
	{
		return -1;
	}
	return fileRead(f, p, n);
}

// Write to file.  Only implemented for pipes and console at present. 

int sys_write(void)
{
	File *f;
	int n;
	char *p;
	if (argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
	{
		return -1;
	}
	return fileWrite(f, p, n);
}

// Close file.

int sys_close(void)
{
	int fd;
	File *f;

	if (argfd(0, &fd, &f) < 0)
	{
		return -1;
	}
	myProcess()->OpenFile[fd] = 0;
	fileClose(f);
	return 0;
}

// Return file stats.  Not fully implemented.

int sys_fstat(void)
{
	File *f;
	Stat *st;

	if (argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
	{
		return -1;
	}
	return fileStat(f, st);
}

// Open a file. 

int sys_open(void)
{
	char *path;
	int fd, omode;
	File * f;
	
	if (argstr(0, &path) < 0 || argint(1, &omode) < 0)
	{
		return -1;
	}
	
	Process *curproc = myProcess();
	// At the moment, only file reading is supported
	f = fsFat12Open(curproc->Cwd, path, 0);
	if (f == 0)
	{
		return -1;
	}
	fd = fdalloc(f);
	if (fd < 0)
	{
		fileClose(f);
		return -1;
	}
	f->Readable = !(omode & O_WRONLY);
	f->Writable = (omode & O_WRONLY) || (omode & O_RDWR);
	return fd;
}

// Execute a program

int sys_exec(void)
{
	char *path, *argv[MAXARG];
	int i;
	uint32_t uargv, uarg;
	char adjustedPath[200];
	
	if (argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0) 
	{
		return -1;
	}
	memset(argv, 0, sizeof(argv));
	for (i = 0;; i++) 
	{
		if (i >= NELEM(argv))
		{
			return -1;
		}
		if (fetchint(uargv + 4 * i, (int*)&uarg) < 0)
		{
			return -1;
		}
		if (uarg == 0) 
		{
			argv[i] = 0;
			break;
		}
		if (fetchstr(uarg, &argv[i]) < 0)
		{
			return -1;
		}
	}
	int pathLen = strlen(path);
	safestrcpy(adjustedPath, path, 200);
	if (path[pathLen - 4] != '.')
	{
		safestrcpy(adjustedPath + pathLen, ".exe", 200 - pathLen);
		adjustedPath[pathLen + 4] = 0;
	}
	return exec(adjustedPath, argv);
}

int sys_pipe(void)
{
	int *fd;
	File *rf, *wf;
	int fd0, fd1;

	if (argptr(0, (void*)&fd, 2 * sizeof(fd[0])) < 0)
	{
		return -1;
	}
	if (pipealloc(&rf, &wf) < 0)
	{
		return -1;
	}
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) 
	{
		if (fd0 >= 0)
		{
			myProcess()->OpenFile[fd0] = 0;
		}
		fileClose(rf);
		fileClose(wf);
		return -1;
	}
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}
// Assessment Additions
// Currently supports any <dir> and using ../ or .. to go back but not ../<dir>
int sys_chdir(void)
{
	char *directory;
	char goBack[] = "../";
	char goBackAlt[] = "..";
	int counter = 0;
	
	// Check values on the stack exist
	if (argstr(0, &directory) < 0)
	{
		return -1;
	}
		
	// Get current process
	Process *curproc = myProcess();
		
	int length = strlen(directory);
	int currentLength = strlen(curproc->Cwd);
	
	// See if the user has opted to go back a level (../ or ..)
	if((strcmp(directory, goBack) != 0) && (strcmp(directory, goBackAlt) != 0))
	{
		// Change working directory	
		while(counter <= length)	
		{
			curproc->Cwd[currentLength + counter] = directory[counter];
			counter++;
		}
				
		// Check if directory ends in a slash or backslash, if not append one
		if(directory[counter - 2] != '/' && directory[counter - 2] != '\\')	
		{
			curproc->Cwd[currentLength + counter - 1] = '/';	
		}
	}
	else
	{		
		// Strip the last /.../ section
		
		// Set i to be what we need and loop through removing the last directory
		int i = currentLength - 2;
		
		for(i = i; i > 0; i--)
		{
			if(curproc->Cwd[i] != '/' && curproc->Cwd[i] != '\\')
			{
				// Set to end terminator to remove it
				curproc->Cwd[i] = '\0';
			}
			else 
			{
				break;
			}
		}
	}
		
	return 0;
}

int sys_getcwd(void)
{
	// buffer (currentDirectory buffer)
	char *buffer;		
	int bufferSize;
	
	// Check values on the stack exist
	if (argstr(0, &buffer) < 0 || argint(1, &bufferSize) < 0)
	{
		return -1;
	}
	
	// Get current process
	Process *curproc = myProcess();
	
	// Initialise empty counter
	int counter = 0;
	
	// Loop through until we have every character in the buffer
	while(counter < bufferSize)
	{
		buffer[counter] = curproc->Cwd[counter];
		counter++;
	}
	
	return 0;
}

// Stage 2
int sys_opendir(void)
{
	// Read in passed directory
	char *directory;
	int uniqueDescriptor;
	File *f;
	
	// Check values on the stack exist
	if (argstr(0, &directory) < 0)
	{
		return 0;
	}
	
	// Get current process
	Process *curproc = myProcess();
	
	// Prepare a copy we will modify if directory isn't passed in
	char cwdCopy[255];
	safestrcpy(cwdCopy, curproc->Cwd, MAXCWDSIZE);
	int cwdLength = strlen(cwdCopy);
	
	// Determine whether reading root or subdirectory
	if((strcmp(directory, "/") == 0) || (strcmp(cwdCopy, "/") == 0 && strcmp(directory, "") == 0))
	{	
		// Return 0 for the root's directoryDescriptor
		return 0;
	}
	
	// If directory is null, remove the last / since fsFat12Open will add one
	if(directory[0] == '\0')
	{		
		// Change the last / to a terminator
		cwdCopy[cwdLength - 1] = '\0';
	}
	
	// Open directory	
	f = fsFat12Open(cwdCopy, directory, 1);			
	if (f == 0)
	{
		// Just exit and show an error message so we don't get confused with the root
		cprintf("Failed to open directory\n");
		// Errors without this return
		return -1;
	} 
	else
	{
		// Set unique descriptor
		uniqueDescriptor = fdalloc(f);
	}	
		
	// Return the unique Descriptor if it opened up fine
	return uniqueDescriptor; 
}
 
int sys_readdir(void)
{
	int directoryDescriptor;
    DirectoryEntry *dirEntry;
	
	if(argint(0, &directoryDescriptor) < 0 || argptr(1, (void*)&dirEntry, sizeof(*dirEntry)) < 0)
	{
		return -1;
	}
	
	if(directoryDescriptor == 0)
	{
		// Open root directory
		fsFat12ReadRootDirectory(dirEntry);
		
		if(dirEntry->Filename[0] != '\0')
		{
			return 0;
		}
		return -1;
	}
	else
	{
		// Fetch opened file and pass across results
		Process *curproc = myProcess();
		File *directory = curproc->OpenFile[directoryDescriptor];		
			
		// Start pulling contents from the directory and sending it back to LS	
		unsigned char buffer[256];
		
		// Read the next chunk into the buffer (next file)
		fsFat12Read(directory, buffer, 32);
		
		// Copy this to the passed in empty struct
		DirectoryEntry *foundEntry = (DirectoryEntry *)buffer;
		memmove((char *)dirEntry, (char *)foundEntry, sizeof(DirectoryEntry));
	
		// Return to say file is completely read
		if(buffer[0] != '\0')
		{
			return 0;
		}
		return -1;
	}	
}

int sys_closedir(void)
{
	int directoryDescriptor;

	if(argint(0, &directoryDescriptor) < 0)
	{
		return -1;
	}
	
	// Get current process 
	Process *curproc = myProcess();	
	
	// Fetch the opened directory
	File * directory = curproc->OpenFile[directoryDescriptor];	
	
	// Close it
	fileClose(directory);
	curproc->OpenFile[directoryDescriptor] = 0;	
	return 0;
}
