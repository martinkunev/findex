enum {PATH_SIZE_LIMIT = 4096};

int db_path_init(char path[static PATH_SIZE_LIMIT]);
int normalize(char path[static restrict PATH_SIZE_LIMIT], size_t *restrict path_length, const char *restrict raw, size_t raw_length);
