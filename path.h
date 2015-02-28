enum {PATH_SIZE_LIMIT = 4096};

char *normalize_(const char *restrict relative, size_t relative_length, size_t *restrict path_length);
int normalize(char path[static restrict PATH_SIZE_LIMIT], size_t *restrict path_length, const char *restrict relative, size_t relative_length);
