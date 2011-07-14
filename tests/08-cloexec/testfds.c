#include <onion/log.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @short Tests how many file descriptor it has open. 
 * 
 * Should be the ones that argv says, no more no less. Prints result, and returns 0 if ok, other if different.
 */
int main(int argc, char **argv){
	ONION_INFO("Hello, Im going to check it all");

	int nfd=0;

	DIR *dir=opendir("/proc/self/fd");
	struct dirent *de;
	
	while ( (de=readdir(dir))!=NULL ){
		if (de->d_name[0]=='.')
			continue;
		ONION_INFO("Checking fd %s", de->d_name);
		int i;
		for (i=1;i<argc;i++){
			if (strcmp(de->d_name, argv[i])==0){
				ONION_INFO("OK on argv");
				break;
			}
		}
		if (i==argc){
			if (dirfd(dir)==atoi(de->d_name)){
				ONION_INFO("Ok, its the dir descriptor");
			}
			else{
				ONION_ERROR("Hey, one fd I dont know about!");
				char tmp[256];
				snprintf(tmp, sizeof(tmp), "ls --color -l /proc/%d/fd/%s", getpid(), de->d_name);
				system(tmp);
				nfd++;
			}
		}
	}
	
	closedir(dir);
	
	return nfd;
}
