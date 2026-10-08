int capture_begin(int *saved);
int capture_end(int fd, int saved, char *output, size_t capacity);
int bonus_run(int argc, char **argv, const char *const *names,
    int (*const *cases)(void), size_t count);
int bonus_valid_state(void);
