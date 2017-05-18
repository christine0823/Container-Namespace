#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

int main(int argc, char *argv[])
{
	int inotifyFd, wd, j;
	char buf[BUF_LEN] __attribute__ ((aligned(8)));
	ssize_t numRead;
	char *p;
	struct inotify_event *event;

	inotifyFd = inotify_init();
	if (inotifyFd == -1) {
		perror(strerror(errno));
		printf("inotifyFd\n");
		return 1;
	}
	char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("Current working dir: %s\n", cwd);

	wd = inotify_add_watch(inotifyFd, cwd, IN_CLOSE_WRITE);
	if (wd == -1) {
		perror(strerror(errno));
		printf("inotify_add_watch\n");
		return 1;
	}

	char wordstr[256];
rcv:	memset(wordstr,0,256);

	while(1) {
		numRead = read(inotifyFd, buf, BUF_LEN);
		if (numRead <= 0) {
			perror(strerror(errno));
			printf("read() from inotify fd returned %d!", numRead);
			return 1;
		}
		
		for (p = buf; p < buf + numRead; ) {
			event = (struct inotify_event *) p;

			if((event->mask & IN_CLOSE_WRITE) && !strcmp(event->name, "message")){
				FILE *fp = fopen("message", "r");
				char ch;
				int i = 0;
				printf("Recv:");
				while((ch = fgetc(fp)) != '\n'){
					putchar(ch);
					wordstr[i++] = ch;
				}
				printf("\n");
				wordstr[i] = '\0';
				fclose(fp);
				system("rm -f message");
				goto send;
			}

			p += sizeof(struct inotify_event) + event->len;
		}
	}

send:
	printf("send %s\n",wordstr);	
	FILE *fout = fopen("message1", "w");
	int i;
        for(i = 0;i < strlen(wordstr);i++) fputc(wordstr[i],fout);

        fputc('\n', fout);
	fclose(fout);
	
	 while(1) {
                numRead = read(inotifyFd, buf, BUF_LEN);
                if (numRead <= 0) {
                        perror(strerror(errno));
                        printf("read() from inotify fd returned %d!", numRead);
                        return 1;
                }

                for (p = buf; p < buf + numRead; ) {
                        event = (struct inotify_event *) p;
                        if(!strcmp(event->name, "output")){
				printf("goto rcv\n");
                                goto rcv;
                        }

                        p += sizeof(struct inotify_event) + event->len;
                }
        }


end:
	close(wd);
	close(inotifyFd);
//	exit(EXIT_SUCCESS);
}
