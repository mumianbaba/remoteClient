#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CONFIG_FILE1 "/tmp/client.cfg"
 
#ifndef bool
typedef int bool;
#define false  0
#define true   1
#endif

static char cfg_path[64];

void cfg_set_path(char* path)
{
    if(!path)
	{
        printf("%s:%d  param err\n", __func__, __LINE__);
        return;
    }
    snprintf(cfg_path, sizeof(cfg_path), "%s", path);
}


int cfg_getinfo(char* key, char** value)
{
	if (!key || !value)
	{
		printf ("para is null\n");
		return -1;
	}
	FILE * fp1; 
	FILE * fp2;
	
	bool check1 = false;
	bool check2 = false;
	
    * value = NULL;
	int ret = access(cfg_path, F_OK);
	if (ret == 0 && (fp1 = fopen(cfg_path, "r")))
	{
		check1 = true;	
	}
 
	ret = access(CONFIG_FILE1, F_OK);
	if (ret == 0 && (fp2 = fopen(CONFIG_FILE1, "r")))
	{
		check2 = true;	
	}

	char *line = NULL;
	char*    p = NULL;
	char* goal = NULL;
	
	size_t len = 0;
	ssize_t read;
	int i = 0;
	if (check1)
	{
		while ((read = getline(&line, &len, fp1)) != -1) 
		{
			//printf("Retrieved line of length %zu :\n", read);
			//printf("%s", line);
			p = strstr(line, key);
			if (p){
				p += strlen(key);
				//printf ("find the %s\n", key);
				for (i=0 ; *p == ' ' || *p == '\t';p++, i++);
				if (i != 0){
					goal = (char*)calloc(1, strlen(p)+1);
					strcpy(goal, p);
					*value = goal;
					//printf("Retrieved line of length %zu :\n", read);
					//printf("%s", line);
					break;
				}
			}
		}
		free(line);
		fclose(fp1);
		
		if (goal)
		{
			//printf ("find the key:%s value:%s at %s\n",key, *value, CONFIG_FILE1);
			return 0;
		}
	}

	line = NULL;
	p = NULL;
	goal = NULL;	
	len = 0;
 
	if (check2)
	{
		while ((read = getline(&line, &len, fp2)) != -1) 
		{
			//printf("Retrieved line of length %zu :\n", read);
			//printf("%s", line);
			p = strstr(line, key);
			if (p){
				p += strlen(key);
				for (i=0  ; *p == ' ' || *p == '\t';p++, i++);
				if (i != 0){
					goal = (char*)calloc(1, strlen(p)+1);
					strcpy(goal, p);
					*value = goal;
					//printf("Retrieved line of length %zu :\n", read);
					//printf("%s", line);
					break;
				}
			}
		}
 
		free(line);
		fclose(fp2);
		if (goal)
		{
			//printf ("find the key:%s value:%s at %s\n",key, *value, CONFIG_FILE1);
			return 0;
		}
	}
 
	printf ("not find the the %s\n", key);
	return -1;	
}