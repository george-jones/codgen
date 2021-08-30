#ifndef MAZEMAKER_H
#define MAZEMAKER_H

typedef struct {
    unsigned int up      : 1;
    unsigned int right   : 1;
    unsigned int down    : 1;
    unsigned int left    : 1;
    unsigned int path    : 1;
    unsigned int visited : 1;
} cell_t;
// I don't really like this, but am too lazy to change it right now.  It would be better to have a structure that
// contains a pointer to the cells, and also the width and height so the caller doesn't have to lug those around.
typedef cell_t* maze_t;

maze_t InitMaze(int width, int height);
void FreeMaze(maze_t maze);

void CreateMaze(maze_t maze, int width, int height);

#endif // MAZEMAKER_H
