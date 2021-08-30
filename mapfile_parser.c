
#include "primitives.h"
#include "mapfile_parser.h"

// returns 0 if there was no line to read.  trims leading spaces
static int readline(FILE *f, char *buf, size_t siz)
{
	char c;
	int i=0;
	int j=0;
	int in_margin=1;
	
	for (i=0; i<siz-1; i++) {
		c = fgetc(f);
		if (!in_margin || c != ' ') {
			if (c == '\n' || c == EOF) {
				if (c == EOF && j == 0) j = -1;
				break;
			} else {
				buf[j] = c;
			}
			j++;
			in_margin = 0;
		}
	}

	if (j > -1) buf[j] = 0;	
	
	return j+1;
}

static void parse_brush_line(MAPINFO *mi, char *buffer, int first)
{
	FloatPoint3 p[3];
	int i=0;

	sscanf(buffer, "( %f %f %f ) ( %f %f %f ) ( %f %f %f )",
		   &p[0].x, &p[0].y, &p[0].z,
		   &p[1].x, &p[1].y, &p[1].z,
		   &p[2].x, &p[2].y, &p[2].z);

	for (i=0; i<3; i++) {
		if ((i==0 && first) || p[i].x < mi->p1.x) mi->p1.x = p[i].x;		
		if ((i==0 && first) || p[i].x > mi->p2.x) mi->p2.x = p[i].x;
		if ((i==0 && first) || p[i].y < mi->p1.y) mi->p1.y = p[i].y;		
		if ((i==0 && first) || p[i].y > mi->p2.y) mi->p2.y = p[i].y;
		if ((i==0 && first) || p[i].z < mi->p1.z) mi->p1.z = p[i].z;		
		if ((i==0 && first) || p[i].z > mi->p2.z) mi->p2.z = p[i].z;
	}
}

void mapfile_parse(char *filename, MAPINFO *mi)
{
	FILE *f = fopen(filename, "r");

	mi->p1.x = 0;
	mi->p1.y = 0;
	mi->p1.z = 0;

	mi->p2.x = 0;
	mi->p2.y = 0;
	mi->p2.z = 0;

	mi->valid = 1;

	if (f) {
		char buf[1024];
		int j = 0;
		int cb = 0;
		int len = 0;
		int first = 1;

		for (j=0; (len = readline(f,buf,sizeof(buf))) > 0; j++) {
			if (j==0) {
				if (strcmp(buf, "iwmap 4") != 0) { // first line should say this
					mi->valid = 1;
					break;
				}
			} else {
				if (strncmp(buf, "//", 2) == 0) continue; // ignore comments
				switch (buf[0]) {
				case '{':
					cb++;
					continue;
				case '}':
					cb--;
					continue;
				case '(':
					if (cb == 2) { // we are in a brush, not a curve, not a mesh.
						parse_brush_line(mi, buf, first);
						first = 0;
					}
					break;
				}

			}
		}		

		fclose(f);
	}
}
