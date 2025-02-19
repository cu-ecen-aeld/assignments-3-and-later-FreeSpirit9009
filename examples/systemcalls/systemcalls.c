#include "systemcalls.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h> // fork
#include <sys/wait.h> // wait
#include <fcntl.h> // open
#include <assert.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    const int system_call_retval = system(cmd);
    
    if (system_call_retval == 0)
    {
      printf("%s: leaving, line=%u\n", __func__, __LINE__);
      return true;
    }
    
    printf("ERROR: system() returned error code %d (%s)\n", system_call_retval, strerror(system_call_retval));
    printf("%s: leaving, line=%u\n", __func__, __LINE__);
    return false;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    
    // fork() creates a new process by duplicating the calling process...................................
    // use return value to (a) check for errors and (b) determine who the parent is 
    bool is_parent = true;
    const pid_t pid = fork();
    if(pid == -1)
    {
      const int errno_local = errno;
      printf("ERROR: fork() returned error code %d, errno is %d (%s)\n", pid, errno_local, strerror(errno_local));
      //printf("%s: leaving, line=%u\n", __func__, __LINE__);
      return false;
    }
    else if ( pid == 0 )
    {
      is_parent = false;
    }
    
    // I'd expect to see this line twice, once as parent and once as child
    //printf("Process is_parent=%d, pid=%d\n", is_parent, pid);

    // The exec() family of functions replaces the current process image with a new process image ......
    //
    // the child process does the exec()
    if(!is_parent)
    {
      //printf("Will now do execv with %s\n", command[0]);
      const int execv_call_retval = execv(command[0], &command[0]);
      if (execv_call_retval == -1)
      {
        // execv failed, but we are still in a child process. We will end it with exit()
        const int errno_local = errno;
        printf("ERROR: execv() returned error code %d, errno is %d (%s)\n", execv_call_retval, errno_local, strerror(errno_local));
        
        // https://stackoverflow.com/questions/56090328/should-we-use-exit-or-return-in-child-process: use exit(), not return.
        exit(1);
      }
      
      // we should never get here
      assert(false);
    }
    
    // the child process never gets here
    assert(is_parent);
    
    // wait will give us the return value of the child process..........................................
    //printf("Parent process will now wait for child...\n");
    int wstatus = 0;
    const pid_t pid_wait = wait(&wstatus);
    if(pid_wait != pid || pid_wait == -1)
    {
      printf("ERROR: wait() returned unexpected pid %d (%d was expected)\n", (int)pid_wait, (int)pid);
      //printf("%s: leaving, line=%u\n", __func__, __LINE__);
      return false;
    }
    //printf("WIFEXITED=%d, WEXITSTATUS=%d\n", WIFEXITED(wstatus), WEXITSTATUS(wstatus));

    // clean up 
    va_end(args);
    
    // the expected exit status for the *child* process is 0
    const bool retval = (WIFEXITED(wstatus) == 1 && WEXITSTATUS(wstatus) == 0) ? true : false;
    //printf("%s: leaving, line=%u, retval=%u\n", __func__, __LINE__, retval);
    return retval;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        printf("%s: command[%u] = %s\n", __func__, i, command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    const int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd < 0)
    {
        return false;
    }
    // post: open was successful, we must close the file at the end
    printf("Successfully opened %s\n", outputfile);

    // TODO: fork
    bool is_parent = true;
    const pid_t pid = fork();
    if(pid == -1)
    {
      const int errno_local = errno;
      printf("ERROR: fork() returned error code %d, errno is %d (%s)\n", pid, errno_local, strerror(errno_local));
      return false;
    }
    else if ( pid == 0 )
    {
      is_parent = false;
    }
    
    // I'd expect to see this line twice, once as parent and once as child
    printf("Process is_parent=%d, pid=%d\n", is_parent, pid);
    
    
    // https://stackoverflow.com/questions/24538470/what-does-dup2-do-in-c - purpose might be to redirect stdout to the file, hem? Would be nice if that was stated somewhere.
    // https://stackoverflow.com/questions/14543443/in-c-how-do-you-redirect-stdin-stdout-stderr-to-files-when-making-an-execvp-or
    
    if(is_parent == false)
    {
        // reroute STDXXX to fd
        dup2(fd, STDIN_FILENO);
    	dup2(fd, STDOUT_FILENO);
    	dup2(fd, STDERR_FILENO);
    	close(fd);
    	
    	// do the exec
    	// TODO: maybe handover is incorrect, check out https://stackoverflow.com/questions/45318767/execve-how-can-i-initialise-char-argv-with-multiple-commands-instead-of-a-s
    	// From https://linux.die.net/man/3/execv: 
    	// The first argument, by convention, should point to the filename associated with the file being executed.
    	// The list of arguments must be terminated by a NULL pointer, and, since these are variadic functions, this pointer must be cast (char *) NULL.
    	const int execv_call_retval = execv(command[0], &command[0]);
    	//const int execv_call_retval = execv("echo home is $HOME", &command[count]);
    	if (execv_call_retval == -1)
        {
	    const int errno_local = errno;
      	    printf("ERROR: execv() returned error code %d, errno is %d (%s)\n", execv_call_retval, errno_local, strerror(errno_local));
      	    exit(1);
      	}
      	
      	// we never get here
      	assert(false);
    }

    // we only get here as parent    
    assert(is_parent);

    // wait for child process
    int wstatus = 0;
    const pid_t pid_wait = wait(&wstatus);
    if(pid_wait != pid || pid_wait == -1)
    {
      printf("ERROR: wait() returned unexpected pid %d (%d was expected)\n", (int)pid_wait, (int)pid);
      return false;
    }
 
    // TODO: close the file if open was successful! We already did that in the child process though, need to do again?
    va_end(args);
    printf("%s: leaving, line=%u\n", __func__, __LINE__);
    return true;
}

