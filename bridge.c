#define _GNU_SOURCE
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<sched.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/msg.h>
#include<sys/types.h>
#include<errno.h>
#include <sys/wait.h>
#include"msg_helper.h"
#include <sys/inotify.h>
#include <limits.h>

#define N_NS 3
#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int delete_queue(int qid);
int ipc_rcv();
int ipc_send(char* filename);
int mnt_rcv();
int mnt_send(char* wordstr);

int ns[N_NS] = {CLONE_NEWNET, CLONE_NEWNS, CLONE_NEWIPC};

int main(int argc, char *argv[])
{
	//read container id
  	FILE *file1 = fopen ("/tmp/ipc_client.pid", "r" );
	FILE *file2 = fopen ("/tmp/mnt_server.pid", "r" );
	
	char clientID[10];
	char serverID[10];
	if(fgets(clientID,sizeof(clientID),file1)== NULL){
		printf("Cannot get clientID.\n");
		return 1;
	}
	if(fgets(serverID,sizeof(serverID),file2)== NULL){
                printf("Cannot get serverID.\n");
                return 1;
        }

	
	char ipc_cmd[20]= "/proc/";
	strcat(ipc_cmd,clientID);
	strcat(ipc_cmd,"/ns/ipc"); 

	char mnt_cmd[20]= "/proc/";
        strcat(mnt_cmd,serverID);
        strcat(mnt_cmd,"/ns/mnt");
	
	
	pid_t  pid;
        int    status;
	char wordstr[256];
	
	//setns ipc
        if(setns(open(ipc_cmd, O_RDONLY), ns[2])){
        	printf("setns fails\n");
        	return 1;
        }
	//execute rcv
        printf("enter %s...\n", ipc_cmd);


	
	while(1){
			
		if(ipc_rcv() == -1){
			printf("ipc_rcv error\n");
		}
		
		delete_queue(5566);

		FILE *fin = fopen("/tmp/message","r");
		fgets(wordstr,256,fin);	
		
     		
		if ((pid = fork()) < 0) {     /* fork a child process           */
          		printf("*** ERROR: forking child process failed\n");
          		exit(1);
     		}
     		else if (pid == 0) {          /* for the child process:         */
                	
			//setns mnt
		        if(setns(open(mnt_cmd, O_RDONLY), ns[1])){
                		printf("setns mnt fails\n");
                		return 1;
        		}
        		printf("enter %s...\n", mnt_cmd);

			//execute
//                	chdir("/mnt");

                	if(mnt_send(wordstr) == -1) printf("mnt_send error\n");
                	if(mnt_rcv() == -1) printf("mnt_rcv error\n");
			
     		}
     		else {                                  /* for the parent:      */
          		while (wait(&status) != pid);      /* wait for completion  */
               
     		}		
		
	
		//send msg back
                //execute rcv
                if(ipc_send("server/rootfs/output") == -1){
                	perror("execve error\n");
                }
			
		if(!strcmp(wordstr,"exit\n")) break;
	}
	
	return 0;
}


int delete_queue(int qid){
	int msgqid, rc;
        msgqid = msgget(qid, MSGPERM|IPC_CREAT);
	printf("delete_queue\n");
        rc = msgctl(msgqid, IPC_RMID, NULL);
        if (rc < 0) {
                perror( strerror(errno) );
                printf("msgctl (return queue) failed, rc=%d\n", rc);
                return 1;
        }
	return 0;
}

int ipc_rcv(){
	int msgqid, rc;
        struct msg_buf msg;

        msgqid = msgget(5566, MSGPERM|IPC_CREAT);
        if (msgqid < 0) {
                perror(strerror(errno));
                printf("failed to create message queue with msgqid = %d\n", msgqid);
                return -1;
        }

        FILE *fout;
        fout = fopen("/tmp/message","w");
        if((rc = msgrcv(msgqid, &msg, sizeof(msg.mtext), 0, 0)) >= 0){
                printf("Recv from client: %s",msg.mtext);
                fputs(msg.mtext,fout);
        }
        fclose(fout);
        if (rc < 0) {
                perror( strerror(errno) );
                printf("msgrcv failed, rc=%d\n", rc);
                return -1;
        }
	
	return 0;
}

int ipc_send(char* filename){
	int msgqid, rc;
        struct msg_buf msg;

        msgqid = msgget(7788, MSGPERM|IPC_CREAT);
        if (msgqid < 0) {
                perror(strerror(errno));
                printf("failed to create message queue with msgqid = %d\n", msgqid);
                return 1;
        }

        msg.mtype = 2;

        FILE *fin = fopen(filename,"r");
        if((fgets(msg.mtext, 256, fin)) != NULL){
                printf("Send to client: %s",msg.mtext);
                rc = msgsnd(msgqid, &msg, sizeof(msg.mtext), 0);
                if (rc < 0) {
                        perror( strerror(errno) );
                        printf("msgsnd failed, rc = %d\n", rc);
                        return -1;
                }
        }

        return 0;
}

int mnt_send(char* wordstr){
	
	int inotifyFd, wd, j;
        char buf[BUF_LEN] __attribute__ ((aligned(8)));
        ssize_t numRead;
        char *p;
        struct inotify_event *event;

        inotifyFd = inotify_init();
        if (inotifyFd == -1) {
                perror(strerror(errno));
                printf("inotifyFd\n");
                return -1;
        }

        wd = inotify_add_watch(inotifyFd, getcwd(NULL,0), IN_DELETE);
        if (wd == -1) {
                perror(strerror(errno));
                printf("inotify_add_watch\n");
                return -1;
        }

        FILE *fp = fopen("message", "w");
	
	int i;
        for(i = 0;i < strlen(wordstr);i++) fputc(wordstr[i],fp);
	printf("Send to server: %s",wordstr);
        fclose(fp);
	
	while(1) {
                numRead = read(inotifyFd, buf, BUF_LEN);
                if (numRead <= 0) {
                        perror(strerror(errno));
                        printf("read() from inotify fd returned %d!", numRead);
                        return 1;
                }

                for (p = buf; p < buf + numRead; ) {
                        event = (struct inotify_event *) p;

                        if((event->mask & IN_DELETE) && !strcmp(event->name, "message")){
//				close(inotifyFd);
//        			close(wd);
				return 0;
			}

                        p += sizeof(struct inotify_event) + event->len;
                }
        }
	
        return 0;
}

int mnt_rcv(){
	int inotifyFd, wd, j;
        char buf[BUF_LEN] __attribute__ ((aligned(8)));
        ssize_t numRead;
        char *p;
        struct inotify_event *event;

        inotifyFd = inotify_init();
        if (inotifyFd == -1) {
                perror(strerror(errno));
                printf("inotifyFd\n");
                return -1;
        }
        wd = inotify_add_watch(inotifyFd, getcwd(NULL, 0), IN_CLOSE_WRITE);
        if (wd == -1) {
                perror(strerror(errno));
                printf("inotify_add_watch\n");
                return -1;
        }
       	printf("Recv from server\n"); 
	while(1) {
	
                numRead = read(inotifyFd, buf, BUF_LEN);
	
                if (numRead <= 0) {
                        perror(strerror(errno));
                        printf("read() from inotify fd returned %d!", numRead);
                        return -1;
                }

                for (p = buf; p < buf + numRead; ) {
                        event = (struct inotify_event *) p;
                        
			if((event->mask & IN_CLOSE_WRITE) && !strcmp(event->name, "message1")){
                                system("cat message1 > output");
                                system("rm -f message1");
        			return 0;
                        }

                        p += sizeof(struct inotify_event) + event->len;
                }
        }
        return 0;
}
