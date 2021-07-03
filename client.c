#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>

#include "term_history.h"
#include "term_echo.h"
#include "telnet_cmd.h"
#include "client_config.h"
#include "client.h"


static int sockfd = -1;
static TermBuffer tb;
static TermHistoryHeader hb;
static  EchoMode echoMode = SelfEchoEnable;


/* get keyboard and send to net peer */
static void keyboard_input_to_peer(void);
static int  parse_dev_id(char* src, char* dst, int len);
static void exit_callback(void);



 static char* server_ip = DEFAULT_SERVER_IP;
 static int server_port = DEFAULT_PORT; 
 static char* client_id = DEFAULT_DEVID; 
 static int client_type = TELPROXY_TYPE_CLINET;
 static int debug_flg   = 0;

static void usage(char* name)
{
    if (name){
        printf ("usage:%s ip  port  devid type\n", name);
        printf ("for example: %s 192.168.99.117 50000 TENBAY-001 c\n", name);
        printf ("usage:%s mac  , please check ./client.cfg\n", name);
        printf ("for example: %s 11:22:33:44:55:66\n", name);        
    }
}

static int param_main_argv(int argc, char** argv)
{
    int ret = 0;
    if (argc == 5 || argc == 6)
    {
        server_ip   = argv[1];
        server_port = atoi(argv[2]);
        client_id   = argv[3];
        if (argv[4][0] == 'c' || argv[4][0] == 'C'){
            client_type = TELPROXY_TYPE_CLINET; //2
        }
        else if (argv[4][0] == 'd' || argv[4][0] == 'D'){
            client_type = TELPROXY_TYPE_DEVICE; //1
        }
        else{
            printf ("%s:%d tell me your type\n",__func__, __LINE__);
            return -1;
        }
        printf ("ip:%s  port:%d  id:%s  type:%s\n", server_ip, server_port,client_id,
                (client_type==1)?"telnetd":"telnetc");
    }
    else if (argc == 2 || argc == 3)
    {
        char* p = NULL;
        p = (char*)calloc(1, 32);
        parse_dev_id(argv[1], p, 32);
        client_id =p;
        p = NULL;

        cfg_set_path("./client.cfg");
        ret = cfg_getinfo("server_ip", &p);
        if (ret == 0){
            server_ip = p;
        }
        else{
            printf ("%s:%d get server_ip failed, check the the client.cfg\n",__func__, __LINE__);
            return -1;
        }
        
        p = NULL;
        ret = cfg_getinfo("server_port", &p);
        if (ret == 0){
            server_port = atoi(p);
            free(p);
            p = NULL;
        }
        else{
            printf ("%s:%d get server_port failed, check the the client.cfg\n",__func__, __LINE__);
            return -1;
        }
    }
    else
    {
        printf ("arg num is not 2 or 5\n");
        return -1;
    }

    if (argc == 3 || argc == 6)
    {
        if (strcmp(argv[argc-1], "debug") == 0){
            debug_flg = 1;
        }
    }
    
    return 0;
}



int main(int argc, char* argv[])
{
    int ret     = 0;
    int enable  = 1;
    int recvnum = 0;
    int i       = 0;
    int telnetok = 0;
    char buf [1024];
    char macstr[32];
    TelnetState ts;
    struct TelProxyLogin login;
    struct sockaddr_in s_addr;
    char* p = NULL;
    
    ret = param_main_argv(argc, argv);
    if(ret != 0)
    {
        usage(argv[0]);
        return -1;
    }

    dbg_ptf (debug_flg, "dev id:%s\n", client_id);
    dbg_ptf (debug_flg, "ip:%s, port:%d\n", server_ip, server_port);
    memset(&ts, 0, sizeof(ts));
    memset(&s_addr, 0, sizeof(s_addr));
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd <= 0){
        printf ("%s:%d socket error\n", __func__, __LINE__);
        return -1;
    }
    s_addr.sin_family= AF_INET;
    s_addr.sin_port = htons(server_port);
    s_addr.sin_addr.s_addr = inet_addr(server_ip);

    ret = connect (sockfd, (struct sockaddr*)&s_addr, sizeof(s_addr));
    if (ret != 0 )
    {
        perror("connect error");
        return -1;
    }
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
    
    memset(&login, 0, sizeof(login));
    strncpy(login.sync, TELPROXY_SYN, sizeof(TELPROXY_SYN));
    login.type = htonl(client_type);
    strncpy(login.id, client_id, strlen(client_id));

    ret = send(sockfd, &login, sizeof(login), 0);
    dbg_ptf (debug_flg, "send len=%lu and real=%d\n", sizeof(login), ret);

    atexit(exit_callback);
    keyboard_input_to_peer();
    while (1)
    {
        recvnum = recv (sockfd, buf, sizeof(buf)-1, 0);
        if (recvnum <= 0)
        {
            printf ("%s:%d the connect is unreachable\n", __func__, __LINE__);
            break;
        }
        buf[recvnum] = '\0';
        if (telnetok== 0 && strstr(buf, TELPROXY_OK) != NULL){
            telnetok = 1;
            continue;
        }

        for ( i = 0; i < recvnum; i++)
        {
            ret = telnet_iac_parse(&ts, buf[i]);
            if (ret == ECHO_DISABLE){
                echoMode = SelfEchoDisable;
            }
            else if(ret == ECHO_ENABLE){
                echoMode = SelfEchoEnable;
            }

            if ((buf[i] >=0x20 && buf[i]<= 0x7e)
            || (buf[i] == 0x9) || (buf[i] == 0x0a)){

                if (telnetok == 1){
                    putchar(buf[i]);
                }
            }
        }
        fflush(stdout);
    }
    exit(0);
}


static void exit_callback(void)
{
    if (sockfd >= 0)
    {
        close (sockfd);
    }

    int term_fd =tb.term_fd;
    if (term_fd >= 0)
    {
        term_enable_echo(term_fd, NULL);
        term_enable_sig(term_fd, NULL);
        close (term_fd);
    }
    printf ("%s:%d exit callback\n", __func__, __LINE__);
}


static int parse_dev_id(char* src, char* dst, int len)
{
    if (!src || !dst || len <= 0)
    {
        printf ("%s:%d param err\n", __func__, __LINE__);
        return -1;
    }
    memset(dst, 0, len);

    int ret = 0;
    char* format = NULL;
    char* def_mac = "11:22:33:44:55:66";
    int mac[6]; /* int is very important */

    if(strlen(src) == strlen(def_mac))
    {
        format = "%02x:%02x:%02x:%02x:%02x:%02x";
    }
    else if (strlen(src) == strlen(def_mac)-5)
    {
        format = "%02x%02x%02x%02x%02x%02x";
    }

    if(format)
    {
        ret = sscanf(src, format, mac, mac+1, mac+2, mac+3, mac+4, mac+5);
        if (ret != 6){
            printf ("%s:%d mac format is error\n", __func__, __LINE__);
            snprintf(dst, len, "%s", src);
        }
        else{
            
            snprintf(dst, len, "%02x%02x%02x%02x%02x%02x",
                    0xff & mac[0], 0xff & mac[1], 0xff & mac[2],
                    0xff & mac[3], 0xff & mac[4], 0xff & mac[5]);
        }
    }
    else
    {
        printf ("%s:%d not mac format\n", __func__, __LINE__);
        snprintf(dst, len, "%s", src);
    }
    //printf ("%s:%d id src:%s  dst:%s\n", __func__, __LINE__, src, dst);
    return 0;
}


static void* keyboard_input_to_peer_thread(void* argc)
{
    int ret = 0;
    int term_fd;
    int socket;
    char cc;

    TermLine line;

    socket = sockfd;
    if (socket < 0)
    {
        printf ("%s:%d param err\n", __func__, __LINE__); 
        exit(-1);
    }

    ret = term_open(&tb);
    if (ret != 0)
    {
        printf ("%s:%d term_open err\n", __func__, __LINE__);
        exit(-1);
    }
	term_history_init(&hb);
    term_fd = tb.term_fd;
    ret = term_disable_echo(term_fd, NULL);
    if (ret != 0)
    {
        printf ("%s:%d term_disable_echo err\n", __func__, __LINE__);
        exit(-1);
    }
    term_disable_sig(term_fd, NULL);
    while(1)
    {
        ret = read(term_fd, &cc, 1);
        if (ret != 1)
        {
            continue;
        }
        line.data = NULL;
        ret = term_echo_self(&tb, &hb, cc, echoMode, &line);
        if(ret == 0 &&line.data){
            ret = term_self_cmd_exit(line.data, line.size);
            if (ret == 1){
                exit(0);
            }
            ret = write(socket, line.data, line.size);
            if (ret < 0){
                printf ("%s:%d socket write err\n", __func__, __LINE__);
                exit(-1);
            }
        }
    }
    pthread_exit(NULL);
}

static void keyboard_input_to_peer(void)
{
    pthread_t tid;
    int ret = pthread_create (&tid, NULL,  keyboard_input_to_peer_thread, NULL);
    if (ret != 0)
    {
        printf("%s:%d create thread failed\n", __func__, __LINE__);
    }
    return;
}


static int http_url_to_ip(char* url, struct in_addr* addr)
{
    char str [256];

    memset (str, 0, sizeof(str));
	char* host = strstr (url, "http://");
	char* end = NULL;
	if (host)
	{
		host += strlen("http://");
	}
	else if (host = strstr(url, "https://"))
	{
		host += strlen("https://");
	}
	else{
		snprintf(str, sizeof(str),"%s", "192.168.99.117");
	}

	if (host)
	{
		if (!(end = strstr(host, ":")) && !(end = strstr(host, "/"))){
			snprintf(str, sizeof(str),"%s", "192.168.99.117");
		}
	}

	if (end)
	{
		*end= '\0';
	}
	else{
		host = str;
	}

	printf("the remote server host:%s\n", host);
    return 0;
}
