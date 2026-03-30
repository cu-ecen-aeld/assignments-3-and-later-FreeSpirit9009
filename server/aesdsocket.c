/**
 * requirements from course assignment:
 * - Opens a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
 * - Listens for and accepts a connection
 * - Logs message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected client. 
 * - Receives data over the connection and appends to file /var/tmp/aesdsocketdata, creating this file if it doesn’t exist.
 * - Your implementation should use a newline to separate data packets received.  In other words a packet is considered complete when a newline character is found in the input receive stream, and each newline should result in an append to the /var/tmp/aesdsocketdata file.
 * - You may assume the data stream does not include null characters (therefore can be processed using string handling functions).
 * - You may assume the length of the packet will be shorter than the available heap size.  In other words, as long as you handle malloc() associated failures with error messages you may discard associated over-length packets.
 * - f. Returns the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
 * - You may assume the total size of all packets sent (and therefore size of /var/tmp/aesdsocketdata) will be less than the size of the root filesystem, however you may not assume this total size of all packets sent will be less than the size of the available RAM for the process heap.
 * - g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
 * - h. Restarts accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received (see below).
 * - i. Gracefully exits when SIGINT or SIGTERM is received, completing any open connection operations, closing any open sockets, and deleting the file /var/tmp/aesdsocketdata.
 * - Logs message to the syslog “Caught signal, exiting” when SIGINT or SIGTERM is received.
 * 
 * references:
 * - https://beej.us/guide/bgnet/html/split-wide/client-server-background.html - includes fork
 * 
 * test commands:
 * - nc 127.0.0.1 9000
 * - cat /var/tmp/aesdsocketdata
 * - test ctrl c in the session running the program, (a) before connecting and (b) after connection, and (c) while connected
 * - tail -f /var/log/syslog
 * - ../assignment-autotest/test/assignment5/sockettest.sh 
 * 
 * test cases:
 * - termination
 *   - with no client ever connected
 *   - with client connected but sent no data
 *   - with client connected and sent data, in this case also check that the file was deleted
 * - connection, disconnection and reconnection of client (note, it truncates the file, which might not be desireable)
 * - send some data and check reply and also the file contents
 * - check syslog for all of the above
 * - check the file after program closes gracefully, it should be gone
 * 
 * known issues or limitations:
 * - pressing ctrl c right after starting the program will result in an error message: Error deleting file: No such file or directory
 * - when client disconnects and reconnects, temporary file is deleted, not sure whether that's as intended
 * - option "-d" not yet implemented; full-test uses this
 * - socket-test.sh currently fails, likely because file is deleted when client disconnects
 * - socket-test.sh currently fails, likely because file is deleted when client disconnects
 *
 * references:
 * - https://stackoverflow.com/questions/16990746/isnt-recv-in-c-socket-programming-blocking
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // close
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <syslog.h>
#include <assert.h>


//--------------------------------------------------------------------------------------------------
// signal handler
//--------------------------------------------------------------------------------------------------
static sig_atomic_t isApplicationOrderedToStop = false;

/**
 * Signal handler for graceful shutdown
 */
void writeToSyslog(char* message);
void signalHandler(int signum) {
  fprintf(stderr, "\nSignal %d received, initiating graceful shutdown...\n", signum);
  writeToSyslog("Caught signal, exiting");
  
  if (signum == SIGINT || signum == SIGTERM) {
    atomic_store(&isApplicationOrderedToStop, true);   
  }
}

//--------------------------------------------------------------------------------------------------
// helper functions
//--------------------------------------------------------------------------------------------------
/**
 * @brief get sockaddr, IPv4 or IPv6
 */
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/**
 * @brief write a message to syslog
 */
void writeToSyslog(char* message) {
	openlog("aesdsocket", LOG_PID, LOG_USER);
	syslog(LOG_INFO, "%s", message);  // Use format specifier
	closelog();   
}


//--------------------------------------------------------------------------------------------------
// main
//--------------------------------------------------------------------------------------------------
int main() {
	
	int new_fd; // TODO: we may need one per connection?
	int sockfd;
	struct addrinfo *servinfo;  // will point to the results

	// TODO: handle arguments
	
	// setup signal handlers for graceful shutdown
	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0; // Do NOT include SA_RESTART
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	
	// prepare socket communication
	int status;
	struct addrinfo hints;

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;       // use AF_UNSPEC if don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0) {
		fprintf(stderr, "gai error: %s\n", gai_strerror(status));
		exit(1);
	}
	
	// servinfo now points to a linked list of 1 or more
	// struct addrinfos
	//
	// ... do everything until you don't need servinfo anymore ....
	printf("IP addresses:\n\n");
	struct addrinfo* p;
	char ipstr[INET6_ADDRSTRLEN];

	for(p = servinfo;p != NULL; p = p->ai_next) {
		void *addr;
		char *ipver;
		struct sockaddr_in *ipv4;
		struct sockaddr_in6 *ipv6;

		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		} else { // IPv6
			ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}

		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf("  %s: %s\n", ipver, ipstr);
	}
	
	// create the socket
	sockfd  = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (sockfd  < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno));
		exit(1);		
	}
	
	// precent "socket is in use" issue on binding
	int opt = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));   
	int res = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
	if (res  < 0) {
		fprintf(stderr, "bind error: %s\n", strerror(errno));
		exit(1);
	}
	
	// listen
	res = listen(sockfd, 1); 
	if (res  < 0) {
		if (errno == EINTR && isApplicationOrderedToStop) {
			printf("exit by signal handler with 0 (listen)\n");
			exit(0);
		} else {
			fprintf(stderr, "listen error: %s\n", strerror(errno));
			exit(1);		
		}
	}
	
	// truncate the file, note: when server disconnects and reconnects, file will be truncated. If this isn't
	// what they want, move this outside the outer loop
	FILE *file = fopen("/var/tmp/aesdsocketdata", "w");
	fclose(file);
	
	// outer loop
	while(!isApplicationOrderedToStop) {

		// accept incoming connection
		struct sockaddr_storage their_addr;
		socklen_t addr_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
		printf("accept returned, addr_size=%u, new_fd=%u\n", addr_size, new_fd);
		if (new_fd == -1) {
			if (errno == EINTR && isApplicationOrderedToStop) {
				printf("exit by signal handler with 0 (accept)\n");
				break;
			} else {
				fprintf(stderr, "accept error: %s\n", strerror(errno));
				exit(1);
			}
		}
		
		// handle new incoming connection
		char s[INET6_ADDRSTRLEN];
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);
		
		// welcome message is not welcome to sockettest.sh
#if 0
		char *msg = "Welcome to aesdsocket!";
		int len, bytes_sent;
		len = strlen(msg);
		bytes_sent = send(new_fd/*sockfd*/, msg, len, 0);
		if(bytes_sent < 0) {
			fprintf(stderr, "send error: %s\n", strerror(errno));
			exit(1);		
		}
		printf("bytes_sent=%u\n",bytes_sent);
#endif
		
		// d. Logs message to the syslog “Accepted connection from xxx” where XXX is the IP address of the connected client. 
		{
			char message[1000];
			(void)memset(message, 0, sizeof(message));   
			(void)snprintf(message, sizeof(message), "Accepted connection from %s", s);
			writeToSyslog(message);
		}

#if 0
		// truncate the file, note: when server disconnects and reconnects, file will be truncated. If this isn't
		// what they want, move this outside the outer loop
		FILE *file = fopen("/var/tmp/aesdsocketdata", "w");
		fclose(file);
#endif

		// inner loop
		while(!isApplicationOrderedToStop) {

			// use buffered access (getline) to prevent having to tinker with individual bytes and chunks of messages
			FILE *fp = fdopen(new_fd, "r");
			if (!fp) {
				perror("fdopen");
				printf("line %u: fdopen error\n", __LINE__);
				break;
			}

			char *line = NULL;
			size_t cap = 0;

			// getline() allocates or grows the buffer as needed
			ssize_t len = getline(&line, &cap, fp);

			// handle all possible errors and situations
			if (len >= 0) {
				// SUCCESS: full line read, we will process the line after this if block
				printf("line %u: read %lu bytes\n", __LINE__, len);
			} else {

				// len == -1: error or EOF.
				if (feof(fp)) {
					// CLEAN EOF: peer closed connection, this occurs when I close netcat with Ctrl-C.
					// --> Trigger graceful shutdown of this connection
					// --> Cleanup resources and return
					printf("line %u: clean EOF\n", __LINE__);
					free(line);
					
					// g. Logs message to the syslog “Closed connection from XXX” where XXX is the IP address of the connected client.
					char message[1000];
					(void)memset(message, 0, sizeof(message));   
					(void)snprintf(message, sizeof(message), "Closed connection from %s", s);
					writeToSyslog(message);
					
					// get out of the loop, this will delete the temporary file but otherwise works fine
					break;
				}

				if (errno == EINTR) {
					// INTERRUPTED BY SIGNAL
					// --> Check if this was a shutdown-triggering signal
					// --> If yes: perform graceful shutdown
					// --> Otherwise: retry reading					
					if (isApplicationOrderedToStop) {
						printf("exit by signal handler with 0 (getline)\n");
						break;
					}					
					
					printf("line %u: errno=%u\n", __LINE__, errno);
					continue;   // retry getline()
				}

				// If we reach here, it's a *real error* (connection reset, I/O error, etc.)
				// Typical errno values:
				//   ECONNRESET  (peer reset connection)
				//   ETIMEDOUT   (timeout if configured)
				//   EIO         (generic I/O error)
				//   ENOMEM      (out of memory; very rare but defined)
				if (errno == ECONNRESET) {
					// CONNECTION RESET BY PEER
					// --> Log reset event
					// --> Trigger graceful cleanup of this connection
					printf("line %u: reset by peer\n", __LINE__);
					free(line);
					continue;
				}

				if (errno == ENOMEM) {
					// OUT OF MEMORY
					// --> Handle memory exhaustion path
					// --> You must abort or reclaim memory; safe exit is preferred
					printf("line %u: errno=%u, out of memory, will close program immediately, no graceful shutdown\n", __LINE__, errno);
					free(line);
					exit(1);
				}

				// ANY OTHER ERROR (EIO, ETIMEDOUT, unknown)
				// --> Perform error logging
				// --> Trigger graceful shutdown
				printf("line %u: errno=%u, len=%ld, unknown error\n", __LINE__, errno, len);
				free(line);
				break;
			}

            // at this point we have a valid line with a valid length; in error cases we either got out of the loop,
            // terminated the program, or restarted the loop, so we would not get here
            assert(len > 0);
            assert(line != 0);
			printf("line %u: line is <%s>\n", __LINE__, line);			

			// open file, create if not present
			FILE *file = fopen("/var/tmp/aesdsocketdata", "a"); // Use "wb" for binary, "w" for text
			if (file == NULL) {
				printf("Error: Could not create/open file.\n");
				exit(1);
			}

			// write data to file
			size_t result = fwrite(line, 1, len, file);
			if (result != (size_t)len) {
				printf("Error: Failed to write all data.\n");
			} else {
				printf("Data written successfully.\n");
			}
			
			// close the file and free memory allocated by getline
			fclose(file);
			free(line);
			
			// send reply to client, reopen the file but this time for reading
			file = fopen("/var/tmp/aesdsocketdata", "r");
            if (!file) {
				printf("Error: could not open file for reading, error=%s\n", strerror(errno));
				return -1;
			}

            // read from file line by line
            // f. ... you may not assume this total size of all packets sent will be less than the size of the
            // available RAM for the process heap
            // use {} because variables are named same as above
            {
				char* line = NULL;      // getline() will allocate, we must free
				size_t cap = 0;         // capacity managed by getline
				ssize_t len;
				while ((len = getline(&line, &cap, file)) != -1) {
					
					//printf("Info: line=<%s>, len=%zd\n", line, len);

					// send requires a while loop
					//
					// This is required by POSIX and documented explicitly in Linux man 2 send:
					//
					// "The call may return the number of bytes actually sent, which can be less than the number requested to be sent..."
					// (Linux manual page — man 2 send)
					//
					size_t total_sent = 0;
					while (total_sent < (size_t)len) {
						ssize_t sent = send(new_fd, line + total_sent, len - total_sent, 0);

						if (sent <= 0) {
							printf("Error: send failed, error=%s\n", strerror(errno));
							free(line);
							fclose(file);
							return -1;
						}
						total_sent += sent;
					}
				}

				free(line);
				fclose(file);
			}
		}
		
		// we will also leave outer loop only if isApplicationOrderedToStop due to the while() condition
		printf("left inner loop, isApplicationOrderedToStop=%u\n", isApplicationOrderedToStop);
	}
	
	printf("left outer loop\n");
	
	// clean shutdown
	close(new_fd);
	close(sockfd);
	freeaddrinfo(servinfo); // free the linked-list
	
	// delete the file - use if 0 for debugging
#if 1
	if (remove("/var/tmp/aesdsocketdata") != 0) {
		perror("Error deleting file");
	}
#endif

  return 0;
}


