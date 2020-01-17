#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd)
  char *file;        // the input/output file
  int flags;         // flags for open() indicating read or write
  int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe
  struct cmd *right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);
    fprintf(stderr, "exec not implemented\n");
    // Your code here ...
    if (access(ecmd->argv[0], F_OK) == 0)
        {
            execv(ecmd->argv[0], ecmd->argv);
        }
        else
        {
            const char *bin_path[] = {
                "/bin/",
                "/usr/bin/"};
            char *abs_path;
            int bin_count = sizeof(bin_path) / sizeof(bin_path[0]);
            int found = 0;
            for (int i = 0; i < bin_count && found == 0; i++)
            {
                int pathLen = strlen(bin_path[i]) + strlen(ecmd->argv[0]);
                abs_path = (char *)malloc((pathLen + 1) * sizeof(char));
                strcpy(abs_path, bin_path[i]);
                strcat(abs_path, ecmd->argv[0]);
                if (access(abs_path, F_OK) == 0)
                {
                    execv(abs_path, ecmd->argv);
                    found = 1;
                }
                free(abs_path);
            }
            if (found == 0)
            {
                fprintf(stderr, "%s: Command not found\n", ecmd->argv[0]);
            }
        }
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    //fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    close(rcmd->fd);
       if (open(rcmd->file, rcmd->flags, 0644) < 0)
       {
         fprintf(stderr, "Unable to open file: %s\n", rcmd->file);
            exit(0);
       }
    runcmd(rcmd->cmd);
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    //fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    // 建立管道
    if (pipe(p) < 0)
       fprintf(stderr, "pipe failed\n");
    // 先fork一个子进程处理左面的命令，
    // 并将左面命令的执行结果的标准输出定向到管道的输入
    if (fork1() == 0)
    {
      // 先关闭标准输出再 dup
      close(1);
      // dup 会把标准输出定向到 p[1] 所指文件，即管道写入端
      dup(p[1]);
      // 去掉管道对端口的引用
      close(p[0]);
      close(p[1]);
      // 此时 left 的标准输入不变，标准输出流入管道
      runcmd(pcmd->left);
    }
    // 再fork一个子进程处理右面的命令，
    // 将标准输入定向到管道的输出，
    // 即读取了来自左面命令返回的结果
    if (fork1() == 0)
    {
     // 先关闭标准输入再 dup
     close(0);
     // dup 会把标准输入定向到 p[0] 所指文件，即管道读取端
     dup(p[0]);
     // 去掉管道对端口的引用
     close(p[0]);
     close(p[1]);
     // 此时 right 的标准输入从管道读取，标准输出不变
     runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait(&r);
    wait(&r);
    break;
  }    
  _exit(0);
}

// 获取命令，并将当前命令存入缓冲字符串，以便后面进行处理
int
getcmd(char *buf, int nbuf)
{ 
  // 判断是否为终端输入
  if (isatty(fileno(stdin)))
    fprintf(stdout, "6.828$ ");
   // 清空存储输入命令的缓冲字符串
  memset(buf, 0, nbuf);
   // 将终端输入的命令存入缓冲字符串
  if(fgets(buf, nbuf, stdin) == 0)
    return -1; // EOF
  return 0;
}

int
main(void)
{
  //存储输入的命令
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  //当输入命令不为空时，读取并运行输入命令
  while(getcmd(buf, sizeof(buf)) >= 0){
    // 如果是[cd fileName]命令，进行目录切换
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
        //continue继续读取命令
      continue;
    }
    // 否则fork出子进程，来执行输入的命令
    // fork()函数详解 http://www.cnblogs.com/jeakon/archive/2012/05/26/2816828.html
    //如果返回0，说明是子进程，进而在子进程中执行相应的命令
    if(fork1() == 0)
      runcmd(parsecmd(buf));
      // wait函数介绍 http://www.jb51.net/article/71747.htm
      // wait()函数用于使父进程(也就是调用wait()的进程)阻塞，
      // 直到一个子进程结束或该进程接收到一个指定的信号为止。
      // 如果该父进程没有子进程或它的子进程已经结束，
      // 则wait()就会立即返回。
      // pid_t wait (int * status);
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
