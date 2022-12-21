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
#include <time.h>
#include <sys/time.h>

#define RECEIVER_PORT 5093 //the port that the reciver listens to.
#define BUFF_SIZE 32768  //receiver buffer size
#define MSG_SIZE 601418

/*
TCP Receiver
*/
int main(){
   
    static int Flag = 1;
    char Ack[] = "0000 0100 0001 0011"; //bitwise XOR between 4 last id's numbers.(9493^8454)
    int aLen =strlen(Ack)+1;
    char exitmsg[] = "Closeing connection";
    char ctumsg[] = "Continue connection";
    double timeData[2][1000] = {0}; //Dataset to contain all the times took for receiver.
    struct timeval staTime,finTime;
    int tryCount1=0,tryCount2=0,byteSent=0,byteRec=0,i=0,steps=0;    
    long rcveduntil1=0,rcveduntil2=0,rcved1=0,rcved2=0;
    double timerAvg1 =0, timerAvg2 =0, secs=0, usecs=0;
    
    //open listening socket for reciver
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(listening_socket == -1){
        perror("Could not create listening socket.\n");
        close(listening_socket);
        return -1;
    }
    
    //attach the socket to the reciver details.
    struct sockaddr_in receiverAddress;
    memset(&receiverAddress,0,sizeof(receiverAddress));
    receiverAddress.sin_family = AF_INET;
    receiverAddress.sin_addr.s_addr = INADDR_ANY;
    receiverAddress.sin_port = htons(RECEIVER_PORT);
    
    //Reuse the address and port.(prevents errors such as "address already in use")
    int yes = 1;
    if((setsockopt(listening_socket,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT , &yes, sizeof(yes))) < 0){
        perror("setsockopt() failed\n");
        close(listening_socket);
        exit(1);
    }
     //binding the socket to the port with any IP connecting to port.
    if(bind(listening_socket,(struct sockaddr *)&receiverAddress, sizeof(receiverAddress)) == -1){
        perror("Binding failed.\n");
        close(listening_socket);
        return -1;
    }
    printf("Bind succeeded!\n");

    //Make the socket listening connection requests
    //maximum requests in queue is 7.
    if(listen(listening_socket, 7000)== -1){    
        perror("listen() failed!\n");
        close(listening_socket);
        return -1;
    }
    printf("Waiting for TCP connection requests...\n");

    //initializing new connection socket.
    struct sockaddr_in senderAddress;
    socklen_t senderAddressLen = sizeof(senderAddress);
    memset(&senderAddress,0,sizeof(senderAddress));
    senderAddressLen = sizeof(senderAddress);
    
    //accepting connection request and binding the sender details.
    int senderSocket= accept(listening_socket,(struct sockaddr *)&senderAddress, &senderAddressLen);
    if(senderSocket == -1){
        perror("accept() failed\n");
        close(senderSocket);
        return -1;
    }
    printf("A new connection accepted!\n");

    while(Flag){

        char buff[BUFF_SIZE]={0};       //initializing buffer
        printf("Receiver ready\n");
        rcveduntil1 = 0 ,tryCount1 = 0;
        gettimeofday(&staTime,NULL);    //catching the start time of the receival
        while(rcveduntil1<MSG_SIZE){    //loop until receiving first part of message in cubic
            rcved1 = recv(senderSocket,buff,sizeof(buff),0);
            rcveduntil1 += rcved1;
            tryCount1++;                    
            if(rcveduntil1 == MSG_SIZE){   
                printf("first part arrived completely\n");
                break;
            }
            if(rcved1 == -1){
              perror("recv() error.\n");
                return -1;
            }
            if(rcved1 == 0){
                printf("Peer closed the TCP connection socket.\n");
                close(senderSocket);
                Flag=0;
                return -1;
            }
        }
        gettimeofday(&finTime,NULL);        //catching the finish time of the receival
        secs = (double)(finTime.tv_sec-staTime.tv_sec);
        usecs = (double)(finTime.tv_usec-staTime.tv_usec);
        double timeSum1 = (double)((secs*1000)+(usecs/1000));  //calculating time needed for messag to arrive.
        timeData[0][i]=timeSum1;                               //filling the Data set with new Average time
        printf("Average time for arrival: %f. in %d Segments.\n",timeSum1,tryCount1);


        byteSent = 0;
        //Sending Acknowledgment agreed upon for the arrival of the 1st part.
        while((byteSent = send(senderSocket,Ack,aLen,0))>0){
            if(byteSent == -1){
                perror("Acknowledgment send() failed.");
                return -1;
            }
            if(byteSent == 0){
                printf("Peer closed the TCP connection socket.\n");
                close(senderSocket);
                Flag=0;
                return -1;
            }
            if(byteSent<aLen){
                printf("Ack not fully sent.\n");
            }
            if(byteSent==aLen){
                printf("Acknowledgment sent successfully, waiting for data.\n");
                break;
            }
        }
        rcveduntil2 = 0,tryCount2=0;
        gettimeofday(&staTime,NULL);        //catching the start time of the receival        
        while(rcveduntil2<MSG_SIZE){        //loop until receiving second half of message in reno.(changed CC)
            rcved2 = recv(senderSocket,buff,sizeof(buff),0);
            rcveduntil2+=rcved2;
            tryCount2++;       
            if(rcveduntil2==MSG_SIZE){
                printf("second part arrived completely\n");
                break;
            }
            if(rcved2==-1){
                perror("recv() error.\n");
                return -1;
            }
            if(rcved2==0){
                printf("Peer closed the TCP connection socket.\n");
                close(senderSocket);
                Flag=0;
                return -1;
            }      
        }
        gettimeofday(&finTime,NULL);    //catching the finish time of the receival
        secs = (double)(finTime.tv_sec-staTime.tv_sec);
        usecs = (double)(finTime.tv_usec-staTime.tv_usec);
        double timeSum2 = (double)((secs*1000)+(usecs/1000));  //calculating time needed for messag to arrive.
        timeData[1][i]=timeSum2;                               //filling the Data set with new Average time
        i++;
        printf("Average time for arrival:% f. in %d attempts.\n",timeSum2,tryCount2);


        byteRec = 0;
        printf("The receiver wait for sender decision \n");
        
        //Waiting for Exit\Continue message
        while(1){
            byteRec = recv(senderSocket,buff,sizeof(buff),0);
            if(byteRec<0){
                perror("recv() error.\n");
                close(senderSocket);
                return -1;           
            }
            if(byteRec==0){
                printf("Receiver socker closed.\n");
                close(senderSocket);
                Flag=0;
                break;;           
            }
            if(strcmp(buff,exitmsg) == 0){      //stop the receival of data. print and calculate the times.
                printf("all times:\n");
                steps=i;
                while(i>0){
                i--;
                printf("Round num: %d. Cubic: %f. Reno: %f.\n",i+1,timeData[0][i],timeData[1][i]);
                timerAvg1+=timeData[0][i];
                timerAvg2+=timeData[1][i];
                
                }
                printf("Average time for cubic: %f\n",timerAvg1/steps);
                printf("Average time for reno: %f\n",timerAvg2/steps);
                printf("Average time for all the file: %f\n",(timerAvg1+timerAvg2)/(steps*2));
                close(senderSocket);
                Flag=0;
                break;
            }
            if(strcmp(buff,ctumsg) == 0){       //continue for another receival
                printf("Receiver continue..\n");
                break;
            }  
        }
    }
close(listening_socket);        //closing listening socket.
return 0;
}