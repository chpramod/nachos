//#ifndef STRUCT_H
//#define STRUCT_H


#include "copyright.h"
#include "utility.h"
#include "list.h"
#include "thread.h"




class childListElement{
    private:
        NachOSThread* childThread;
        ChildStatus childStatus;
        int exitVal;
        
    public:
        childListElement(NachOSThread* childThread, ChildStatus childStatus, int exitVal){
            this->childThread=childThread;
            this->childStatus=childStatus;
            this->exitVal=exitVal;
        };
        ChildStatus getChildStatus(){ return this->childStatus; };
        void changeChildStatus(ChildStatus childStatus){ this->childStatus=childStatus; };
        int getExitVal(){ return this->exitVal; };
        void changeExitVal(int exitVal){ this->exitVal=exitVal; };
        NachOSThread* getChildThread(){ return this->childThread;   };
        void resetParent(){
            if(this->childStatus==ALIVE){
                this->childThread->setPPID(0);
                this->childThread->parent=NULL;
            }
        }
        
       
};
