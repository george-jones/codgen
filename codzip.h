#ifndef CODZIP_H
#define CODZIP_H

typedef struct _file_2_zip_ {
	char abs_path[255];
	char path_in_zip[255];
	struct _file_2_zip_ *next;
} FILE2ZIP;

int CreateZIP(char *zipfilename, FILE2ZIP *files);

#endif //CODZIP_H
