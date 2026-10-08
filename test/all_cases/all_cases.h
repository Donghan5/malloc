pid_t waitpid(pid_t pid, int *status, int options);
int run_isolated(char *const argv[], int enabled);
int suite_count(const char *binary, size_t *count);
