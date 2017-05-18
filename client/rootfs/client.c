#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<time.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include<sys/errno.h>
#include<errno.h>

#include"msg_helper.h"
int delete_queue(int qid);

int main(int argc,char **argv)
{
	int msgqid, msgqid1 , rc;
	struct msg_buf msg,msg1;


	msg.mtype = 1;
	while((fgets(msg.mtext, 256, stdin)) != NULL){
		 
		msgqid = msgget(5566, MSGPERM|IPC_CREAT);
        	if (msgqid < 0) {
                	perror(strerror(errno));
                	printf("failed to create message queue with msgqid = %d\n", msgqid);
                	return 1;
        	}

		rc = msgsnd(msgqid, &msg, sizeof(msg.mtext), 0);
		if (rc < 0) {
			perror( strerror(errno) );
			printf("msgsnd failed, rc = %d\n", rc);
			return 1;
		}		
		
               
		msgqid1 = msgget(7788, MSGPERM|IPC_CREAT);
        	if (msgqid1 < 0) {
                	perror(strerror(errno));
                	printf("failed to create message queue with msgqid = %d\n", msgqid1);
                	return 1;
        	}
	
		rc = msgrcv(msgqid1, &msg1, sizeof(msg1.mtext), 0, 0);
	        if (rc < 0) {
                	perror( strerror(errno) );
                	printf("msgrcv failed, rc=%d\n", rc);
                	return 1;
        	}
        	printf("%s",msg1.mtext);
		delete_queue(7788);
	}
	
	return 0;
}

int delete_queue(int qid){
        int msgqid, rc;
//        printf("delete queue\n");
        msgqid = msgget(qid, MSGPERM|IPC_CREAT);
        rc = msgctl(msgqid, IPC_RMID, NULL);
        if (rc < 0) {
                perror( strerror(errno) );
                printf("msgctl (return queue) failed, rc=%d\n", rc);
                return 1;
        }
        return 0;
}

