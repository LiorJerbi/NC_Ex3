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
#define MSG_SIZE 600517

/*
TCP Sender
*/
int main(){

    //file handle.
    FILE *fp;
    fp = fopen("Datafile.txt","r");  //the file we want to send.
    if(fp== NULL){
        perror("failed open file\n");
        return -1;
    }
    fseek(fp,0L,SEEK_END);
    long flen = ftell(fp);
    rewind(fp);
    size_t hflen = flen/2,l1,l2;
    char *msg1 = (char*)malloc(sizeof(char)*(hflen)); 
    char *msg2 = (char*)malloc(sizeof(char)*(hflen));
    l1 = fread(msg1,sizeof(char),hflen,fp);    //sepreating the file to 2 diffrent messages.
    fseek(fp,hflen,SEEK_SET);       //sets file pointer to middle of the file.
    l2 =fread(msg2,sizeof(char),hflen,fp);
    fclose(fp);
    
    
    char loop = '0';
    char Ack[] = "0100 0001 0011"; // Acknowledge agreed upon.
    int byteSent=0,byteRecv=0,aLen = strlen(Ack);
   
         
    
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
    char buff[BUFF_SIZE]={0}; 
    char buffcc[256]={0};
    socklen_t buflen= sizeof(buffcc);
    
    while(loop!='Y'&& loop!='y'){       
        printf("Setting CC Algo to cubic\n");
        strcpy(buffcc,"cubic");   //Changing the CC algo to "cubic"
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

        //Sending 1st half of the file in "cubic" CC algo
       
        while(1){ //if the file didn't fully arrived resend.
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
   /*         if(byteSent<l1){
                printf("message didnt fully arrived  %d bytes arrived,sending again...\n",byteSent);
                byteSent = send(sender_sockfd,msg1,l1,0);
            }*/
        }
        byteSent = 0;
        printf("Sender waiting for Ack...\n");
        //waiting to get Acknowledge from reciver.
        //memset(buff,0,sizeof(buff));
        byteRecv = recv(sender_sockfd,buff,sizeof(buff),0);
        while(1){
            
            printf("Ack: %d, Byte: %d,Sizeifbuff: %ld, ValueOfBuff: %s \n",aLen,byteRecv,sizeof(buff),buff);
            if(strcmp(Ack,buff) == 0){
                printf("Ack recived, sending second part of msg...\n"); //recived Ack from reciver continue.
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
            if(byteRecv< sizeof(buff)){
                printf("Ack not fully arrived, try again\n");
               // byteRecv += recv(sender_sockfd,buff,sizeof(buff),0);
            }
            byteRecv = recv(sender_sockfd,buff,sizeof(buff),0);
        }
        memset(buff,0,BUFF_SIZE);
        byteRecv =0;

        //Changing the CC algo
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
        while(1){ //if the file didn't fully arrived resend.
            if(byteSent==l2){
                printf("second part of message sent successfully!\n");
                break;
            }
            if(byteSent==-1){
                perror("send() error!\n");
                close(sender_sockfd);
            return -1;
            }
/*            if(byteSent==0){
                printf("Peer closed TCP connection socket.\n");
                return -1;
            }*/
            if(byteSent<l2){
                printf("message didnt fully arrived sending again...\n");
                byteSent = send(sender_sockfd,msg2,l2,0);
            }
        }
        printf("message fully sent!\nPrepairing to send again, if you want to stop press Y or y else any other key:\n");
        scanf("%c",&loop);
        //giving the user an option to send the message again.
        //scanf("message fully sent!\nPrepairing to send again, if you want to stop press Y or y else any other key:\n%c",&loop);
        
        //Continue communication with receiver
        if(loop!='Y' && loop!='y'){
            char ctumsg[] = "Continue connection";
            int lenEx = strlen(ctumsg)+1;
            byteSent=0;
            while((byteSent=send(sender_sockfd,ctumsg,lenEx,0))<lenEx){ //until the message is fully sent.
                if(byteSent==-1){
                    perror("continue send() error!\n");
                    return -1;
                }
/*                if(byteSent==0){
                    printf("Peer closed TCP connection socket.\n");
                    return -1;
                }*/
            } 

        }
    }
    //Finish communication with receiver.
    char exitmsg[] = "Closeing connection";
    int lenEx = strlen(exitmsg)+1;
    byteSent=0;
    while((byteSent=send(sender_sockfd,exitmsg,lenEx,0))<lenEx){ //until the message is fully sent.
        if(byteSent==-1){
            perror("exit send() error!\n");
        return -1;
        }
        if(byteSent==0){
            printf("Peer closed TCP connection socket.\n");
            return -1;
        }
    }
    printf("Exit message sent successfully!\n");
    close(sender_sockfd);  //closing the socket.



free(msg1);
free(msg2);
return -1;
}