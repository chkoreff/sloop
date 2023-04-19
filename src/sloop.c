#include <arpa/inet.h> // socket inet_addr
#include <ctype.h> // isdigit
#include <signal.h> // sigaction
#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // strcmp strlen
#include <sys/wait.h> // waitpid
#include <unistd.h> // dup2 close usleep fork getpgid
#include <util.h>

static int argc;
static const char **argv;

static char full_path[128];

unsigned long path_pos_1;
unsigned long path_pos_2;

static pid_t do_fork(void)
	{
	pid_t pid = fork();
	if (pid == -1) die("fork failed");
	return pid;
	}

static void do_dup2(int oldfd, int newfd)
	{
	int status = dup2(oldfd,newfd);
	if (status == -1) die("dup2 failed");
	}

static void do_close(int fd)
	{
	int status = close(fd);
	if (status == -1) die("close failed");
	}

static void die_perror(const char *msg)
	{
	perror(msg);
	die(0);
	}

static void handle_signal(int signum)
	{
	(void)signum;
	}

/* Reference:
https://stackoverflow.com/questions/31784823/interrupting-open-with-sigalrm

This sets a signal handler so it does not kill the process when the signal
happens, but instead interrupts any system call in progress.
*/
static void set_handler(int signum)
	{
	struct sigaction sa;
	sa.sa_handler = handle_signal;
	sa.sa_flags = 0; // Override default SA_RESTART.
	sigemptyset(&sa.sa_mask);
	if (sigaction(signum, &sa, NULL) < 0)
		die_perror("sigaction(2) error");
	}

static int make_socket(const char *ip, unsigned long port)
	{
	int fd_listen = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd_listen == -1)
		die_perror("socket");

	/* Set flags so you can restart the server quickly.  Otherwise you get
	the error "Address already in use" if you stop the server while a
	client is connected and then try to restart it within TIME_WAIT
	interval. */
	{
	int on = 1;
	setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	}

	// Bind the socket to the ip and port.
	{
	unsigned int count = 10; // Max retries, usually 1 suffices.
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(port);

	while (1)
		{
		if (bind(fd_listen, (struct sockaddr *)&addr, sizeof(addr)) != -1)
			break;

		if (count == 0)
			die_perror("bind");

		/* If the previous server process has been killed but the socket system
		hasn't quite released the address yet, the bind can fail.  In that case
		I try again to give it a little more time. */

		count--;
		usleep(10); // Sleep 10 microseconds.
		}
	}

	// Listen to the socket.
	{
	int backlog = 10;
	if (listen(fd_listen, backlog) == -1)
		die_perror("listen");
	}

	return fd_listen;
	}

static char *get_full_path(const char *name)
	{
	safe_copy(full_path, sizeof(full_path), path_pos_2+1, name, strlen(name));
	return full_path;
	}

static void write_pid(pid_t pid)
	{
	FILE *fh = fopen(get_full_path("run/pid"),"w+");

	if (fh == 0)
		die("Could not open pid file");

	fprintf(fh,"%d\n",pid);
	fclose(fh);
	}

// Stop any running server and its children.
static void stop_server()
	{
	FILE *fh = fopen(get_full_path("run/pid"),"r");
	if (fh)
		{
		int pid = 0;
		while (1)
			{
			int ch = fgetc(fh);
			if (!isdigit(ch))
				break;
			pid = 10*pid + (ch - '0');
			}

		if (pid)
			{
			pid_t pgid = getpgid(pid);
			if (pgid != -1)
				kill(-pgid,SIGINT);
			}

		fclose(fh);

		// Remove the pid file.
		remove(full_path);
		}
	}

static FILE *open_error_log(void)
	{
	FILE *fh = fopen(get_full_path("run/error_log"),"a+");
	if (fh == 0)
		die("Could not open error log");

	return fh;
	}

// Accept an inbound connection and handle requests.
static void do_connection(int fd_listen, void do_session(void))
	{
	{
	/* If a child exits I get a SIGCHLD signal which interrupts any system
	call, giving me a chance to wait for any child processes to finish here.
	This prevents defunct processes. */
	while (1)
		{
		int status;
		int pid = waitpid(-1,&status,WNOHANG);
		if (pid <= 0) break;
		}
	}

	{
	struct sockaddr_in addr;
	socklen_t size = sizeof(addr);

	int channel = accept(fd_listen, (struct sockaddr *)&addr, &size);
	if (channel != -1)
	{
	// Fork a process to handle the connection.
	pid_t pid = do_fork();
	if (pid == 0)
		{
		// Child process

		/* Close the listening socket so you can restart the server after
		killing it with a client still connected.  Any such clients continue to
		run with the old server process, but the new server process can now
		receive connections. */
		close(fd_listen);

		// Close stdin and stdout and replace them with the client socket.
		close(0);
		close(1);

		do_dup2(channel,0);
		do_dup2(channel,1);

		// Make stdout unbuffered to you don't have to call fflush.
		setvbuf(stdout,0,_IONBF,0);

		do_close(channel);

		do_session();
		exit(0);
		}
	else
		{
		// Parent process
		do_close(channel);
		}
	}
	}
	}

static void start_server(const char *ip, unsigned long port,
	void do_session(void))
	{
	// Fork the server so it runs in the background.
	pid_t pid = do_fork();
	if (pid == 0)
		{
		// Child process
		int fd_listen = make_socket(ip,port);

		{
		// Redirect stderr to log file.
		FILE *fh_log = open_error_log();
		do_dup2(fileno(fh_log),2);
		fclose(fh_log);
		}

		while (1)
			do_connection(fd_listen, do_session);

		close(fd_listen);
		exit(0);
		}
	else
		{
		// Parent process
		write_pid(pid);
		}
	}

static void usage(void)
	{
	fprintf(stderr,"Usage: %s [start|stop]\n", argv[0]+path_pos_1+1);
	exit(1);
	}

static void run_sloop(const char *ip, unsigned long port,
	void do_session(void))
	{
	int start = 0;

	if (argc < 2)
		usage();

	{
	const char *arg = argv[1];
	if (strcmp(arg,"start") == 0)
		start = 1;
	else if (strcmp(arg,"stop") == 0)
		;
	else
		usage();
	}

	set_handler(SIGALRM);
	set_handler(SIGCHLD);

	stop_server();

	if (start)
		start_server(ip,port,do_session);
	}

// Return the position of the '/' in buf which occurs before pos.  Return 0 if
// not found (or if found at position 0).
static unsigned long find_slash_before(const char *buf, unsigned long pos)
	{
	while (pos > 0)
		{
		char ch = buf[--pos];
		if (ch == '/')
			break;
		}
	return pos;
	}

static void beg_path(void)
	{
	const char *path = argv[0];
	unsigned long len = strlen(path);

	path_pos_1 = find_slash_before(path, len);
	path_pos_2 = find_slash_before(path, path_pos_1);

	safe_copy(full_path, sizeof(full_path), 0, path, path_pos_2+1);
	}

int run_server(int _argc, const char *_argv[],
	const char *ip, unsigned long port,
	void do_session(void)
	)
	{
	argc = _argc;
	argv = _argv;

	beg_path();
	run_sloop(ip,port,do_session);
	return 0;
	}
