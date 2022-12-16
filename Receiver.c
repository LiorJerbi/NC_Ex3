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

#define RECEIVER_PORT 5093 //the port that the reciver listens to.
#define BUFF_SIZE 1024
#define MSG_SIZE 600517

/*
TCP Receiver
*/
int main(){
    
    static int Flag = 1;
    char *Ack = (char *)(9493^8454); // Acknowledge agreed upon.
    char exitmsg[] = "Closeing connection";
    char ctumsg[] = "Continue connection";
    char buff[BUFF_SIZE]={0};      
    
    //open listening socket for reciver
    int listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(listening_socket==-1){
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
    if((setsockopt(listening_socket,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT , &yes, sizeof(yes))) == -1){
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
    if(listen(listening_socket, 7)== -1){ //maximum requests in queue is 7.
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
    if(senderSocket==-1){
        perror("accept() failed\n");
        close(senderSocket);
        return -1;
    }
    printf("A new connection accepted!\n");
    
    while(Flag){ //Running the loop until sender close socket.
        clock_t staTime,finTime;
        int tryCount1=0,rcved=0,rcveduntil=0;
        printf("Receiver ready\n");
        staTime = clock(); //a variable for holding the time to calculate
        while(1){   //loop for receiving first half of message from sender.
            rcved = recv(senderSocket,buff,sizeof(buff),0);
            rcveduntil+=rcved;
            tryCount1++;       //Attempt count
            printf("received now: %d, received total: %d.\n",rcved,rcveduntil);
            if(rcveduntil==MSG_SIZE){
                printf("first part arrived completely\n");
                break;
            }
        }
        finTime = clock();
        double timeSum1 = (double)(finTime-staTime);  //calculating seconds needed for messag to arrive.
        printf("Recieved bytes total:%d in %f seconds.\n",rcveduntil,timeSum1);
        if(rcved==-1){
            perror("recv() error.\n");
            return -1;
        }
        if(rcved==0){
            printf("Peer closed the TCP connection socket.\n");
            close(senderSocket);
            Flag=0;
            return -1;
        }

        printf("New Average time for receiving data: %f\n",timeSum1/tryCount1);
        printf("Total time for arrival:%f\nin %d attempts.\n",timeSum1,tryCount1);

        //Sending back authentication to the sender.
        
        int byteSent=0;
        size_t aLen =sizeof(Ack);
        byteSent=send(senderSocket,Ack,aLen,0);
        while(1){
            if(byteSent==-1){
                perror("Acknowledgment send() failed.");
                return -1;
            }
            if(byteSent==0){
                printf("Peer closed the TCP connection socket.\n");
                close(senderSocket);
                Flag=0;
                return -1;
            }
            if(byteSent<=sizeof(Ack)){
                printf("Ack not fully sent.\n");
                byteSent+=send(senderSocket,Ack,sizeof(Ack),0);
            }
        }
        printf("Acknowledgment arrived successfully, waiting for data.\n");

        int tryCount2=0;
        rcveduntil=0,rcved=0;
        staTime = clock(); //a variable for holding the start time to calculate
        while(1){  //loop for receiving second half of message from sender.(changed CC)
            rcved = recv(senderSocket,buff,sizeof(buff),0);
            rcveduntil+=rcved;
            tryCount2++;       //Attempt count
            printf("received now: %d, received total: %d.\n",rcved,rcveduntil);
            if(rcveduntil==MSG_SIZE){
                printf("second part arrived completely\n");
                break;
            }
        }
        finTime = clock();
        double timeSum2 = (double)(finTime-staTime);  //calculating seconds needed for messag to arrive.
            
        printf("Recieved bytes total:%d in %f seconds.\n",rcveduntil,timeSum2);
        
        if(rcved==-1){
            perror("recv() error.\n");
            return -1;
        }
/*        if(rcved==0){
            printf("Peer closed the TCP connection socket.\n");
            close(senderSocket);
            Flag=0;
            return -1;
            }
*/        
        printf("New Average time for receiving data: %f\n",timeSum2/tryCount2);
        printf("Total time for arrival:%f\nin %d attempts.\n",timeSum2,tryCount2);

        //checking for exit\continue msg.
        memset(buff,0,sizeof(buff));
        int byteRec = 0;
        while(1){
            byteRec = recv(senderSocket,buff,sizeof(buff),0);
            if(byteRec<0){
                perror("recv() error.\n");
                close(senderSocket);
                return -1;           
            }
            if(strcmp(buff,exitmsg)==0){
                printf("all times:\n");//need to print out the times
                printf("Averae time took to send the 1st part: %f\nAverae time took to send the 2nd part: %f \n",(timeSum1/tryCount1),(timeSum2/tryCount2));//calculate avg time of each part of the msg
                printf("Average time for sending the complete file: %f\n",((timeSum1+timeSum2)/(tryCount1+tryCount2)));//need to print avg time of all file
                close(senderSocket);
                Flag=0;
                break;
            }
            if(strcmp(buff,ctumsg)==0){
                break;
            }  
        }

    }
close(listening_socket);
return 0;
}