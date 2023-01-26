/*
This code is from Yongyao Huang.
This program can communicate with other drones(clients) within 2 squares
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#define MaxHopcount 5
#define max_hosts 256 //max number of the hosts
#define TIMEOUT 5 //5 seconds

struct Memory {
    bool isEmpty;
    long time;
    int count;
    char path[5][5];
    int path_len;
    char To[10];
    char From[10];
    char msg_TYPE[10];
    char msg_id[10];
    char temp1[200];
};

int main(int argc, char *argv[]) {
    int this_port = atoi(argv[1]);
    struct sockaddr_in server_addr[max_hosts];  //max number of server is 30;
    char location[10];
    char File_String[1000];
    int index = -1; //the first line of config file will be <row> <col>
    int this_index = -1;
    struct sockaddr_in client_addr;  //client's address information
    socklen_t fromLength = sizeof (struct sockaddr);
    int row;
    int col;
    int this_loc;  //loc of this drone
    int this_row;  //x of this drone
    int this_col;  //y of this drone
    //msgID[i] is the next msg_id sending to port "port_array[i]".
    //last_ID[i] is the last msg_id received from port "port_array[i]".
    int msgID[max_hosts] = {0};  //keep track of sequence number sending to every port number, start with 1.
    int port_array[max_hosts] = {0};  //store port number in config file.
    int last_ID[max_hosts] = {0}; //keep track of last msg_id received from every port number.
    for (int i = 0; i < max_hosts; i++) {
        port_array[i] = -1;
        msgID[i] = 1;
        last_ID[i] = 0;
    }
    struct Memory memory[30];
    for (int i = 0; i < 30; ++i) {
        memory[i].isEmpty = true;
        memory[i].time = time((time_t*)NULL);
        memory[i].count = 5;
    }
    
    if (argc < 2) {
        printf("Need to input the port number.\n");
        printf("Usage is: drone7 <port number>\n");
        exit(1);
    }
    FILE* fp = fopen("config.txt", "r");
    if (fp == NULL) {
        printf("config file is not found.");
        exit(1);
    }
    
    while (index < max_hosts) {
        if (fgets(File_String, 1000, fp) == NULL) {
            break;
        }
        if (index == -1) { //scan the <row> <col>
            ++index;
            char* row_String = strtok(File_String, " \n");
            char* col_String = strtok(NULL, " \n");
            row = atoi(row_String);
            col = atoi(col_String);
            continue;
        }
        char* ip_String = strtok(File_String, " \n");
        if (ip_String == NULL){
            break;
        }
        char* port_String = strtok(NULL, " \n");
        char* location_String = strtok(NULL, " \n");
        int portnumber = atoi(port_String);
        if (portnumber == this_port) {
            this_index = index;
            strcat(location, location_String);
            this_loc = atoi(location);
            if (this_loc > row*col) {
                printf("the current drone's location is %d, and it's out of ranged.\n", this_loc);
                exit(1);
            }
            this_row = ceil(this_loc/(double)col);  //calculate the coordinate x
            this_col = (this_loc-1)%col + 1;    //calculate the coordinate y
            printf("Your coordinates are (%d,%d)\n", this_row, this_col);
        }
        if (portnumber == 0) {
            printf("Invalid port number.\n");
            continue;
        }
        
        
        server_addr[index].sin_port = htons(portnumber);  //portnumber
        server_addr[index].sin_family = AF_INET;   //IPv4
        server_addr[index].sin_addr.s_addr = inet_addr(ip_String);  //server's ip address
        if (server_addr[index].sin_addr.s_addr < 0) {
            printf("Invalid ip address.\n");
            continue;
        }
        port_array[index] = portnumber;
        index++;
    }
    if (this_index == -1) {
        printf("The input port number is not contained in config file.");
        exit(1);
    }
    
    int sock_rt = socket(AF_INET, SOCK_DGRAM, 0);  //create a socket
    if (sock_rt < 0 ) {
        printf("Failed to create a socket");
        exit(1);
    }
    
    int rc; //return value
    rc = bind(sock_rt, (struct sockaddr *) &server_addr[this_index], sizeof(server_addr[this_index]));  // bind the port to the socket
    char buf[100]; //message to be sent, no more than 100 bytes(enter key not included).
    
    fd_set socketFDS;
    int maxSD;
    printf("Please enter: <port> <message>.\n");
    
    while (1) {
        fflush(stdout);  //update?
        memset(buf, '\0', 100);   //clean the buf
        //printf("Input message no more than 100 bytes or input \"STOP\" to teminate:\n");
        
        FD_ZERO(&socketFDS);
        FD_SET(sock_rt, &socketFDS);
        FD_SET(0, &socketFDS);
        if (0 > sock_rt) {
            maxSD = 0;
        }else {
            maxSD = sock_rt;
        }
        
        struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;
        rc = select(maxSD+1, &socketFDS, NULL, NULL, &timeout);
        
        // resend messages
        long cur_time = time((time_t*)NULL);
        for (int n = 0; n < 30; ++n) {
            if (memory[n].isEmpty == true) continue;
            if (cur_time - memory[n].time >= TIMEOUT) {
                memory[n].time = cur_time;
                
                char temp2[200];
                sprintf(temp2, "%s", memory[n].temp1);
                char* Ver = strtok(temp2, " ");  //Protocol version
                strtok(NULL, " ");
                char* To = strtok(NULL, " ");
                char* From = strtok(NULL, " ");
                char* Hop = strtok(NULL, " ");
                char* msg_TYPE = strtok(NULL, " ");
                char* msg_id = strtok(NULL, " ");
                char* Route = strtok(NULL, " ");
                char* M = strtok(NULL, "");
                char temp1[200];
                sprintf(temp1, "%s %s %s %s %s %s %s %s %s", Ver, location, To, From, Hop, msg_TYPE, msg_id, Route, M);
//                printf("messages resent: %s\n", temp1);  //for debugging
                for (int i = 0; i < index; i++) {
                    bool IftoSend = true;
                    for (int j = 0; j < memory[n].path_len; j++) {  //to see if port_array[i] is already in the path. if yes, don't send the message to port_array[i]
                        if(port_array[i] == atoi(memory[n].path[j])) {
                            IftoSend = false;
                            break;
                        }
                    }
                    if (IftoSend == false) continue;
                    rc = sendto(sock_rt, temp1, strlen(temp1), 0, (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));  //send message
                    if (rc < 0) {
                        printf("Unable to send the message");
                        exit(1);
                    }
                }
                memory[n].count--;
                if (memory[n].count <= 0) memory[n].isEmpty = true;
            }
        }
        
        //receive from stdin
        if (FD_ISSET(0, &socketFDS)) {
            fgets(buf, 101, stdin);   //only at most the first 100 chars will be sent at a message
            if (buf[strlen(buf)-1] == '\n') {
                buf[strlen(buf)-1] = '\0'; //delete '\n' at the tail of buf
            }
            if (strlen(buf) == 0) {    //null string can not be sent, that is, only entering "\n" will not send any message
                continue;
            }
            if (strcmp(buf, "STOP") == 0) {
                printf("Stop sending message.\n");
                exit(1);
            }
            
            int Hop_number = MaxHopcount - 1;
            char* Ver = "6"; //Protocol version
            char* To = strtok(buf, " ");
            char* M = strtok(NULL, "");
            char temp_M[100];
            strcpy(temp_M, M);
            char* M1 = strtok(temp_M, " "); // to see if contains "MOV" command
            char* msg_TYPE;
            int seq; //msg_id of this port to target port
            if (strcmp(M1, "MOV") == 0) {
                msg_TYPE = "MOV";
                M = strtok(NULL, " "); // the location that the destport going to move to
                seq = -1;
            }else {
                msg_TYPE = "MSG";
                int To_number = atoi(To);
                for (int i = 0; i < index; i++) {
                    if (To_number == port_array[i]) {
                        seq = msgID[i];
                        break;
                    }
                }
            }

            char Route[100];
            sprintf(Route, "%d", this_port);
            char temp1[200]; //String to be sent
            //“<v> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>”
            sprintf(temp1, "%s %s %s %d %d %s %d %s %s", Ver, location, To, this_port, Hop_number, msg_TYPE, seq, Route, M);
            
            //store the message in the memory
            if (strcmp(msg_TYPE, "MSG") == 0) {
                for (int i = 0; i < 30; ++i) {
                    if (memory[i].isEmpty == false) continue;
                    memory[i].isEmpty = false;
                    memory[i].count = 5;
                    memory[i].time = time((time_t*)NULL);
                    
                    sprintf(memory[i].To, "%s", To);
                    sprintf(memory[i].From, "%d", this_port);
                    sprintf(memory[i].msg_id, "%d", seq);
                    sprintf(memory[i].msg_TYPE, "%s", msg_TYPE);
                    sprintf(memory[i].temp1, "%s", temp1);
                    
//                    printf("message stored: %s\n", temp1); //for debugging
                    
                    sprintf(memory[i].path[0], "%d", this_port);
                    memory[i].path_len = 1;
                    break;
                }
            }
            
            
            for (int i = 0; i < index; i++) {
                if (this_index == i) continue; //do not send message to itself.
                rc = sendto(sock_rt, temp1, strlen(temp1), 0, (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));  //send message
                if (rc < 0) {
                    printf("Unable to send the message");
                    exit(1);
                }
            }
        }
        
        //receive from socket
        if (FD_ISSET(sock_rt, &socketFDS)) {
            rc = recvfrom(sock_rt, buf, sizeof(buf), 0, (struct sockaddr *) &client_addr, &fromLength);  //receive message
            if (rc < 0) {
                perror("recvfrom");
                exit(1);
            }
            char* Ver = strtok(buf, " ");  //Protocol version
            char* Loc = strtok(NULL, " ");
            char* To = strtok(NULL, " ");
            char* From = strtok(NULL, " ");
            char* Hop = strtok(NULL, " ");
            char* msg_TYPE = strtok(NULL, " ");
            char* msg_id = strtok(NULL, " ");
            char* Route = strtok(NULL, " ");
            char* M = strtok(NULL, "");
            int Loc_number = atoi(Loc);
            int To_number = atoi(To);
            int From_number = atoi(From);
            int Hop_number = atoi(Hop);
            int id_number = atoi(msg_id);
            int x = ceil(Loc_number/(double)col);
            int y = (Loc_number-1)%col + 1;
            int distance = sqrt(pow(this_row-x, 2) + pow(this_col-y, 2)); //must be "int".
            int temp_index = -1;
            for (int i = 0; i < index; ++i) {
                if (From_number == port_array[i]) {
                    temp_index = i;
                    break;
                }
            }
            if (strcmp(Ver, "6") != 0) {
                printf("Wrong version!\n");
                continue;
            }
            if (strcmp(msg_TYPE, "MOV") == 0) {
                if (To_number == this_port) {
                    strcpy(location, M);   //update the location
                    this_loc = atoi(M);   //update the location
                    this_row = ceil(this_loc/(double)col);  //calculate the coordinate x
                    this_col = (this_loc-1)%col + 1;    //calculate the coordinate y
//                    printf("Received a MOV command from port %s.\n", From);
//                    printf("Moved to location %s, the coordinates are (%d,%d).\n", location, this_row, this_col);
                }
                continue;
            }
            
            if (strcmp(msg_TYPE, "LOC") == 0) {
                if (To_number == this_port) {
                    char temp1[200];
                    sprintf(temp1, "%s %s %s %d %d %s %d %d %s", Ver, location, From, this_port, MaxHopcount, msg_TYPE, -1, this_port, location);
//                    printf("Received a LOC command from port %s.\n", From);
//                    printf("Send: %s\n", temp1);
                    for (int i = 0; i < index; i++) {
                        if (this_index == i) continue; //do not send message to itself.
                        if (port_array[i] != From_number) continue;
                        rc = sendto(sock_rt, temp1, strlen(temp1), 0, (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));  //send message
                        if (rc < 0) {
                            printf("Unable to send the message");
                            exit(1);
                        }
                    }
                }
                continue;
            }
            
            if (Loc_number <= col*row && distance <= 2) {
                
                //remove message in the memory after receive a corresponding ACK
                if (strcmp(msg_TYPE, "ACK") == 0) {
                    for (int i = 0; i < 30; ++i) {
                        if (memory[i].isEmpty == true) continue;
                        if (strcmp(memory[i].msg_TYPE, "ACK") == 0) continue;
                        if (strcmp(memory[i].From, To) == 0 && strcmp(memory[i].To, From) == 0 && strcmp(memory[i].msg_id, msg_id) == 0) {
                            memory[i].isEmpty = true;
//                            printf("message deleted: %s\n", memory[i].temp1);
                        }
                    }
                }
                
                
                if (To_number == this_port) {
                    if (strcmp(msg_TYPE, "MSG") == 0 && last_ID[temp_index] == id_number-1) { //send an ACK
                        last_ID[temp_index] = id_number;
                        
                        printf("Version: %s; Received messages from Location %s; The source of the message is port %s; Hopcount: %d.\n", Ver, Loc, From, Hop_number);
                        printf("msg_ID: %s; Type of the message: %s.\n", msg_id, msg_TYPE);
                        printf("The routing path: %s\n", Route);
                        printf("The messege received: %s\n", M);
                        
                        int Hop_number = MaxHopcount - 1;
                        char* Ver = "6";
                        char* M = "#ACK#";
                        char* msg_TYPE = "ACK";
                        char Route[100];
                        sprintf(Route, "%d", this_port);
                        char temp1[200];
                        sprintf(temp1, "%s %s %s %d %d %s %s %s %s", Ver, location, From, this_port, Hop_number, msg_TYPE, msg_id, Route, M);
                        
                        for (int i = 0; i < index; i++) {
                            if (this_index == i) continue; //do not send message to itself.
                            rc = sendto(sock_rt, temp1, strlen(temp1), 0, (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));  //send message
                            if (rc < 0) {
                                printf("Unable to send the message");
                                exit(1);
                            }
                        }
                        
                        //store the message in the memory
                        for (int i = 0; i < 30; ++i) {
                            if (memory[i].isEmpty == false) continue;
                            memory[i].isEmpty = false;
                            memory[i].count = 5;
                            memory[i].time = time((time_t*)NULL);
                            
                            sprintf(memory[i].To, "%s", From);
                            sprintf(memory[i].From, "%d", this_port);
                            sprintf(memory[i].msg_id, "%s", msg_id);
                            sprintf(memory[i].msg_TYPE, "%s", msg_TYPE);
                            sprintf(memory[i].temp1, "%s", temp1);
                            
//                            printf("message stored: %s\n", temp1); //for debugging
                            
                            memory[i].path_len = 1;
                            sprintf(memory[i].path[0], "%d", this_port);
                            break;
                        }
                    }
                    
                    if (strcmp(msg_TYPE, "ACK") == 0 && msgID[temp_index] <= id_number) {
                        msgID[temp_index] = id_number + 1;
                        
                        printf("Version: %s; Received messages from Location %s; The source of the message is port %s; Hopcount: %d.\n", Ver, Loc, From, Hop_number);
                        printf("msg_ID: %s; Type of the message: %s.", msg_id, msg_TYPE);
                        printf("The routing path: %s\n", Route);
                        printf("The messege received: %s\n", M);
                    }
                }else {
                    if (Hop_number == 0) continue;
                    Hop_number--;
                    char* Ver = "6";  //Protocol version
                    char new_Route[100];
                    sprintf(new_Route, "%s,%d", Route, this_port); //add the port to the route path.
                    char temp1[200];
                    //“<v> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>”
                    sprintf(temp1, "%s %s %s %s %d %s %s %s %s", Ver, location, To, From, Hop_number, msg_TYPE, msg_id, new_Route, M);
                    char* path[5];
                    int path_len = 0;
                    char* token = strtok(new_Route, ",");
                    while (token != NULL) {
                        path[path_len] = token;
                        path_len++;
                        token = strtok(NULL, ",");
                    }
                    
                    //store the message in the memory
                    for (int i = 0; i < 30; ++i) {
                        if (memory[i].isEmpty == false) continue;
                        memory[i].isEmpty = false;
                        memory[i].count = 5;
                        memory[i].time = time((time_t*)NULL);
                        
                        sprintf(memory[i].To, "%s", To);
                        sprintf(memory[i].From, "%s", From);
                        sprintf(memory[i].msg_id, "%s", msg_id);
                        sprintf(memory[i].msg_TYPE, "%s", msg_TYPE);
                        sprintf(memory[i].temp1, "%s", temp1);
                        
//                        printf("message stored: %s\n", temp1); //for debugging
                        
                        memory[i].path_len = path_len;
                        for (int j = 0; j < path_len; ++j) {
                            sprintf(memory[i].path[j], "%s", path[j]);
                        }
                        break;
                    }
                    
                    for (int i = 0; i < index; i++) {
                        bool IftoSend = true;
                        for (int j = 0; j < path_len; j++) {  //to see if port_array[i] is already in the path. if yes, don't send the message to port_array[i]
                            if(port_array[i] == atoi(path[j])) {
                                IftoSend = false;
                                break;
                            }
                        }
                        if (IftoSend == false) continue;
                        rc = sendto(sock_rt, temp1, strlen(temp1), 0, (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));  //send message
                        if (rc < 0) {
                            printf("Unable to send the message");
                            exit(1);
                        }
                    }
                }
            }
        }
    }
    return 0;
}


