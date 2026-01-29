#ifndef NOCTER_SCRIPT_H
#define NOCTER_SCRIPT_H

typedef struct script {
    char *head;
    char *p;
    char *file;
} script;

script loadscript(const char* path);

void skip(script* src);

void error(script* src, int len);

#endif