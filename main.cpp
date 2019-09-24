#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */
#include <iostream>
#include <semaphore.h>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <fstream>
#include <random>
#include "graphcycle.h"

///Includes
//LETS DEFINE THE TRAINS
#define N 0
#define W 1
#define S 2
#define E 3
#define REQUEST 1
#define ALLOCATE 2
#define DEALLOCATE 0
//Name Semaphores File Discriptos
std::string toDirectionString(int direction);
//

/// BEGIN - Names
std::string NORTH_NAME= "NORTH_LOCK";
std::string SOUTH_NAME = "SOUTH_LOCK";
std::string EAST_NAME = "EAST_LOCK";
std::string WEST_NAME = "WEST_LOCK";
std::string MATRIX_NAME = "MATRIX_LOCK";
std::string JUNCTION_NAME= "JUNCTION_LOCK";
std::string Shared_MATRIX = "MATRIX";
///NAMES END
/// Begin LOCKS
sem_t* NORTH_LOCK;
sem_t* SOUTH_LOCK;
sem_t* EAST_LOCK;
sem_t* WEST_LOCK;
sem_t* MATRIX_LOCK;
sem_t* JUNCTION_LOCK;

///ENDS LOCKS
void setUpLocks(); //This Sets Up The Lock And Also mke sure the sempahore names are clear before using them
void unlinkLocks();
//SETTING UP THE MAIN FUNCTIONS TO MANAGE THE TRAIN
void trainManager();
void trainProcess(int train_direction, int trainID);
//FUNCTION TO READ THE SEQUENCE FROM FILE AND STORE IT INTO VECTOR
std::vector<int> readSequence();
//SETTING UP TRAINS WITH RESPECTIVE LOCKS AND DIRECTIONS
void setUpTrain(const int& directionCode, std::string& trainDirection, sem_t* & trainDirectionLock,
                std::string& rightDirection, sem_t* & trainRightLock, int& rightTrainCode );
//FUNCTION TO REQUEST LOCKS ACCORDING TO DIRECTION AND TRAIN ID
void request(int pid, int trainId, int direction, const std::string& directionString);
//FUNCTION TO ALLOCATE LOCKS TO THE TRAIN
void allocate(int pid, int trainId, int direction, const std::string& directionString);
//FUNCTION TO RELEASE THE LOCKS AFTER TRAIN HAS PASSED
void deallocate(int trainId, int direction);
//MATRIX SETUP
void setUpMatrix();
//Deadlock Detection
bool deadLockDetection(const std::vector<pid_t>& pidProc, const std::vector<int>& squence);
void printDeadLock(const std::vector<pid_t>& pidProc, const std::vector<int>& squence,const std::vector<int>& cycle );
//
//HELPS TO BUILD THE GRAPH
void helperGraphBuilder(GraphCycle& waitForGraph, std::vector < std::vector < int >>& graph,
                   const int & column , const int &trainIndex); //Assitn In Creating the wait For graph

//End
//READING AND WRITING TO FILES
std::string sequenceFileName =  "sequence.txt";
std::string matrixFileName =  "matrix.txt";
//COUNTING TRAINS
int trainCount;
//KEEPTING TRACKS OF THE LOCKS
int locksCount;
//PROCESS IDENTIFIER FOR TRAINS
pid_t trainprocess;
int main(int argc, char *argv[])
{
    setUpLocks();
    //Probabailty Block
    float probabilty  = -1;
    while(probabilty < 0.2 || probabilty > 0.7 ){
        std::cout << "Enter A Value Between 0.2 and 0.7: ";
        std::cin >> probabilty;
        std::cout << std::endl;
    }

    //intiate random engine 
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 1);
    std::vector<pid_t> pidTrains;
    //Probability Block Ends
    std::vector<int> seq = readSequence(); //reads and returns the squence to the user
    //adds the no. of trains from size of the sequence
    trainCount = seq.size();
    //no. of directional locks
    locksCount = 4;
    unsigned int train = 0 ;
    setbuf(stdout, NULL);
    setUpMatrix();
    std::cout << trainCount <<std::endl;
    //maintain train process intiate process 
    while (true) {
        if (train < trainCount && dis(gen) <= probabilty){
            //forking processes
            if ((trainprocess =fork()) == 0){
                trainProcess(seq[train], train);
            }
            //if forking fails
            else if (trainprocess < 0 ){
                std::cout << "Error Creating Train" << std::endl;
            }
            else{
                train++;
                pidTrains.push_back(trainprocess);

            }

        }//ENDIF
        else if(train < trainCount) {
            if (deadLockDetection(pidTrains,seq)){
                exit(0);
            }
        }
        else {
            while(train >= trainCount){
                sleep(1);
                if (deadLockDetection(pidTrains,seq)){
                    exit(0);
                }
            }
        }
    }
    while(true);






    return 0;
}
void setUpLocks(){
    //The Remove All Existing Semaphore Just In Case The Prrevious User happen to leave them unclean
    unlinkLocks();
    //Creates The Semaphores for each locks
    NORTH_LOCK =sem_open(NORTH_NAME.c_str(), O_CREAT, 0666, 1);
    SOUTH_LOCK =sem_open(SOUTH_NAME.c_str(), O_CREAT, 0666, 1);
    EAST_LOCK =sem_open(EAST_NAME.c_str(), O_CREAT, 0666, 1);
    WEST_LOCK =sem_open(WEST_NAME.c_str(), O_CREAT, 0666, 1);
    MATRIX_LOCK=sem_open(MATRIX_NAME.c_str(), O_CREAT, 0666, 1);
    JUNCTION_LOCK=sem_open(JUNCTION_NAME.c_str(), O_CREAT, 0666, 1);

}
void setUpMatrix(){
    //inititalizing the matrix here
    std::ofstream matrixFile(matrixFileName);
    if (matrixFile.is_open()){
        for (int i = 0 ; i < trainCount  ; i++){ //writes zero the file
            for (int y = 0 ; y < locksCount ; y++ ){
                matrixFile << 0 << " ";
            }
            matrixFile << std::endl;
        }
    }
    else {
        std::cout << "Could Not Create Matrix File" <<std::endl;
        exit(1);
    }
}

void unlinkLocks(){
    //named semaphores to unlink from specific trains(processes)
    sem_unlink(NORTH_NAME.c_str());
    sem_unlink(SOUTH_NAME.c_str());
    sem_unlink(EAST_NAME.c_str());
    sem_unlink(WEST_NAME.c_str());
    sem_unlink(MATRIX_NAME.c_str());
    sem_unlink(JUNCTION_NAME.c_str());
}

void trainProcess(int train_direction, int trainID){
    int pid = getpid() - getppid();
    //semaphores for the locks
    sem_t* directionLock;
    sem_t* rightLock;
    //direction of trains
    std::string trainDirection;  //The Train Direction
    std::string rightDirection;  //The Train Direction
    //setting up trains and giving the precedence to the right train in junction
    int rightTrainDirection;
    setUpTrain(train_direction,trainDirection,directionLock,rightDirection,rightLock,rightTrainDirection); //Sets The Train Up With It's Corresponding Lock
    //Time To Get The junction And matrix lock
    std::cout << "Train <pid" <<pid << ">: " <<trainDirection<< " train started" << std::endl;
    //requesting the locks
    request(pid,trainID,train_direction,trainDirection);//Request Direction
    //semaphore to get the directional lock
    sem_wait(directionLock);
    //if semaphore avaliable then allocate to the train
    allocate(pid,trainID,train_direction,trainDirection);//Allocate Directioon

    request(pid,trainID,rightTrainDirection,rightDirection);//Request Direction
    sem_wait(rightLock);
    allocate(pid,trainID,rightTrainDirection,rightDirection);//Allocate Directioon
    //after all proper directional locks then add the semaphore for junction lock
    sem_wait(JUNCTION_LOCK);
    std::cout << "Train <pid" <<pid << ">: " <<trainDirection<< " is crossing the junction" << std::endl;
    sleep(5);
    sem_post(directionLock);
    deallocate(trainID,train_direction);
    sem_post(rightLock);
    deallocate(trainID,rightTrainDirection);
    std::cout << "Train <pid" <<pid << ">: " <<trainDirection<< " has crossed the junction" << std::endl;
    sem_post(JUNCTION_LOCK);
    exit(1);
}

void setUpTrain(const int &directionCode, std::string& trainDirection,
                sem_t* & trainDirectionLock,
                std::string &rightDirection, sem_t* & trainRightLock,
                int &rightTrainCode ){
    //seeting up trains for each directions with specific semaphores
    if (directionCode == N){
        trainDirection = "North";
        trainDirectionLock = sem_open(NORTH_NAME.c_str(), O_CREAT, 0666, 0);
        rightDirection = "West";
        trainRightLock =sem_open(WEST_NAME.c_str(), O_CREAT, 0666, 0);
        rightTrainCode = W;
    }
    else if (directionCode ==S){
        trainDirection = "South";
        trainDirectionLock = sem_open(SOUTH_NAME.c_str(), O_CREAT, 0666, 0);
        rightDirection = "East";
        trainRightLock =sem_open(EAST_NAME.c_str(), O_CREAT, 0666, 0);
        rightTrainCode = E;
    }
    else if (directionCode== W){
        trainDirection = "West";
        trainDirectionLock = sem_open(WEST_NAME.c_str(), O_CREAT, 0666, 0);
         rightDirection = "South";
        trainRightLock =sem_open(SOUTH_NAME.c_str(), O_CREAT, 0666, 0);
        rightTrainCode = S;
    }
    else if (directionCode == E){
        trainDirection = "East";
        trainDirectionLock = sem_open(EAST_NAME.c_str(), O_CREAT, 0666, 0);
         rightDirection = "North";
        trainRightLock =sem_open(NORTH_NAME.c_str(), O_CREAT, 0666, 0);
        rightTrainCode = N;
    }

}

//File handling and using vectors to parse the stings
std::vector<int> readSequence(){
    std::ifstream fileSq(sequenceFileName,std::ios::in);
    if(!fileSq) {
        std::cout << "Cannot open file sequence for input.\n";
    }
    else {
        std::vector<int> sequence;
        char c;
        while(fileSq.get(c)){
            if (c != '\n'){
                std::cout << c;
                if (c == 'N'){
                    sequence.push_back(N);
                }
                else if (c == 'S'){
                    sequence.push_back(S);
                }
                else if (c == 'E'){
                    sequence.push_back(E);
                }
                else if (c == 'W'){
                    sequence.push_back(W);
                }

            }


        }
        fileSq.close();
        return sequence;
    }
    exit(0);

}

//Lock requesting function
void request(int pid,int trainId, int direction, const std::string& directionString){
    //matrix lock
    sem_wait(MATRIX_LOCK);
    //matrix file handling
    std::ifstream infile;
    infile.open (matrixFileName);
    //updating matrix
    if (infile.is_open()){
        std::vector < std::vector < int >> graph(trainCount);
        for (unsigned int i = 0; i < graph.size(); i++){
            graph[i] = std::move(std::vector < int >(locksCount, 0));
        }
        for (int i = 0; i < graph.size(); i++) { //taking the i vals to construct the rows
                for (int j = 0; j < graph[i].size(); j++) { //taking the j vals to construct the columns
                   infile >> graph[i][j] ;
                }
        }
        infile.close();
        graph[trainId][direction] = REQUEST;

        std::ofstream matrixFile(matrixFileName);
        if (matrixFile.is_open()){
            for (int i = 0 ; i < trainCount  ; i++){ //writes zero the file
                for (int y = 0 ; y < locksCount ; y++ ){
                    matrixFile << graph[i][y] << " ";
                }
                matrixFile << std::endl;
            }
        }
        matrixFile.close();
    }

    else{
        std::cout << "Train <pid" <<pid << ">: " << " could not get a " <<directionString<< "-Lock" <<std::endl;
        exit(-1);
    }
    //after matrix updates train requests the directional lock
    std::cout << "Train <pid" <<pid << ">: " << " requests " <<directionString<< "-Lock" <<std::endl;
    sem_post(MATRIX_LOCK);
}

//Lock allocating function
void allocate(int pid, int trainId, int direction, const std::string& directionString){
    //matrix lock
    sem_wait(MATRIX_LOCK);
    //matrix file handling
    std::ifstream infile;
    infile.open (matrixFileName);
    //updating matrix
    if (infile.is_open()){
        std::vector < std::vector < int >> graph(trainCount);
        for (unsigned int i = 0; i < graph.size(); i++){
            graph[i] = std::move(std::vector < int >(locksCount, 0));
        }
        for (int i = 0; i < graph.size(); i++) { //taking the i vals to construct the rows
                for (int j = 0; j < graph[i].size(); j++) { //taking the j vals to construct the columns
                   infile >> graph[i][j] ;
                }
        }
        infile.close();
        graph[trainId][direction] = ALLOCATE;

        std::ofstream matrixFile(matrixFileName);
        if (matrixFile.is_open()){
            for (int i = 0 ; i < trainCount  ; i++){ //writes zero the file
                for (int y = 0 ; y < locksCount ; y++ ){
                    matrixFile << graph[i][y] << " ";
                }
                matrixFile << std::endl;
            }
        }
        matrixFile.close();
    }

    else{
        std::cout << "Train <pid" <<pid << ">: " << " could not get a lock " <<directionString<< std::endl;
        exit(-1);
    }
    //after matrix updates train acquires the directional lock
    std::cout << "Train <pid" <<pid << ">: " << " Acquires " <<directionString<< "-Lock" <<std::endl;
    sem_post(MATRIX_LOCK);

}

//delocking function
void deallocate(int trainId, int direction){
    //matrix lock
    sem_wait(MATRIX_LOCK);
    //matrix file handling
    std::ifstream infile;
    infile.open (matrixFileName);
    //updating matrix
    if (infile.is_open()){
        std::vector < std::vector < int >> graph(trainCount);
        for (unsigned int i = 0; i < graph.size(); i++){
            graph[i] = std::move(std::vector < int >(locksCount, 0));
        }
        for (int i = 0; i < graph.size(); i++) { //taking the i vals to construct the rows
                for (int j = 0; j < graph[i].size(); j++) { //taking the j vals to construct the columns
                   infile >> graph[i][j] ;
                }
        }
        infile.close();
        graph[trainId][direction] = DEALLOCATE;

        std::ofstream matrixFile(matrixFileName);
        if (matrixFile.is_open()){
            for (int i = 0 ; i < trainCount  ; i++){ //writes zero the file
                for (int y = 0 ; y < locksCount ; y++ ){
                    matrixFile << graph[i][y] << " ";
                }
                matrixFile << std::endl;
            }
        }
        matrixFile.close();
    }

    else{
        std::cout << "Could Not Open File";
    }
    //After the locks have been deallocated they are up for use

    sem_post(MATRIX_LOCK);

}
//Main deadlock detection algorithm
bool deadLockDetection(const std::vector<pid_t>& pidProc, const std::vector<int>& squence){
     sem_wait(MATRIX_LOCK);//Get Matrix Lock
     //Matrix Manipulation
     std::vector < std::vector < int >> graph(trainCount);
     std::ifstream infile;
     infile.open (matrixFileName);
     if (infile.is_open()){
         for (unsigned int i = 0; i < graph.size(); i++){
             graph[i] = std::move(std::vector < int >(locksCount, 0));
         }
         for (int i = 0; i < graph.size(); i++) { //taking the i vals to construct the rows
                 for (int j = 0; j < graph[i].size(); j++) { //taking the j vals to construct the columns
                    infile >> graph[i][j] ;
                 }
         }
     }
     else {

         std::cout << "Could Not Open Matrix File From Manager";
         return false;
     }
     infile.close();
     GraphCycle waitForGraph(trainCount);
     //Next Convert RAG Graph To Wait For Graph
     for (int i = 0 ; i < locksCount; i++){//We are going to search the columns for twos?
         for (int y = 0 ; y < trainCount ;  y++){ //we are looking for two along a column
             if (graph[y][i] == ALLOCATE){
                 helperGraphBuilder(waitForGraph,graph,i,y);
             }
         }
     }
     //Time To test for the existence of a cycle
     std::vector<int> cyclePath;// This is where we are going to store our cycle//
     bool foundCycle = waitForGraph.cycle(cyclePath);
     sem_post(MATRIX_LOCK);//Let Go Of Matrix Lock
     if (foundCycle){
         std::cout << "***********DeadLock Found**********"<<std::endl;
         printDeadLock(pidProc,squence,cyclePath);
         return true; //We  found  A Cycle ThereFore  A DeadLock

     }
     else {
        return false; //We didn't find  A Cycle ThereFore not A DeadLock
     }
}

//Funtion to assist in building the graph from graphcycle.cpp
void helperGraphBuilder(GraphCycle& waitForGraph,std::vector < std::vector < int >>& graph, const int & column, const int& trainIndex ){
    for (int i = 0 ; i < trainCount; i++){//We are going to search the columns for twos?
        if (graph[i][column] == REQUEST){
            waitForGraph.addEdge(i,trainIndex);
        }
    }
}

void printDeadLock(const std::vector<pid_t>& pidProc, const std::vector<int>& squence,const std::vector<int>& cycle ){
    std::cout << cycle.size() << std::endl;
    for (int i = 0 ; i < cycle.size() ; i++){
        std::cout << "Train<pid"<<pidProc[cycle[i%cycle.size()]]<<"> from " <<toDirectionString(squence[cycle[i%cycle.size()]])<<
                     " is waiting for Train<"<<pidProc[cycle[(i+1)%cycle.size()]]<<"> from "<<
                                                                            toDirectionString(squence[cycle[(i+1)%cycle.size()]])<<" --------->" << std::endl;
    }
    exit(1);
}
std::string toDirectionString(int direction){
    if (direction ==N)
        return "NORTH";
    else if (direction == S)
        return "SOUTH";
    else if (direction == W)
        return  "WEST";
    else if (direction ==E)
        return "EAST";
}
