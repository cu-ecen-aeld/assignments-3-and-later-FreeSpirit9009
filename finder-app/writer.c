// writer.c
//
// requirements
// Write a C application “writer” (finder-app/writer.c) which can be used as an alternative to the “writer.sh” test script created in assignment1 and using File IO as described in LSP chapter 2.  See the Assignment 1 requirements for the writer.sh test script and these additional instructions:
// One difference from the write.sh instructions in Assignment 1: You do not need to make your "writer" utility create directories which do not exist. You can assume the directory is created by the caller.
// Setup syslog logging for your utility using the LOG_USER facility.
// Use the syslog capability to write a message “Writing <string> to <file>” where <string> is the text string written to file (second argument) and <file> is the file created by the script.  This should be written with LOG_DEBUG level.
// Use the syslog capability to log any unexpected errors with LOG_ERR level.

#include <syslog.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  // open syslog
  openlog("writer", LOG_PID, LOG_USER);

  // check preconditions
  if (argc < 3)
  {
    printf("Not enough parameters supplied, exiting.\n");
    syslog(LOG_ERR, "Not enough parameters supplied, exiting.\n");
    return 1;
  }
  
  // name the input parameters
  char* pWriteFile = argv[1];
  char* pWriteStr  = argv[2];
  printf("pWriteFile=%s, pWriteStr=%s\n", pWriteFile, pWriteStr);
  syslog(LOG_INFO, "Writing %s to %s\n", pWriteStr, pWriteFile );

  // open file for writing, and write
  FILE* fptr;
  fptr = fopen(pWriteFile,"w");
  
  if(fptr == NULL)
  {
    printf("Could not open file <%s> for writing, exiting.\n", pWriteFile);
    syslog(LOG_ERR, "Could not open file <%s> for writing, exiting.\n", pWriteFile);
    return 1;
  }
  
  fprintf(fptr,"%s",pWriteStr);
 
  // clean up
  fclose(fptr);
  closelog();
  return 0;
}
