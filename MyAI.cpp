#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#define MAXTHREAD 4


struct Move
{
    int X;
    int Y;
    int Score;
};

struct MinMaxData
{
    char (*Board)[8];
    int Side;
    int Depth;
    int Alpha;
    int Beta;
    int *Score;
};

int PriorityMap[8][8]=
{
    250,-5,25,10,10,25,-5,250,
    -5,-50,5,5,5,5,-50,-5,
    25,5,5,5,5,5,5,25,
    10,5,5,0,0,5,5,10,
    10,5,5,0,0,5,5,10,
    25,5,5,5,5,5,5,25,
    -5,-50,5,5,5,5,-50,-5,
    250,-5,25,10,10,25,-5,250,
};


extern int Round;
extern char MyTable[8][8];
extern int Playable(int Y,int X,int Player,unsigned char *ReverseRecord,const char MyTestTable[8][8]);
extern int Reverse(int Y,int X,int Player,unsigned char ReverseRecord,char MyTestTable[8][8]);
extern HANDLE MyMutex;
extern int AvailableMove(int Side);

int ParallelLevel;
int CMin=999999,CMax=-999999;
int DepthLimit;


int MinMax(char TestBoard[8][8], int Side, int Depth,int Alpha,int Beta);
int CMP_Max(const void* A,const void* B);
int CMP_Min(const void* A,const void* B);
int Referee(const char TestBoard[8][8]);
unsigned int __stdcall SearchThread(void * Data);
Move NextMove(int Side);
Move RandomMove(int Side);

int CMP_Max(const void* A,const void* B)
{
    return ((Move*)B)->Score-((Move*)A)->Score;
}

int CMP_Min(const void* A,const void* B)
{
    return ((Move*)A)->Score-((Move*)B)->Score;
}

int Referee(const char TestBoard[8][8])
{
    int I,J,K;
    for (I=0,K=0; I<8; I++)
    {
        for (J=0; J<8; J++)
        {
            if (Round >= 50)
            {
                K+=TestBoard[I][J];
            }
            else
            {
                K+=TestBoard[I][J]*PriorityMap[I][J];
            }
        }
    }
    return K;
}

int ExternalDiscs(const char TestBoard[8][8])
{
    int I,J,K;
    int W=0,B=0;
    for (I=0; I<8; I++)
    {
        for (J=0; J<8; J++)
        {
            K=0;
            if (MyTable[I][J]!=0)
            {
                if (I-1>=0)
                {
                    if (MyTable[I-1][J]==0)
                        K=1;
                }
                if (I+1<8)
                {
                    if (MyTable[I+1][J]==0)
                        K=1;
                }
                if (J-1>=0)
                {
                    if (MyTable[I][J-1]==0)
                        K=1;
                }
                if (J+1<8)
                {
                    if (MyTable[I][J+1]==0)
                        K=1;
                }
                /////////////////////////////
                if (I-1>=0&&J-1>=0)
                {
                    if (MyTable[I-1][J-1]==0)
                        K=1;
                }
                if (I+1<8&&J-1>=0)
                {
                    if (MyTable[I+1][J-1]==0)
                        K=1;
                }
                if (I-1>=0&&J+1<8)
                {
                    if (MyTable[I-1][J+1]==0)
                        K=1;
                }
               if (I+1<8&&J+1<8)
                {
                    if (MyTable[I+1][J+1]==0)
                        K=1;
                }
                ///////////////////////////////
            }
            if (K==1)
            {
                if (MyTable[I][J]==1)
                    W++;
                else
                    B++;
            }
        }
    }
    return W-B;
}

Move RandomMove(int Side)
{
    int I,J,K;
    Move MyMove[64]= {-1,-1,0};
    srand(time(NULL));
    unsigned char RR;
    for (I=0,K=0; I<8; I++)
    {
        for (J=0; J<8; J++)
        {
            if (MyTable[I][J]==0)
            {
                if (0!=Playable(I,J,Side,&(RR),MyTable))
                {
                    MyMove[K].X=J;
                    MyMove[K].Y=I;
                    K++;
                }
            }
        }
    }
    if (K==0)
    {
        return MyMove[0];
    }
    MyMove[0]=MyMove[rand()%K];
    MyTable[MyMove[0].Y][MyMove[0].X]=Side;
    Playable(MyMove[0].Y,MyMove[0].X,Side,&(RR),MyTable);
    Reverse(MyMove[0].Y,MyMove[0].X,Side,RR,MyTable);
    return MyMove[0];
}

Move NextMove(int Side)
{
    unsigned char RR[64];
    char NextBoard[64][8][8];

    MinMaxData MyMinMaxData[64];
    HANDLE hSearchThread[64]= {0};

    char text[20];
    int I,J,K,L,M;
    int Index;
    Move MyMove[64];
    Move MyTemp;
    if (Round >= 50)
    {
        DepthLimit=15;
    }
    for (Index=0,I=0; I<8; I++)
    {
        for (J=0; J<8; J++)
        {
            if (MyTable[I][J]==0)
            {
                if (0!=Playable(I,J,Side,&(RR[0]),MyTable))
                {
                    MyMove[Index].X=J;
                    MyMove[Index].Y=I;
                    MyMove[Index].Score=1;
                    Index++;
                }
            }
        }
    }
    if (Index==0)
        return {-1,-1,0};
    /*Pre-Search*/
    for (I=0; I<Index; I++)
    {
        Playable(MyMove[I].Y,MyMove[I].X,Side,&RR[0],MyTable);
        memcpy(NextBoard[0],MyTable,8*8);
        NextBoard[0][MyMove[I].Y][MyMove[I].X]=Side;
        Reverse(MyMove[I].Y,MyMove[I].X,Side,RR[0],NextBoard[0]);
        MyMove[I].Score=MinMax(NextBoard[0],(Side==1?-1:1),DepthLimit/2,-999999,999999);
    }
    qsort(MyMove,Index,sizeof(Move),(Side==1)?CMP_Max:CMP_Min);
    /**/
    CMin=999999,CMax=-999999;
    for (I=0,J=0; I<Index; I++)
    {
        Playable(MyMove[I].Y,MyMove[I].X,Side,&(RR[J]),MyTable);
        memcpy(NextBoard[J],MyTable,8*8);/*not a thread-safety function but never mind. Fix it in later version*/
        NextBoard[J][MyMove[I].Y][MyMove[I].X]=Side;
        Reverse(MyMove[I].Y,MyMove[I].X,Side,RR[J],NextBoard[J]);
        MyMinMaxData[J].Board=NextBoard[J];
        MyMinMaxData[J].Side=Side;
        MyMinMaxData[J].Alpha=CMax;
        MyMinMaxData[J].Beta=CMin;
        MyMinMaxData[J].Depth=0;
        MyMinMaxData[J].Score=&(MyMove[I].Score);
        hSearchThread[J]=(HANDLE)_beginthreadex(NULL,0,SearchThread,(void*)&(MyMinMaxData[J]),NULL,NULL);
        if (hSearchThread[J]==0)
        {
            sprintf(text,"Thread fail to be generated.\n");
            MessageBox(NULL,text,TEXT("Error!"),MB_OK);
        }
        J++;
        if (J==ParallelLevel)
        {
            if (0xFFFFFFFF==WaitForMultipleObjects(J,hSearchThread,TRUE,INFINITE))
            {
                sprintf(text,"Error %d\n",GetLastError());
                MessageBox(NULL,text,TEXT("Error!"),MB_OK);
            }
            J=0;
            for (L=0; L<J; L++)
            {
                CloseHandle(hSearchThread[J]);
            }
        }
    }
    if (J>0)
    {
        if (0xFFFFFFFF==WaitForMultipleObjects(J,hSearchThread,TRUE,INFINITE))
        {
            sprintf(text,"Error %d\n",GetLastError());
            MessageBox(NULL,text,TEXT("Error!"),MB_OK);
        }
        for (L=0; L<J; L++)
        {
            CloseHandle(hSearchThread[J]);
        }
    }
    for (I=0; I<Index; I++)
    {
        printf("%d ",MyMove[I].Score);
    }
    printf("==========");
    MyTemp.X=-1;
    MyTemp.Y=-1;

    if (Side==1)
    {
        MyTemp.Score=-999999;
        for (I = 0; I < Index; I++)
        {
            if (MyMove[I].Score>MyTemp.Score)
            {
                MyTemp=MyMove[I];
            }
        }
    }
    else
    {
        MyTemp.Score=999999;
        for (I = 0; I < Index; I++)
        {
            if (MyMove[I].Score<MyTemp.Score)
            {
                MyTemp=MyMove[I];
            }
        }
    }
    printf("%d ==========\n",MyTemp.Score);
    MyTable[MyTemp.Y][MyTemp.X]=Side;
    Playable(MyTemp.Y,MyTemp.X,Side,&(RR[0]),MyTable);
    Reverse(MyTemp.Y,MyTemp.X,Side,RR[0],MyTable);
    return MyTemp;
}

int MinMax(char TestBoard[8][8], int Side, int Depth,int Alpha,int Beta)
{
    int I,J,K,L,M;
    unsigned char RR=0;
    char NextBoard[8][8];
    int Max=-999999,Min=999999;
    Move MyMove[64];
    int Index=0;

    int MWeight[8]= {2,2,2,2,2,0,0,0};
    int FWeight[8]= {1,1,1,1,1,0,0,0};
    if (Depth < DepthLimit)
    {
        for (Index=0,I=0; I<8; I++)
        {
            for (J=0; J<8; J++)
            {
                if (TestBoard[I][J]==0)
                {
                    if (0!=Playable(I,J,Side,&RR,TestBoard))
                    {
                        MyMove[Index].X=J;
                        MyMove[Index].Y=I;
                        MyMove[Index].Score=1;
                        Index++;
                    }
                }
            }
        }
        /*Pre-Search*/
        for (I=0; I<Index; I++)
        {
            Playable(MyMove[I].Y,MyMove[I].X,Side,&RR,TestBoard);
            memcpy(NextBoard,TestBoard,8*8);
            NextBoard[MyMove[I].Y][MyMove[I].X]=Side;
            Reverse(MyMove[I].Y,MyMove[I].X,Side,RR,NextBoard);
            MyMove[I].Score=MinMax(NextBoard,(Side==1?-1:1),Depth+DepthLimit/2,-999999,999999);
        }
        qsort(MyMove,Index,sizeof(Move),(Side==1)?CMP_Max:CMP_Min);
        /**/

        for (I=0; I<Index; I++)
        {
            Playable(MyMove[I].Y,MyMove[I].X,Side,&RR,TestBoard);
            memcpy(NextBoard,TestBoard,8*8);  /*not a thread-safety function but never mind. Fix it in later version*/
            NextBoard[MyMove[I].Y][MyMove[I].X]=Side;
            Reverse(MyMove[I].Y,MyMove[I].X,Side,RR,NextBoard);
            K=MinMax(NextBoard,(Side==1?-1:1),Depth+1,Alpha,Beta)+MWeight[(int)(Round/10.0)]*Side*Index;
            if (Side==1)
            {
                if (K>Max)
                {
                    Max=K;
                    if (Alpha<Max)
                        Alpha=Max;
                }
            }
            else
            {
                if (K<Min)
                {
                    Min=K;
                    if (Beta>Min)
                        Beta=Min;
                }
            }
            if (Alpha>=Beta)
                return (Side==1?Max:Min);
        }
        if (Index==0)
        {
            if (Round >= 50)
                return MinMax(TestBoard,(Side==1?-1:1),Depth+1,Alpha,Beta);
            else
                return MinMax(TestBoard,(Side==1?-1:1),Depth+1,Alpha,Beta)+(-1)*Side*50;
        }

        return (Side==1?Max:Min);
    }
    return Referee(TestBoard);
}

unsigned int __stdcall SearchThread(void * Data)
{
    int K;
    //printf("Search Thread running!!\n");
    *(((MinMaxData*) Data)->Score)=K=MinMax( (((MinMaxData*) Data)->Board),
                                     ((((MinMaxData*) Data)->Side)==1?-1:1),
                                     (((MinMaxData*) Data)->Depth)+1,
                                     ((MinMaxData*) Data)->Alpha,
                                     ((MinMaxData*) Data)->Beta);

    /*critical section Bug-related

    WaitForSingleObject(MyMutex, INFINITE);
    if (((MinMaxData*) Data)->Side==1)
    {
        if (K>CMax)
        {
            CMax=K;
        }
    }
    else
    {
        if (K<CMin)
        {
            CMin=K;
        }
    }

    ReleaseMutex(MyMutex);
    /**/
    //printf("Search Thread ending!!\n");
    _endthreadex(0);
}
