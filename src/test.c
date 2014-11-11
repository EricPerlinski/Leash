#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "utils.h"


void test_2_read_writeFD(){
	int size;
	int written;
	printf("TEST\n");
	FILE* fic = fopen("fichiers_test/grande_taille.txt","r");
	fseek(fic,0,SEEK_END);
	size=ftell(fic);
	rewind(fic);
	printf("%d\n",size);
	int fdfic = open("fichiers_test/grande_taille.txt",O_RDONLY);
	char buff[1024];
	memset(buff,0,1024);
	if((read(fdfic,buff,sizeof buff))==-1){
	printf("Prob read\n");
	}	
	printf("%s\n",buff);
	written=readWriteFD(fdfic,1,size);
	printf("%d-%d\n",written,fdfic);
	fclose(fic);
}

void test_fonction_ecriture_lecture_readWriteFD(){
	int send;
	char buffer_out[1024];
	memset(buffer_out,0,sizeof buffer_out);
	strcpy(buffer_out,"TEST A ENVOYER\n");

	int pipefd[2];
	pipe(pipefd);
        
	if((send=write(pipefd[1],buffer_out,sizeof buffer_out))==-1){
		printf("test>Probleme ecriture buffer_out\n");
	}
	int written;
        written=readWriteFD(pipefd[0],1,1024);
        char buffer_in[written];
        memset(buffer_in,0,written);
        if((read(pipefd[0],&buffer_in,sizeof buffer_in))==-1){
		printf("test>Problème lecture buffer_in\n");
	}

	printf("|%s|\n",buffer_in);

}

int main(int argc, char *argv[]){
	/*
	int err=untar(argv[1],argv[2]);
	printf("err : %d\n", err);
	test_fonction_ecriture_lecture_readWriteFD();
	printf("TEST2\n");
	*/test_2_read_writeFD();
	return 0;
}
