//
//  main.cpp
//  doctor
//
//  Created by Tiffany Kyu on 11/4/17.
//  Copyright © 2017 Tiffany Kyu. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string>

using namespace std;

//Hardcoded UDP ports
#define HEALTH_CENTER_PORT "30040"
#define DOCTOR1_PORT "41040"
#define DOCTOR2_PORT "42040"

string do_phase2(int doctor_num, string doctor_name, string doctor_pwd);
void do_phase3(int doctor_num, string msg);

int main(int argc, const char * argv[]) {
    int doctor_num = 0;
    int pid;
    
    string doctorName[] = {"doctor1" , "doctor2"};
    string doctorPass[] = {"aaaaa", "bbbbb"};
    
    while(doctor_num < 2){
        if( (pid = fork()) == 0){
            string message = do_phase2(doctor_num, doctorName[doctor_num], doctorPass[doctor_num]);
            do_phase3(doctor_num, message);
            break;
        }
        else{
            doctor_num++;
        }
    }
    wait(NULL);
    
    return 0;
}

// Use UDP to send user name password to health center get information from healthcenter
string do_phase2(int doctor_num, string doctor_name, string doctor_pwd){
    char* doctorPorts[] = {DOCTOR1_PORT, DOCTOR2_PORT};
    
    //Sockets that doctor needs or wants to send messages to
    int doctorSocket, centerSocket;
    struct addrinfo hints, *hs, *p;
    
    //Makes sure that hints is cleared and has the proper parametrs
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    //Get address of doctor port socket
    if(getaddrinfo(NULL,doctorPorts[doctor_num], &hints, &hs)){
        perror("Error at doctor getaddrinfo function");
        exit(1);
    }
    
    //Loop through all results
    for(p = hs; p != NULL; p = p->ai_next){
        //Create socket
        if((doctorSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("Error at socket function from doctor");
            continue;
        }
        
        //Bind to socket
        if(::bind(doctorSocket, p->ai_addr, p->ai_addrlen) == -1){
            close(doctorSocket);
            perror("Error at bind function from doctor");
            continue;
        }
        
        break;
    }
    
    freeaddrinfo(hs);
    
    if(p == NULL){
        perror("Doctor didn't connect to anything");
        exit(1);
    }
    
    //Get address info of health center
    if(getaddrinfo(NULL,HEALTH_CENTER_PORT, &hints, &hs)){
        perror("Error at doctor getaddrinfo function (for healthcenter msg)");
        exit(1);
    }
    
    //Loop through all results
    for(p = hs; p != NULL; p = p->ai_next){
        //Try to get file descriptor of socket
        if((centerSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("Error in socket function from doctor (for healthcenter msg)");
            continue;
        }
        
        //Try to connect to the socket
        if(connect(centerSocket,p->ai_addr,p->ai_addrlen) == -1){
            close(centerSocket);
            perror("Error in connect function from doctor (for healthcenter msg)");
            continue;
        }
        
        break;
    }
    
    if(p == NULL){
        perror("Doctor didn't connect to anything for healthcenter");
        exit(1);
    }
    
    freeaddrinfo(hs);
    
    string docNum = to_string(doctor_num + 1);
    
    //Send message to health center
    string message = doctor_name + " " + doctor_pwd;
    char *charMessage = new char[message.length() + 1];
    strcpy(charMessage, message.c_str());
    if(send(centerSocket, charMessage, strlen(charMessage), 0) == -1){
        perror("Error in send function from doctor to healthcenter");
    }
    
    //Receive messages from health center
    int bytesReceived;
    int MAXDATASIZE = 200;
    char buffer[MAXDATASIZE];
    
    if ((bytesReceived = recv(doctorSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
        perror("Error in recv at doctor from healthcenter");
        exit(1);
    }
    
    //Complete c-string
    buffer[bytesReceived] = '\0';
    
    string message2(buffer);
    
    close(doctorSocket);
    close(centerSocket);
    
    return message2;
}

void do_phase3(int doctor_num, string msg){
    //Find out which patients are assigned to this doctor
    char whichPatients[4];  //Patient numbers
    char* patientPorts[4];  //Ports of each patient
    int index = 0;
    
    //Open .txt file to read information from
    ifstream infile;
    infile.open(msg);
    
    string patientInfo;
    while(!infile.eof()){
        getline(infile,patientInfo);
        
        int intDocNum = patientInfo[patientInfo.length() - 1] - '0';
        if(intDocNum == (doctor_num + 1)){
            whichPatients[index] = patientInfo[7];
            
            string stringPortNum = patientInfo.substr(12,5);
            char * charPortNum = new char [stringPortNum.length()+1];
            strcpy (charPortNum, stringPortNum.c_str());
            patientPorts[index] = charPortNum;
            
            index++;
        }
    }
    
    //Open dynamic TCP port for doctor
    int doctorSocket;
    struct sockaddr_in addr;
    socklen_t addrLen;
    
    if ((doctorSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to create TCP socket for doctor");
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (::bind(doctorSocket, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("Failed to bind TCP socket for doctor");
    }
    
    addrLen = sizeof(addr);
    if (getsockname(doctorSocket, (struct sockaddr *)&addr, &addrLen) == -1) {
        printf("getsockname() failed for doctor");
    }
    
    //Actions depending on number of patients assigned to this doctor
    if(index == 0){
        cout << "Doctor" + to_string(doctor_num + 1) + " has no peer subscribers!" << endl;
    }
    else{
        //TCP Socket of patient we want to send message to
        int patientSocket;
        struct addrinfo hints, *hs, *p;
        
        //Makes sure that hints is cleared and setting the proper parameters of the network
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        //Get information of patients TCP socket
        if(getaddrinfo(NULL, patientPorts[0], &hints, &hs) != 0){
            perror("Error in getaddrinfo function at doctors for patients");
            exit(1);
        }
        
        //Go through all returned results in list
        for(p = hs; p != NULL; p = p -> ai_next){
            //Try to get file descriptor of socket
            if((patientSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
                perror("Error in socket function at doctors for patients");
                continue;
            }
            
            //Try to connect to the socket
            if(connect(patientSocket, p->ai_addr, p->ai_addrlen) == -1){
                close(patientSocket);
                perror("Error in connect function at doctor for patients 1");
                continue;
            }
           
            break;
        }
        
        if(p == NULL){
            perror("Doctor failed to connect to patient");
            exit(2);
        }
        
        freeaddrinfo(hs);
        
        if(index == 1){
            cout << "Doctor" + to_string(doctor_num + 1) + " has one patient subscriber!" << endl;
            
            string realMsg = "1 welcome to doctor" + to_string(doctor_num + 1) + " group";
            char *message = new char[realMsg.length() + 1];
            strcpy(message, realMsg.c_str());
            
            //Send message to the socket of server
            if(send(patientSocket, message, strlen(message), 0) == -1){
                perror("Error in send function from doctor to patient");
            }
            
            //Receive acknowledgement of message
            int bytesReceived;
            int MAXDATASIZE = 100;
            char buffer[MAXDATASIZE];
            if ((bytesReceived = recv(patientSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
                perror("Error in recv function at doctor from patient");
                exit(1);
            }
            
            buffer[bytesReceived] = '\0';
        }
        else{
            
            cout << "Doctor" + to_string(doctor_num + 1) + " has " << index <<" patients!" << endl;
            
            string realMsg =  to_string(index) + " welcome to doctor" + to_string(doctor_num + 1) + " group";
            char *message = new char[realMsg.length() + 1];
            strcpy(message, realMsg.c_str());
            
            //Send welcome message to the first patient
            if(send(patientSocket, message, strlen(message), 0) == -1){
                perror("Error in send function from doctor to patient");
            }
            
            //Receive acknowledgement message from patient
            int bytesReceived;
            int MAXDATASIZE = 100;
            char buffer[MAXDATASIZE];
            if ((bytesReceived = recv(patientSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
                perror("Error in recv function at doctor from patient");
                exit(1);
            }
            
            buffer[bytesReceived] = '\0';
            
            //Send message back about info of rest of patients
            string patientMsg = "";
            for(int i = 1; i < index; i++){
                string patient(1, whichPatients[i]);
                patientMsg += "patient" + patient + " ";
                patientMsg += string(patientPorts[i]) + "\n";
            }
            char *convPatientMsg = new char[patientMsg.length() + 1];
            strcpy(convPatientMsg, patientMsg.c_str());
            
            if(send(patientSocket, convPatientMsg, strlen(convPatientMsg), 0) == -1){
                perror("Error in send function from doctor to patient (schedule)");
            }
            
            //Receive acknowledgement and send message of schedule
            //Open file to read doctor's usernames and passswords from
            ifstream infile;
            string fileName = "doctor" + to_string(doctor_num + 1) + ".txt";
            infile.open("/users/tiffanykyu/Desktop/Project2/" + fileName);
            
            string doctorSchedule;
            string scheduleMessage = "";
            //Store the schedule into string to send as a message
            while(!infile.eof()){
                getline(infile,doctorSchedule);
                scheduleMessage += doctorSchedule;
            }
            char *schedulePatientMsg = new char[scheduleMessage.length() + 1];
            strcpy(schedulePatientMsg, scheduleMessage.c_str());
            
            if ((bytesReceived = recv(patientSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
                perror("Error in recv function at doctor from patient");
                exit(1);
            }
            
            buffer[bytesReceived] = '\0';
            
            if(send(patientSocket, schedulePatientMsg, strlen(schedulePatientMsg), 0) == -1){
                perror("Error in send function from doctor to patient (schedule)");
            }
        }
        
        close(patientSocket);
    }
    close(doctorSocket);
}
