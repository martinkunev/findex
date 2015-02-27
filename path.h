enum {PATH_SIZE_LIMIT = 4096};

char *normalize(const char *restrict relative, size_t relative_length, size_t *restrict path_length);
