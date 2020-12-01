#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PIPE_BUF_SIZE 4096

using namespace std;

int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		cout << "Usage: " << argv[0] << " InterpreterFile ScriptFile" << endl;
		exit(0);
	}

	int pipes[2];

	pipe(pipes);

	pid_t pid = fork();

	if (pid == 0) //child
	{
		dup2(pipes[1], STDOUT_FILENO);
		dup2(pipes[1], STDERR_FILENO);

		close(pipes[1]);
		close(pipes[0]);

		if (execv(argv[1], argv + 1) == -1)
		{
			cout << "Error Occured!" << endl;
			exit(errno);
		}
	}
	
	// father
	
	close(pipes[1]);

	char* buf = (char*)malloc(PIPE_BUF_SIZE);

	int bytesRead = read(pipes[0], buf, PIPE_BUF_SIZE);
	buf[bytesRead + 1] = 0;

	int status;

	waitpid(pid, &status, WUNTRACED);

	if (WIFSIGNALED(status))
	{
		int sig = WTERMSIG(status);
		switch (sig)
		{
		case SIGSEGV:
			cout << "Segmentation Fault." << endl;
			return sig;

		default:
			cout << "Exception occured in child process! Signal: " << sig <<endl;
			return sig;
		}
	}
	
	cout << buf << endl;
	
	return 0;
}

