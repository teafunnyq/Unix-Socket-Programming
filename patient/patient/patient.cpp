//
//  main.cpp
//  patient
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
#include <string>

using namespace std;

//Hard coded UDP port numbers
#define HEALTH_CENTER_PORT "30040"
#define PATIENT1_PORT "51040"
#define PATIENT2_PORT "52040"
#define PATIENT3_PORT "53040"
#define PATIENT4_PORT "54040"

void do_phase1(int patient_num);
void do_phase3(int patient_num, string tcp);

int main(int argc, const char * argv[]) {
    int patient_num = 0;
    int pid;
    
    while(patient_num < 4){
        if( (pid = fork()) == 0){
            do_phase1(patient_num);
            //do_phase3(patient_num, tcp);
            break;
        }
        else{
            patient_num++;
        }
    }
    
    wait(NULL);
    
    return 0;
}

void do_phase1(int patient_num){
    char* patientPorts[] = {PATIENT1_PORT, PATIENT2_PORT, PATIENT3_PORT, PATIENT4_PORT};
    
    //Sockets that patient binds to and health center socket to sends to
    int patientSocket, centerSocket;
    struct addrinfo hints, *hs, *p;
    
    //Makes sure that hints is cleared and has the proper parameters
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    //Get address of patient port socket
    if(getaddrinfo(NULL, patientPorts[patient_num], &hints, &hs) != 0){
        perror("Error at patient getaddrinfo function");
        exit(1);
    }
    
    //Loop through all results
    for(p = hs; p != NULL; p = p->ai_next){
        //Create socket for patient
        if((patientSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("Error at socket function from patient");
            continue;
        }
        
        //Bind to socket for patient
        if(::bind(patientSocket, p->ai_addr, p->ai_addrlen) == -1){
            close(patientSocket);
            perror("Error at bind function from patient");
            continue;
        }
        
        break;
    }
    
    if(p == NULL){
        perror("Didn't connect to anything from patient");
        exit(1);
    }
    
    freeaddrinfo(hs);
    
    //Bind patient to dynamically allocated TCP port
    int patientTCPSocket;
    struct sockaddr_in addr;
    
    if ((patientTCPSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("Failed to create TCP socket for patient");
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (::bind(patientTCPSocket, (const struct sockaddr *)&addr, sizeof(addr)) == -1) {
        printf("Failed to bind to TCP socket for patient");
    }
    
    socklen_t addrLen = sizeof(addr);
    if (getsockname(patientTCPSocket, (struct sockaddr *)&addr, &addrLen) == -1) {
        printf("getsockname() failed for patient");
    }
    
    //Makes sure that hints is cleared and has the proper parameters
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    //Get address info of health center
    if(getaddrinfo(NULL,HEALTH_CENTER_PORT, &hints, &hs)){
        perror("Server: error at patient getaddrinfo function");
        exit(1);
    }
    
    //Loop through all results
    for(p = hs; p != NULL; p = p->ai_next){
        //Try to get file descriptor of socket for healthcenter
        if((centerSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("Error in socket function from patient (for healthcenter)");
            continue;
        }
        
        //Try to connect to the socket for health center
        if(connect(centerSocket,p->ai_addr,p->ai_addrlen) == -1){
            close(centerSocket);
            perror("Error in connect function from patient (for healthcenter)");
            continue;
        }
         
        break;
    }
    
    if(p == NULL){
        perror("Patient didn't create a socket for health center");
        exit(1);
    }
    
    freeaddrinfo(hs);
    
    //Send message to health center
    string randomDoc;
    if(patient_num == 0){
        randomDoc = "1";
    }
    else{
        randomDoc = "2";
    }
    
    string tcpPORT = to_string(ntohs(addr.sin_port));
    string message = "patient" + to_string(patient_num + 1) + " IP " + tcpPORT + " doctor" + randomDoc;
    char *charMessage = new char[message.length() + 1];
    strcpy(charMessage, message.c_str());
    if(send(centerSocket, charMessage, strlen(charMessage), 0) == -1){
        perror("Server: error in send function from patient");
    }
    
    //Close UDP sockets betwee patient and health center
    close(patientSocket);
    close(centerSocket);
    
    //Listen for any incoming messages on TCP socket
    if(listen(patientTCPSocket, 10) == -1){
        perror("Error at listen function of patient");
        exit(1);
    }
    
    //Get socket information of doctor/patient sending welcome message
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int receivingSocket = accept(patientTCPSocket, (struct sockaddr *)&their_addr, &addr_size);
    
    if(receivingSocket == -1){
        perror("Error in accept function of patient from doctor");
        exit(1);
    }
    
    //Receive welcome message from doctor/other patient
    int bytesReceived;
    int MAXDATASIZE = 100;
    char buffer[MAXDATASIZE];
    if ((bytesReceived = recv(receivingSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
        perror("Error in recv function at patient from doctor");
        exit(1);
    }
    
    //Complete c-string
    buffer[bytesReceived] = '\0';
    
    cout << "Patient" << patient_num + 1 << ": " << buffer << endl;
    
    int numPatientsLeft = stoi(string(1,buffer[0]));
    int doctorNumber = stoi(string(1,buffer[19]));
    
    //Ack message information
    char *ackMessage = "OK!";
    int byteOfAck = strlen(ackMessage);
    
    if(numPatientsLeft == 1){
        //Send acknowledgement message back after receiving message from doctor/other patient
        if(send(receivingSocket, ackMessage, byteOfAck, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        int bytesReceived2;
        char buffer2[MAXDATASIZE];
        
        //Receive message about other patients to connect to
        if ((bytesReceived2 = recv(receivingSocket, buffer2, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        
        cout << "Patient" << patient_num + 1 << ": " << buffer2 << endl;
        
        buffer2[bytesReceived2] = '\0';
        
        //Send acknowledgement for other patient message
        if(send(receivingSocket, ackMessage, byteOfAck, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        //Receive message on schedule of doctors
        if ((bytesReceived2 = recv(receivingSocket, buffer2, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        buffer2[bytesReceived2] = '\0';
        
        cout << "Patient" << patient_num + 1 << ": " << buffer2 << endl;
        
        cout << "Patient" << to_string(patient_num + 1) << " joined doctor" << doctorNumber << endl;
        
        close(receivingSocket);
        close(patientTCPSocket);
    }
    else{
        //Send acknowledgement message back after receiving message from doctor/other patient
        if(send(receivingSocket, ackMessage, byteOfAck, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        int bytesReceived2;
        char buffer2[MAXDATASIZE];
        
        //Receive message about other patients to connect to
        if ((bytesReceived2 = recv(receivingSocket, buffer2, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        
        cout << "Patient" << patient_num + 1 << ": " << buffer2 << endl;
        
        buffer2[bytesReceived2] = '\0';
        
        //Copy patients into a message to send to other peers
        string otherPatients(buffer2);
        
        //Send acknowledgement for other patient message
        if(send(receivingSocket, ackMessage, byteOfAck, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        //Receive message on schedule of doctors
        if ((bytesReceived2 = recv(receivingSocket, buffer2, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        buffer2[bytesReceived2] = '\0';
        
        cout << "Patient" << patient_num + 1 << ": " << buffer2 << endl;
        
        //Send acknowledgement for schedule
        if(send(receivingSocket, ackMessage, byteOfAck, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        string schedule(buffer2);
        
        cout << "Patient" << to_string(patient_num + 1) << " joined doctor" << doctorNumber << endl;
        
        //Close TCP sockets
        close(receivingSocket);
        close(patientTCPSocket);
        
        //TCP Socket of patient we want to send message to
        int nextPatient;
        struct addrinfo hints, *hs, *p;
        
        //Makes sure that hints is cleared and setting the proper parameters
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        
        //Get port information of patients TCP socket
        string patientTCP = otherPatients.substr(9,5);
        char *charPatientTCP = new char[patientTCP.length() + 1];
        strcpy(charPatientTCP, patientTCP.c_str());
        
        if(getaddrinfo(NULL, charPatientTCP, &hints, &hs) != 0){
            perror("Error in getaddrinfo function at patient for other patients");
            exit(1);
        }
        
        //Go through all returned results in list
        for(p = hs; p != NULL; p = p -> ai_next){
            //Try to get file descriptor of socket for peer patient
            if((nextPatient = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
                perror("Error in socket function at patients for other patients");
                continue;
            }
            
            //Try to connect to the socket for peer patient
            if(connect(nextPatient, p->ai_addr, p->ai_addrlen) == -1){
                perror("Error in connect function at patients for other patients");
                close(nextPatient);
                continue;
            }
            
            break;
        }
        
        if(p == NULL){
            perror("Doctor failed to connect to patient");
            exit(2);
        }
        
        freeaddrinfo(hs);
        
        string welcomeMsg = to_string(numPatientsLeft - 1) + " welcome to doctor" + to_string(doctorNumber) + " group";
        char *charYourDoc = new char[welcomeMsg.length() + 1];
        strcpy(charYourDoc, welcomeMsg.c_str());
        
        string updOtherPatients = otherPatients.substr(15);
        char *charupdPatients = new char[updOtherPatients.length() + 1];
        strcpy(charupdPatients, updOtherPatients.c_str());
        
        string updSchedule = schedule.substr(9);
        char *charupdSchedule = new char[updSchedule.length() + 1];
        strcpy(charupdSchedule, updSchedule.c_str());
        
        //Send welcome message
        int byteOfMsg3 = strlen(charYourDoc);
        if(send(nextPatient, charYourDoc, byteOfMsg3, 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        //Receive Ack on welcome message
        int bytesReceived3;
        char buffer3[MAXDATASIZE];
        if ((bytesReceived3 = recv(nextPatient, buffer3, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        buffer3[bytesReceived3] = '\0';
        
        //Send other patients
        if(send(nextPatient, charupdPatients, strlen(charupdPatients), 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        //Receive ack on other patients
        if ((bytesReceived3 = recv(nextPatient, buffer3, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        buffer3[bytesReceived3] = '\0';
        
        //Send new schedule
        if(send(nextPatient, charupdSchedule, strlen(charupdSchedule), 0) == -1){
            perror("Error in send function for acknowledgement to doctor");
        }
        
        //Receive ack on new schedule
        if ((bytesReceived3 = recv(nextPatient, buffer3, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error in recv function at patient from doctor");
            exit(1);
        }
        buffer3[bytesReceived3] = '\0';
        
        close(nextPatient);
    }
}
