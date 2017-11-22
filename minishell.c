#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX 10
#define LEN 128

struct command {
  int argc;
  char *argv[MAX];
};

struct command *cmd[2];
int num;

void execute(char *argv[]) {
  int error;
  error = execvp(argv[0], argv);
  if (error == -1)
    printf("error\n");
  exit(1);
}

void split_cmd(char *line) {
  struct command *c = (struct command *)malloc(sizeof(struct command));
  cmd[num++] = c;
  c->argc = 0;
  char *save;
  char *arg = strtok_r(line, " \t", &save);
  while (arg) {
    c->argv[c->argc] = arg;
    arg = strtok_r(NULL, " \t", &save);
    c->argc++;
  }
  c->argv[c->argc] = NULL;
}

void split_pipe(char *line) {
  char *save;
  char *arg = strtok_r(line, "|", &save);
  while (arg) {
    split_cmd(arg);
    arg = strtok_r(NULL, "|", &save);
  }
}

void do_pipe(int index) {
  if (index == num - 1)
    execute(cmd[index]->argv);
  int fd[2];
  pipe(fd);
  if (fork() == 0) {
    dup2(fd[1], 1);
    close(fd[0]);
    close(fd[1]);
    execute(cmd[index]->argv);
  }
  dup2(fd[0], 0);
  close(fd[0]);
  close(fd[1]);
  do_pipe(index + 1);
}

int inner(char *line) {
  char *save, *tmp[MAX];
  char t[LEN];
  strcpy(t, line);
  char *arg = strtok_r(line, " \t", &save);
  int i=0;
  while (arg) {
    tmp[i] = arg;
    arg = strtok_r(NULL, " \t", &save);
    i++;
  }
  tmp[i] = NULL;
  if (strcmp(tmp[0], "cd") == 0) // cd
  {
    char buf[LEN];
    if (chdir(tmp[1]) >= 0) {
      getcwd(buf, sizeof(buf));
      printf("dir:%s\n", buf);
    } else {
      printf("error\n");
    }
    return 1;
  } else
    return 0;
}

int main() {
  int pid;
  char buf[LEN], p[LEN];
  while (1) {
    printf("$:");
    fgets(buf, LEN, stdin);
    if (buf[0] == '\n')
      continue;
    buf[strlen(buf) - 1] = '\0';
    strcpy(p, buf);
    int inner_flag;
    inner_flag = inner(buf);
    if (inner_flag == 0) {
      pid = fork();
      if (pid == 0) {
        split_pipe(p);
        do_pipe(0);
        exit(0);
      }
      waitpid(pid, NULL, 0);
    }
  }
  return 0;
}
