/*
	Kenan Krijestorac
	Project 5 - Memory Management
	Due: 26 April 2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

#include "shared.h"

FILE * fp;

void logCheck(char*);
void usage();

int main(int argc, char* argv[]) {

	int opt;
	int m = 18;
	char * fileName = "output.log";
	
	while((opt = getopt(argc, argv, "hm:")) != -1) {
		switch(opt) {	
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			
			case 'm':
				if(!isdigit(*optarg) || (m = atoi(optarg)) < 0 || (m = atoi(optarg)) > 18) {
					perror("oss.c: error: invalid max process");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				perror("oss.c: error: invalid argument");
				usage();
				exit(EXIT_FAILURE);
		}
		
	}
	
	printf("m: %d\n", m);
	printf("fileName: %s\n", fileName);
	
	logCheck(fileName);
	
	fprintf(fp, "i like dogs\n");
	
	fclose(fp);
	

	return EXIT_SUCCESS;
}

void logCheck(char * file) {
	fp = fopen(file, "w");

	if(fp == NULL) {
		perror("oss.c: error: failed to open log file");
		exit(EXIT_FAILURE);
	}
}

void usage() {
	printf("======================================================================\n");
        printf("\t\t\t\tUSAGE\n");
        printf("======================================================================\n");
        printf("./oss -h [-s t] [-l f]\n");
        printf("run as: ./oss [options]\n");
        printf("ex. : ./oss -m 10\n");
        printf("if [-m t] is not defined it is set as default 18\n");
        printf("======================================================================\n");
        printf("-h              :       Describe how project should be run and then, terminate\n");
        printf("-m t            :       Indicate the number of \n");
        printf("======================================================================\n");
}
