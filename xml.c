#include <stdio.h>
#include <string.h>
#include <math.h>

#include "xml.h"

#ifdef WIN32
#define strcasecmp strcmpi
#endif

DOMNODE *XMLDomParseFileName(char *fileName)
{
	DOMNODE *n = NULL;
	FILE *f = NULL;

	f = fopen(fileName, "r");
	if (f) {
		DOMNODE *ret = NULL;
		n = XMLDomParseFile(f);
		fclose(f);
	} else {
		fprintf(stderr, "ERROR: XML Config file (%s) not found!", fileName);		
	}

	return n;
}

static void node_add_child(DOMNODE *n, DOMNODE *c)
{
	int children = 0;
	DOMNODE *curr = NULL;
	int i=0;

	for (i=0; (curr = n->children[i]) != NULL; i++);	
	if (i>0) n->children[i-1]->nextSibling = c;
	i++;	

	n->children = (DOMNODE **)realloc(n->children, sizeof(DOMNODE *) * (i+1));
	if (n->children) {		
		n->children[i-1] = c;
		n->children[i] = NULL;
	}
}

static void node_add_attribute(DOMNODE *n, char *attr_name, char *attr_val)
{
	DOMATTR *a = NULL;
	DOMATTR *ex = NULL;
	int i=0;

	a = (DOMATTR *)malloc(sizeof(DOMATTR));
	if (!a) {
		fprintf(stderr, "ERROR: Out of memory when allocating attribute from XML config file.");
		return;
	}

	a->name = strdup(attr_name);
	a->value = strdup(attr_val);

	for (i=0; (ex = n->attributes[i]) != NULL; i++);
	i++;

	n->attributes = (DOMATTR **)realloc(n->attributes, sizeof(DOMATTR *) * (i+1));
	if (n->attributes) {
		n->attributes[i-1] = a;
		n->attributes[i] = NULL;
	}

}

static void attributes_destroy(DOMNODE *n)
{
	if (n->attributes) {
		DOMATTR *a = NULL;
		int i=0;
		for (i=0; (a = n->attributes[i]) != NULL; i++) {
			free(n->attributes[i]->name);
			free(n->attributes[i]->value);
			free(n->attributes[i]);
		}
		free(n->attributes);
	}
}

void XMLDomDestroy(DOMNODE *n)
{
	if (n) {	
		if (n->children) XMLDomDestroy(n->children[0]);	
		XMLDomDestroy(n->nextSibling);
		if (n->tagName) free(n->tagName);
		attributes_destroy(n);
		free(n);
	}
}

static DOMNODE *node_create(char *tag_name, DOMNODE *parent)
{
	DOMNODE *n = NULL;

	n = (DOMNODE *)calloc(1, sizeof(DOMNODE));
	if (n) {		
		n->parentNode = parent;		
		n->tagName = strdup(tag_name);		
		n->children = (DOMNODE **)calloc(1, sizeof(DOMNODE *));		
		n->attributes = (DOMATTR **)calloc(1, sizeof(DOMATTR *));		
		if (parent) node_add_child(parent, n);
	}

	if (!n || !n->children || !n->attributes | !n->tagName) {
		fprintf(stderr, "ERROR: Unable to allocate memory for new node in XML config file!");
		XMLDomDestroy(n);
		n = NULL;
	}

	return n;
}

DOMNODE *XMLDomParseFile(FILE *f)
{
	DOMNODE *n = NULL;
	char buf[1024];
	int bytes = 0;
	int total = 0;
	char *xml = NULL;

	// read the whole file into memory
	while ((bytes = fread(buf, sizeof(char), sizeof(buf), f)) > 0) {
		total += bytes;
		xml = (char *)realloc(xml, sizeof(char) * (total+1));
		if (!xml) {
			fprintf(stderr, "ERROR: Out of memory reading XML config file!");
			return NULL;
		}
		memcpy(xml + total - bytes, buf, bytes);
	}
	if (xml) {	
		xml[total] = 0; // NULL terminate
		n = XMLDomParseMem(xml);
		free(xml);
	} else {
		fprintf(stderr, "ERROR: XML config file is empty!");
	}

	return n;
}

DOMNODE *XMLDomParseMem(char *xml)
{
	DOMNODE *top = NULL;
	DOMNODE *p = NULL;
	DOMNODE *node = NULL;
	char tag_name[256];
	char attr_name[256];
	char attr_val[256];
	short in_comment = 0;
	short in_tag = 0;
	short in_tag_ending = 0;
	short in_tag_name = 0;
	short in_attr_name = 0;
	short in_attr_val = 0;		
	char c;
	int i=0;
	int len=0;
	int tag_name_start = 0;
	int attr_name_start = 0;
	int attr_val_start = 0;	

	len = strlen(xml);

	for (i=0; i<len; i++) {
		c = xml[i];

		switch (c) {
		case '<':
			if (i+3 < len && strncmp(xml+i, "<!--", 4)==0) {
				in_comment = 1;
			} else if (!in_comment) {		
				if (!in_tag) {
					in_tag = 1;
					in_tag_ending = 0;
					// tag started
				} else {
					// bad state
					goto XMLDOMPARSEERROR;
				}
			}
			break;
		case '>':
			if (i-2 > 0 && strncmp(xml+i-2, "-->", 3)==0) {
				in_comment = 0;
			} else if (in_tag && !in_comment) {
				in_tag = 0;
				in_attr_name=0;
				if (in_tag_name) {
					in_tag_name = 0;
					tag_name[i-tag_name_start] = 0;
					if (in_tag_ending) {											
						node = p;
						if (node) p = node->parentNode;
					} else {
						p = node;
						node = node_create(tag_name, p);
						if (!p) top = node;
					}
				}				
			} else if (!in_comment) {
				// bad state
				goto XMLDOMPARSEERROR;
			}
			break;
		case '/':
			if (!in_comment) {

				if (in_tag_name) {
					in_tag_name = 0;
				}

				if (in_tag && !in_attr_val && i+1 < len && strncmp(xml+i, "/>", 2) == 0) {
					// shortcut ending tag
					node = p;
					if (node) p = node->parentNode;					
					
				} else if (in_tag && i-1 > 0 && strncmp(xml+i-1, "</", 2) == 0) {
					// end tag
					in_tag_name = 1;
					tag_name_start = i + 1;
					in_tag_ending = 1;

				} else if (in_attr_val) {
					// add to attr val
					attr_val[i-attr_val_start] = c;
					
				} else {
					// bad state					
					goto XMLDOMPARSEERROR;
				}
			}
			break;
		case '"':
			if (!in_comment) {
				if (!in_attr_val) {
					// starting attribute value
					in_attr_val = 1;
					attr_val_start = i + 1;
				} else {
					// end attribute value		
					in_attr_val = 0;
					attr_val[i-attr_val_start] = 0;					
					node_add_attribute(node, attr_name, attr_val);
				}
			}
			break;
		case ' ':
			if (in_tag) {			
				if (!in_attr_val) {
					// starting attribute name
					if (in_tag_name) {
						tag_name[i-tag_name_start] = 0;

						p = node;
						node = node_create(tag_name, p);
						if (!p) top = node;

						in_tag_name=0;
					}
					in_attr_name=1;
					attr_name_start = i+1;
				} else {
					// add to attribute value
					attr_val[i-attr_val_start] = c;
				}
			}
			break;
		case '=':
			if (in_attr_name) {
				// end attribute name
				in_attr_name=0;
				attr_name[i-attr_name_start] = 0;				
			}
			break;
		default:
			if (!in_comment) {
				if (in_tag) {
					if (xml[i-1] == '<') {
						// starting tag name
						in_tag_name = 1;
						tag_name_start = i;
						
						// add to tag name
						tag_name[i-tag_name_start] = c;
						
					} else if (in_attr_name) {
						// add to attr name
						attr_name[i-attr_name_start] = c;
						
					} else if (in_attr_val) {
						// add to attr value
						attr_val[i-attr_val_start] = c;
						
					} else if (in_tag) {
						// add to tag name
						tag_name[i-tag_name_start] = c;
						
					} else {
						// bad state
						goto XMLDOMPARSEERROR;
					}
				}				
			}
			break;
		}
	}

	// invalid end states
	if (in_comment || in_tag) goto XMLDOMPARSEERROR;

	return top;

XMLDOMPARSEERROR:
	// yes, GOTO has been considered harmful, but since c lacks try/catch I think this is a legitimate use of it.

	fprintf(stderr, "ERROR: config XML file improperly formatted.");

	// do cleanup
	XMLDomDestroy(top);

	return NULL;
}

static void xml_dom_debug_indent(int indent)
{
	int i=0;
	for (i=0; i<indent; i++) fprintf(stdout, "\t");	
}

static void xml_dom_debug(DOMNODE *n, int indent)
{
	if (n) {		
		DOMATTR *a = NULL;
		DOMNODE *c = NULL;
		int i=0;

		xml_dom_debug_indent(indent);
		fprintf(stdout, "<%s", n->tagName);
		for (i=0; (a = n->attributes[i]) != NULL; i++) {
			fprintf(stdout, " %s=\"%s\"", a->name, a->value);
		}
		fprintf(stdout, ">\r\n");
		for (i=0; (c = n->children[i]) != NULL; i++) {
			xml_dom_debug(c, indent+1);
		}
		xml_dom_debug_indent(indent);
		fprintf(stdout, "</%s>\r\n", n->tagName);
	}
}

void XMLDomDebug(DOMNODE *n)
{
	xml_dom_debug(n, 0);
}

char *XMLDomGetAttribute(DOMNODE *n, char *attrName)
{
	DOMATTR *a = NULL;
	int i=0;

	for (i=0; (a = n->attributes[i]) != NULL; i++) {
		if (strcasecmp(attrName, a->name) == 0) {
			return a->value;
		}
	}

	return NULL;
}

int XMLDomGetAttributeInt(DOMNODE *n, char *attrName, int def_val)
{
	char *val = NULL;
	int ret = def_val; // default

	val = XMLDomGetAttribute(n, attrName);
	if (val) {
		ret = atoi(val);
	}

	return ret;
}

float XMLDomGetAttributeFloat(DOMNODE *n, char *attrName, float def_val)
{
	char *val = NULL;
	float ret = def_val; // default

	val = XMLDomGetAttribute(n, attrName);
	if (val) {		
		ret = atof(val);
	}

	return ret;
}

int XMLDomGetAttributeTF(DOMNODE *n, char *attrName, int def_val)
{
	char *val = NULL;
	int ret = def_val;

	val = XMLDomGetAttribute(n, attrName);
	if (val) {		
		if (ret == 1 && (val[0] == 'f' || val[0] == 'F')) {
			ret = 0;
		} else if (ret == 0 && (val[0] == 't' || val[0] == 'T')) {
			ret = 1;
		}
	}

	return ret;
}

DOMNODE *XMLDomGetChildNamed(DOMNODE *p, char *tagName)
{
	DOMNODE *n = NULL;
	int i=0;

	for (i=0; (n = p->children[i]) != NULL; i++) {
		if (strcasecmp(n->tagName, tagName) == 0) {
			return n;
		}
	}

	return n;
}


