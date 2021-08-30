/*
 * MazeGen.c -- Mark Howell -- 8 May 1991
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mazemaker.h"
#include "primitives.h"

#define WIDTH 39
#define HEIGHT 11

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3
#ifdef TRUE
#undef TRUE
#endif /* TRUE */

#define TRUE 1

#define cell_empty(a) (!(a)->up && !(a)->right && !(a)->down && !(a)->left)

maze_t InitMaze(int width, int height)
{
	return (maze_t) calloc (width * height, sizeof (cell_t));
}

void FreeMaze(maze_t maze)
{
	free(maze);
}

void CreateMaze (maze_t maze, int width, int height)
{
    maze_t mp, maze_top;
    char paths [4];
    int visits, directions;

    visits = width * height - 1;
    mp = maze;
    maze_top = mp + (width * height) - 1;

    while (visits) {
        directions = 0;

        if ((mp - width) >= maze && cell_empty (mp - width))
            paths [directions++] = UP;
        if (mp < maze_top && ((mp - maze + 1) % width) && cell_empty (mp + 1))
            paths [directions++] = RIGHT;
        if ((mp + width) <= maze_top && cell_empty (mp + width))
            paths [directions++] = DOWN;
        if (mp > maze && ((mp - maze) % width) && cell_empty (mp - 1))
            paths [directions++] = LEFT;

        if (directions) {
            visits--;
            directions = ((unsigned) rand () % directions);

            switch (paths [directions]) {
                case UP:
                    mp->up = TRUE;
                    (mp -= width)->down = TRUE;
                    break;
                case RIGHT:
                    mp->right = TRUE;
                    (++mp)->left = TRUE;
                    break;
                case DOWN:
                    mp->down = TRUE;
                    (mp += width)->up = TRUE;
                    break;
                case LEFT:
                    mp->left = TRUE;
                    (--mp)->right = TRUE;
                    break;
                default:
                    break;
            }
        } else {
            do {
                if (++mp > maze_top)
                    mp = maze;
            } while (cell_empty (mp));
        }
    }
}

// this function is kind of screwy - it does the points in the reverse order from expected, but that's
// because we feed it wall coordinates using an upside down frame-of-mind.  Feel free to fix it all,
// just be warned that if the coordinates are in the wron order, your walls will be invalid and won't
// even appear in Radiant, much less the game itself.
static void maze_wall(FILE *f, int i, int j, int vert, int wall_width, int hall_width, int wall_height, char *texture)
{
	IntPoint3 p[4];

	p[2].x = i * (wall_width + hall_width) + wall_width;

	if (vert) {
		p[2].y = j * (wall_width + hall_width) + 2 * wall_width;

		p[1].x = p[2].x + wall_width;
		p[1].y = p[2].y;
	
		p[0].x = p[1].x;
		p[0].y = p[2].y + hall_width - 2 * wall_width;
	} else {
		p[2].y = j * (wall_width + hall_width) + wall_width;

		p[1].x = p[2].x + hall_width - 2 * wall_width;
		p[1].y = p[2].y;
	
		p[0].x = p[1].x;
		p[0].y = p[2].y + wall_width;
	}
	p[3].x = p[2].x;
	p[3].y = p[0].y;

	p[0].z = p[1].z = p[2].z = p[3].z = 0;
	box(f, p, wall_height, texture);
}

void OutputMaze(FILE *f, maze_t m, int width, int height, int wall_height, int wall_width, int hall_width, char *texture)
{
	int i=0;
	int j=0;	

	for (j=0; j < height; j++) {	
		for (i=0; i < width; i++) {
			cell_t cell = m[i + j*width];
			if (j>0 && cell.up == 0) maze_wall(f, i, j, 0, wall_width, hall_width, wall_height, texture);				
			if (i>0 && cell.left == 0) maze_wall(f, i, j, 1, wall_width, hall_width, wall_height, texture);
		}
	}
}
