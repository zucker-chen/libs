#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main (int argc, char *argv[])
{
	int i = 0;

	printf("content-type:text/html;charset=utf-8\n\n"); 
	printf("<tile>Hello World</tile>");
	printf("<h3>Hi, Welcome Home.</h3>");

	for (i = 0; i < argc; i++)
	{
		printf("argv[%d] = %s</br>\n", i, argv[i]);
	}

	printf("HTTP_HOST = %s</br>\n", getenv("HTTP_HOST"));
	printf("REQUEST_URI = %s</br>\n", getenv("REQUEST_URI"));
	printf("QUERY_STRING = %s</br>\n", getenv("QUERY_STRING"));

	char *method = NULL;
	int length = 0;
	method = getenv("REQUEST_METHOD");
	if (!strcmp(method, "POST")) {
		length = atoi(getenv("CONTENT_LENGTH"));
		if (length != 0) {
			char *in_data = malloc(sizeof(char)*length + 1);
			fread(in_data, sizeof(char), length, stdin);
			printf("POST CONTENT: %s</br>\n", in_data);
			free(in_data);
		}
	} else if (!strcmp(method, "GET")) {
		char *in_data = getenv("QUERY_STRING");
		printf("GET QUERY_STRING: %s</br>\n", in_data);
	}


	return 0;
}
