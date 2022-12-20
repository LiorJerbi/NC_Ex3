#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#define RECEIVER_PORT 5093      
#define SENDER_IP "127.0.0.1"  
#define BUFF_SIZE 32768
#define MSG_SIZE 601418

/*
TCP Sender
*/
int main(){

    //file handle.
    FILE *fp;
    fp = fopen("Datafile.txt","r");     //the file we want to send
    if(fp== NULL){
        perror("failed open file\n");
        return -1;
    }
    fseek(fp,0L,SEEK_END);
    long flen = ftell(fp);              //holds the file size
    rewind(fp);
    size_t hflen = flen/2,l1,l2;
    char *msg1 = (char*)malloc(sizeof(char)*(hflen));     
    char *msg2 = (char*)malloc(sizeof(char)*(hflen));
    
    l1 = fread(msg1,sizeof(char),hflen,fp);    //sepreating the file to 2 diffrent messages.
    fseek(fp,hflen,SEEK_SET);       //sets file pointer to middle of the file.
    l2 =fread(msg2,sizeof(char),hflen,fp);
    fclose(fp);                     
    char loop = '0';
    char Ack[] = "0000 0100 0001 0011"; //bitwise XOR between 4 last id's numbers.(9493^8454)
    int byteSent=0,byteRecv=0,byteRecvUntil =0,aLen =strlen(Ack)+1;
   
         
    
    //open sender socket.
    int sender_sockfd = socket(AF_INET,SOCK_STREAM,0);
    
    if(sender_sockfd == -1){
        perror("Could not create socket.\n");
        return -1;
    }
    
    //attaching socket to reciver port.
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress,0,sizeof(receiverAddress)); 
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_port = htons(RECEIVER_PORT);
    int ra = inet_pton(AF_INET,(const char*)SENDER_IP,&receiverAddress.sin_addr);
    if(ra<=0){
        perror("inet_pton() failed!\n");
        return -1;
    }

    //Make a connection to reciver.
    if(connect(sender_sockfd, (struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) == -1){
        perror("Connection failed!\n");
        close(sender_sockfd);
        return -1;
    }
     
    printf("Connection succeeded!\n");
   
    //main loop, runs until client stops.
    while(loop!='Y'&& loop!='y'){   
        char buff[BUFF_SIZE]={0};       //initializing sender buffer
        char buffcc[256]={0};           //buffer for CC switch.
        socklen_t buflen= sizeof(buffcc);

        printf("Setting CC Algo to cubic\n");
        strcpy(buffcc,"cubic");         //Changing the CC algo to "cubic"
        if(setsockopt(sender_sockfd,IPPROTO_TCP,TCP_CONGESTION,buffcc,buflen)!=0){
            perror("setsockopt() failed\n");
            close(sender_sockfd);
            return -1;
        }
        buflen = sizeof(buffcc);
        if(getsockopt(sender_sockfd,IPPROTO_TCP,TCP_CONGESTION,buffcc,&buflen)!=0){
            perror("getsockopt() failed\n");
            close(sender_sockfd);
            return -1;
        }

        byteSent =0;

        //Sending 1st half of the file in "cubic" CC algo
        while((byteSent = send(sender_sockfd,msg1,l1,0))>0){ 
            byteSent = send(sender_sockfd,msg1,l1,0);
            if(byteSent==l1){
                printf("first part of message sent successfully! with %d Bytes.\n",byteSent);
                break;
            }
            if(byteSent==-1){
                perror("send() error!\n");
                return -1;
            }
            if(byteSent==0){ 
                printf("Peer closed the TCP connection socket.\n");
                return -1;
            }
            if(byteSent<l1){
                printf("message didnt fully arrived  %d bytes arrived,sending again...\n",byteSent);
            }
        }

        
        byteRecvUntil =0;
        printf("Sender waiting for Ack...\n");
        //loop until getting Acknowledge from the receiver for receiving 1st half.
        while(byteRecvUntil<aLen){
            byteRecv = recv(sender_sockfd,buff,sizeof(buff),0);
            byteRecvUntil += byteRecv;
            if(strcmp(Ack,buff) == 0){      //comparing Ack sign agreed upon.
                printf("Ack recived, sending second part of msg...\n");
                break;
            }
            if(byteRecv==-1){
                perror("recv() error.\n");
                close(sender_sockfd);
                return -1;
            }
            if(byteRecv==0){
                printf("connection socket closed\n");
                close(sender_sockfd);
                return -1;
            }
            if(byteRecvUntil == strlen(Ack)+1){
                printf("Ack arrived completely\n");
                break;
            }
            
        }
        

        //Changing the CC algo to reno
        printf("Setting CC Algo to reno\n");
        strcpy(buffcc,"reno");
        buflen = sizeof(buffcc);
        if(setsockopt(sender_sockfd,IPPROTO_TCP,TCP_CONGESTION,buffcc,buflen)!=0){
            perror("setsockopt() failed\n");
            close(sender_sockfd);
            return -1;
        }
        buflen = sizeof(buff);
        if(getsockopt(sender_sockfd,IPPROTO_TCP,TCP_CONGESTION,buffcc,&buflen)!=0){
            perror("getsockopt() failed\n");
            close(sender_sockfd);
            return -1;
        }
        
        //Sending 2nd part of the file in "reno" CC algo.
        byteSent=0;
        byteSent = send(sender_sockfd,msg2,l2,0);
        while((byteSent = send(sender_sockfd,msg2,l2,0))>0){ //if the file didn't fully arrived resend.
            byteSent = send(sender_sockfd,msg2,l2,0);
            if(byteSent==l2){
                printf("second part of message sent successfully!\n");
                break;
            }
            if(byteSent==-1){
                perror("send() error!\n");
                close(sender_sockfd);
            return -1;
            }
            if(byteSent==0){
                printf("Peer closed TCP connection socket.\n");
                return -1;
            }
            if(byteSent<l2){
                printf("message didnt fully arrived sending again...\n");
                
            }
        }

        //Checking with the client if to Continue/Exit
        printf("message fully sent! Prepairing to send again, if you want to stop press Y or y else any other key:\n");
        scanf(" %c",&loop);
        
        //Continue communication with receiver
        if(loop!='Y' && loop!='y'){
            char ctumsg[] = "Continue connection";
            int lenEx = strlen(ctumsg)+1;
            byteSent=0;
            while(1){
                byteSent=send(sender_sockfd,ctumsg,lenEx,0);
                if(byteSent==-1){
                    perror("continue send() error!\n");
                    return -1;
                }
                if(byteSent==0){
                    printf("Peer closed TCP connection socket.\n");
                    return -1;
                }
                if(byteSent==lenEx){
                    printf("%s\n",ctumsg);
                    break;
                }
            } 

        }
    }
    //Finish communication with receiver.
    char exitmsg[] = "Closeing connection";
    int lenEx = strlen(exitmsg)+1;
    byteSent=0;
    while(1){
        byteSent=send(sender_sockfd,exitmsg,lenEx,0);
        if(byteSent==-1){
            perror("exit send() error!\n");
        return -1;
        }
        if(byteSent==0){
            printf("Peer closed TCP connection socket.\n");
            return -1;
        }
        if(byteSent==lenEx){
            printf("%s\n",exitmsg);
            break;
        }
    }
    printf("Exit message sent successfully!\n");
    close(sender_sockfd);  //closing the sender socket.



free(msg1);
free(msg2);
return -1;
}