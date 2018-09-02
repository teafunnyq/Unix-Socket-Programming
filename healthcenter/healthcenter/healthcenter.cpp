//
//  main.cpp
//  healthcenter
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

using namespace std;

//Hard coded UDP port numbers
#define HEALTH_CENTER_PORT "30040"
#define DOCTOR1_PORT "41040"
#define DOCTOR2_PORT "42040"

int main(int argc, const char * argv[]) {
    //Socket information that health center wants to use
    int centerSocket;
    struct addrinfo hints, *hs, *p;
    int yes = 1;
    
    //Makes sure that hints is cleared and has the proper parametrs
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    
    //Get address of health center socket at pre-defined port
    if(getaddrinfo(NULL, HEALTH_CENTER_PORT, &hints, &hs) != 0){
        perror("Error at healthcenter getaddrinfo function");
        return 1;
    }
    
    //Loop through all results
    for(p = hs; p != NULL; p = p->ai_next){
        //Create socket
        if((centerSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
            perror("Error at socket function from healthcenter");
            continue;
        }

        //Bind to socket
        if(::bind(centerSocket, p->ai_addr, p->ai_addrlen) == -1){
            close(centerSocket);
            perror("Error at bind function from healthcenter");
            continue;
        }
        
        break;
    }
    
    if(p == NULL){
        perror("Healthcenter didn't connect to anything");
        exit(1);
    }
    freeaddrinfo(hs);
    
    //Create file that we want to write patient information into
    ofstream myfile;
    myfile.open ("/users/tiffanykyu/Desktop/Project2/directory.txt");
    //myfile.open ("directory.txt");
    
    // Open file to read doctor's usernames and passswords from
    ifstream infile;
    infile.open("/users/tiffanykyu/Desktop/Project2/healthcenter.txt");
    //infile.open("healthcenter.txt");
    
    //Arrays to hold doctor information in
    string correctUser[2];
    string correctPass[2];
    int index = 0;
    
    string doctorInfo;
    //Store the correct log in of the doctors
    while(!infile.eof()){
        getline(infile,doctorInfo);
        doctorInfo.erase( remove(doctorInfo.begin(), doctorInfo.end(), '\r'), doctorInfo.end() );
        correctUser[index] = doctorInfo.substr(0,doctorInfo.find(' '));
        correctPass[index] = doctorInfo.substr(doctorInfo.find(' ') + 1);
        index++;
    }
    
    int numPatients = 0;
    while(1){
        //Get message from patients
        int bytesReceived;
        int MAXDATASIZE = 200;
        char buffer[MAXDATASIZE];
        
        if ((bytesReceived = recv(centerSocket, buffer, MAXDATASIZE-1 , 0)) == -1) {
            perror("Error at recv from healthcenter");
            exit(1);
        }
        
        buffer[bytesReceived] = '\0';
        
        //Find out whether message was sent from patient or doctor
        string fromWho(buffer,6);
        
        //If message comes from patient
        if(fromWho == "patien"){
            numPatients++;
            
            //Write message to txt file and to output
            myfile << buffer << endl;
            cout << "healthcenter: " << buffer << endl;
            
            //Write confirmation to screen
            string patientNumber(1,buffer[7]);
            cout << "Patient" + patientNumber + " registration is done successfully!" << endl;
            
            //If we've gone through all the patients, close the file and completed registration
            if(numPatients == 4){
                myfile.close();
                cout << "Registration of peers completed! Run the doctors!" << endl;
            }
        }
        //Message comes from doctors
        else if(fromWho == "doctor"){
            string doctorNumber(1,buffer[6]);
            cout << "incoming message from doctor" + doctorNumber << endl;
            int integerDocNum = stoi(doctorNumber);
            
            //Get address info of doctor sending message
            int doctorSocket;
            char* doctorPorts[] = {DOCTOR1_PORT, DOCTOR2_PORT};
            if(getaddrinfo(NULL,doctorPorts[integerDocNum - 1], &hints, &hs)){
                perror("Error at getaddrinfo function at healthcenter (msg from doctor)");
                exit(1);
            }
            
            //Loop through all results
            for(p = hs; p != NULL; p = p->ai_next){
                //Get file descriptor of socket
                if((doctorSocket = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1){
                    perror("Error at socket function at healthcenter (msg from doctor)");
                    continue;
                }
                
                //Connect to the socket
                if(connect(doctorSocket,p->ai_addr,p->ai_addrlen) == -1){
                    close(doctorSocket);
                    perror("Error at connect function at healthcenter (msg from doctor)");
                    continue;
                }
                break;
            }
            
            if(p == NULL){
                perror("Healthcenter didnt create a socket for doctor");
                exit(1);
            }
            
            freeaddrinfo(hs);
            
            //Doctor's log on information
            string msg(buffer);
            string username = msg.substr(0,msg.find(' '));
            string password = msg.substr(msg.find(' ') + 1);
            
            //Check if doctor's log on credentials is correct
            if(username == correctUser[integerDocNum - 1] && password == correctPass[integerDocNum - 1]){
                cout << "doctor" + doctorNumber + " logged in successfully" << endl;
                
                char *message = "/users/tiffanykyu/Desktop/Project2/directory.txt";
                //char *message = "directory.txt";
                int byteOfMsg = strlen(message);
                if(send(doctorSocket, message, byteOfMsg, 0) == -1){
                    perror("Error in send function from healthcenter to doctor");
                }
            }
            close(doctorSocket);
        }
    }
    
    close(centerSocket);
    
    return 0;
}
