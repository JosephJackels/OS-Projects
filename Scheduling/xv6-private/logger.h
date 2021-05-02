struct logger {
	int mode; //0 = off, 1 = log all, 2 = log selected;
	char *path[];// path to log to
	int selected[];//array of pids to log if mode is selected
	int size;
}