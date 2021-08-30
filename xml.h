#ifndef _XML_H
#define _XML_H

#include <stdio.h>

//
// DOM XML parser.  We're looking for ease of use here, not efficiency.
// node data is currently not supported.  all data must be in attributes.

typedef struct {
	char *name;
	char *value;
} DOMATTR;

typedef struct _DOMNODE {
	char *tagName;
	struct _DOMNODE *parentNode;
	struct _DOMNODE *nextSibling;	
	struct _DOMNODE **children; // NULL-terminated array	
	DOMATTR **attributes; // NULL-terminated array
} DOMNODE;

DOMNODE *XMLDomParseFileName(char *fileName);

DOMNODE *XMLDomParseFile(FILE *f);

DOMNODE *XMLDomParseMem(char *xml);

void XMLDomDestroy(DOMNODE *n);

void XMLDomDebug(DOMNODE *n);

char *XMLDomGetAttribute(DOMNODE *n, char *attrName);

int XMLDomGetAttributeInt(DOMNODE *n, char *attrName, int def_val);

float XMLDomGetAttributeFloat(DOMNODE *n, char *attrName, float def_val);

// whatever
int XMLDomGetAttributeTF(DOMNODE *n, char *attrName, int def_val);

// what a great function
DOMNODE *XMLDomGetChildNamed(DOMNODE *p, char *tagName);

#endif
