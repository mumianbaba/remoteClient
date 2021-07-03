#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

/* set the client.cfg path default ./client.cfg */
void cfg_set_path(char* path);
/* get key form file */
int cfg_getinfo(char* key, char** value);


#endif