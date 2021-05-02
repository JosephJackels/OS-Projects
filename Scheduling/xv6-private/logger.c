#include "logger.h"
/*
	logger {
		int mode; 0 = off, 1 = log all, 2 = log selected
		char *path[]; // logfile path
		int selected[]; // selected pids to log
		int size; // amount of pids in selected, also functions as index of first empty spot
	}
*/
int initlogger(struct logger *l){
	l->mode = 0;
	l->size = 0;
	if((l->path = kalloc()) == 0){
		//could not allocate path
		return 1;
	}
	if((l->selected = kalloc()) == 0){
		//could not allocate selected
		return 2;
	}
	return 0;
}
void setpath(struct logger *l, char *p[]){
		memmove(l->path, p, sizeof(p));
}
void setmode(struct logger *l, int mode){
	logger->mode = mode;
}

int addselected(struct logger *l, int pid){
	if(l->size > 1023){
		//more than one page of ints!;
		//allocate another page?
		return 1;
	}

	l->selected[size] = pid;
	l->size++;
}

int removeselected(struct logger *l, int pid){
	for(int i = 0; i < l->size; i++){
		if(l->selected[i] == pid){
			for(int j = i; j < l->size - 1; j++){
				l->selected[j] = l->selected[j+1];
			}
			l->size--;
			return 0;
		}
	}
	//not found
	return 1;
}

int logstring(struct logger *l, char *string[]){
	//open path

	//write path
}

void openlogger(struct logger*l){
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

void writelogger(){

}