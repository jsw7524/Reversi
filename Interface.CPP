#include <windows.h>
#include <winable.h>
#include <process.h>
#include <stdio.h>
#include "resource.h"

#define PLAYER1 1	//白
#define PLAYER2 -1  //黑
#define BKCOLOR RGB(0,150,250)	//定義棋盤顏色
#define BOARD_WIDTH 50		    //棋盤格寬度

//-----------------------------------------------------------
struct Move		                //定義結構儲存棋子的位置
{
    int x;
    int y;
    int Score;
};
struct ThreadData
{
    int Side;
    struct Move* N;

};
extern int Playable(int Y,int X,int Player,unsigned char* ReverseRecord,const char MyTestTable[8][8]);
extern int Reverse(int Y,int X,int Player,unsigned char ReverseRecord,char MyTestTable[8][8]);
extern int GetBlackNumber();
extern int GetWhiteNumber();
extern Move NextMove(int Side);
extern void  RestoreMyTable();
extern void  BackUpMyTable();
extern void ResetGame();
extern int AvailableMove(int Side);
extern Move RandomMove(int Side);
//-----------------------------------------------------------
extern int DepthLimit;
extern char MyTable[8][8];
extern int  Round;
extern int ParallelLevel;
//-----------------------------------------------------------
int who_play;			//判斷現在誰玩
HANDLE MyMutex;
HWND hwnd;
HMENU hMenu;
//----------------------------------------------------------
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);//視窗訊息處理程式
void PaintBoard(HDC,RECT,POINT);//繪製棋盤的函式
void PaintMyTable(HWND,RECT,Move,int);//繪製棋子的函式
void AI_Thread(void * MyThreadData);
void Benchmark_Thread(void * MyThreadData);
void Manager_Thread(void * MyThreadData);
//----------------------------------------------------------
HBITMAP bmpExercising;
HDC MemDCExercising;
HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
    static TCHAR AppName[]=TEXT("Ruby&Waku Reversi");
    MSG msg;
    WNDCLASS wndclass= {CS_SAVEBITS,WndProc,0,0,hInstance,
                        LoadIcon(hInstance,MAKEINTRESOURCE(IDI_ICON)),LoadCursor(NULL,IDC_ARROW),
                        (HBRUSH)GetStockObject(BLACK_BRUSH),AppName,AppName
                       };
    if(!RegisterClass(&wndclass))
    {
        MessageBox(NULL,TEXT("fail to initialize"),AppName,MB_ICONERROR);
        return 0;
    }
    hwnd=CreateWindowEx(WS_EX_TOPMOST,AppName,"Ruby&Waku Reversi 13.7",
                        WS_CAPTION|WS_BORDER|WS_SYSMENU|WS_MINIMIZEBOX,
                        0,0,500,500,
                        NULL,LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU)),hInstance,NULL);
    MyMutex = CreateMutex(
                  NULL,              // default security attributes
                  FALSE,             // initially not owned
                  NULL);

    ShowWindow(hwnd,nShowCmd);(HBITMAP)
    UpdateWindow(hwnd);
    while(GetMessage(&msg,NULL,0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
ThreadData TD[64];
Move MMM[64];
Move PreviousMove;
int L,F;
SYSTEM_INFO siSysInfo;
LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    static RECT rt_board,//儲存棋盤大小的變數
           rt_MyTable;//儲存棋子大小的變數
    static POINT pt_board;//儲存棋盤格線初始座標的變數
    Move pchoice;
    int check_win,i,j,w,b;
    TCHAR text[20];
    static TCHAR text1[20];
    RECT rt_text;
    unsigned char ReverseRecord=0;
    HANDLE hThreadAI;

    switch(msg)
    {
    case WM_CREATE:
        hMenu=GetMenu(hwnd);
        GetSystemInfo(&siSysInfo);
        SendMessage(hwnd,WM_COMMAND,(WPARAM)IDC_RESET,NULL);
        SendMessage(hwnd,WM_COMMAND,(WPARAM)IDC_COREAUTO,NULL);
        return 0;
    case WM_SIZE:
        SetRect(&rt_board,0,0,400,400);//設定棋盤大小
        SetRect(&rt_MyTable,0,0,50,50);
        pt_board.x=25;//設定第一調棋盤格線的起始座標位置
        pt_board.y=25;
        return 0;
    case WM_PAINT:
        hdc=BeginPaint(hwnd,&ps);
        PaintBoard(hdc,rt_board,pt_board);//繪製棋盤
        for(i=0; i<8; i++)
        {
            for(j=0; j<8; j++)
            {
                pchoice.x=j;
                pchoice.y=i;
                switch(MyTable[i][j])
                {
                case PLAYER1:
                    PaintMyTable(hwnd,rt_MyTable,pchoice,PLAYER1);
                    break;
                case PLAYER2:
                    PaintMyTable(hwnd,rt_MyTable,pchoice,PLAYER2);
                    break;
                }
            }
        }
        if(who_play==PLAYER2)
        {
            SetRect(&rt_text,400,70,500,110);
            //SetTextColor(hdc,RGB(0,0,0));
            SetBkColor(hdc,BKCOLOR);
            DrawText(hdc,"Black",-1,&rt_text,DT_CENTER);
        }
        else
        {
            SetRect(&rt_text,400,70,500,110);
            //SetTextColor(hdc,RGB(255,255,255));
            SetBkColor(hdc,BKCOLOR);
            DrawText(hdc,"White",-1,&rt_text,DT_CENTER);
        }
        SetRect(&rt_text,400,140,500,180);
        SetBkColor(hdc,BKCOLOR);
        switch (DepthLimit)
        {
        case 9:
            sprintf(text1,"%s","Easy");
            break;
        case 10:
            sprintf(text1,"%s","Standard");
            break;
        case 11:
            sprintf(text1,"%s","Hard");
            break;
        default:
            break;
        }

        DrawText(hdc,text1,-1,&rt_text,DT_CENTER);
        SetRect(&rt_text,400,200,500,240);
        //SetTextColor(hdc,RGB(255,255,255));
        SetBkColor(hdc,BKCOLOR);
        sprintf(text,"White:%2d",GetWhiteNumber());
        DrawText(hdc,text,-1,&rt_text,DT_CENTER);
        SetRect(&rt_text,400,250,500,290);

        //SetTextColor(hdc,RGB(0,0,0));
        sprintf(text,"Black:%2d",GetBlackNumber());
        DrawText(hdc,text,-1,&rt_text,DT_CENTER);



        EndPaint(hwnd,&ps);
        return 0;
    case WM_RBUTTONDOWN:
        break;
    case WM_LBUTTONDOWN:

        pchoice.x=LOWORD(lParam)-pt_board.x+0.5*BOARD_WIDTH;
        pchoice.y=HIWORD(lParam)-pt_board.y+0.5*BOARD_WIDTH;
        pchoice.x=pchoice.x/BOARD_WIDTH;//轉換為棋盤格交線位置
        pchoice.y=pchoice.y/BOARD_WIDTH;

        if(L==0&&MyTable[pchoice.y][pchoice.x]==0&&(pchoice.x>=0&&pchoice.x<=7)&&(pchoice.y>=0&&pchoice.y<=7))
        {
            if (Playable(pchoice.y,pchoice.x,who_play,&ReverseRecord,MyTable)!=0)
            {
                BackUpMyTable();
                EnableMenuItem(hMenu,IDC_RESTORATION,MF_ENABLED);
                PaintMyTable(hwnd,rt_MyTable,pchoice,who_play);//畫棋子
                MyTable[pchoice.y][pchoice.x]=who_play;
                Reverse(pchoice.y,pchoice.x,who_play,ReverseRecord,MyTable);
                Round++;
                who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
                InvalidateRect(hwnd,NULL,1);
                UpdateWindow(hwnd);
                hdc=GetDC(hwnd);
                SetRect(&rt_text,0,0,400,400);
                SetTextColor(hdc,RGB(255,0,0));
                SetBkColor(hdc,BKCOLOR);
                sprintf(text,"AI is thinking...");
                DrawText(hdc,text,-1,&rt_text,DT_CENTER);
                ReleaseDC(hwnd,hdc);
                TD[0].Side=who_play;
                TD[0].N=&(MMM[0]);
                L=1;
                hThreadAI=(HANDLE)_beginthread(Manager_Thread,0,(void*)&(TD[0]));
            }
        }
        return 0;
    case WM_DESTROY:
        CloseHandle(MyMutex);
        DeleteDC(MemDCExercising);
        DeleteObject(bmpExercising);
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND:

        switch(LOWORD(wParam))
        {
        case IDC_RESET:
            InvalidateRect(hwnd,NULL,TRUE);
            ResetGame();
            SendMessage(hwnd,WM_COMMAND,(WPARAM)IDC_BLACK,NULL);
            SendMessage(hwnd,WM_COMMAND,(WPARAM)IDC_LEVELSTANDARD,NULL);
            EnableMenuItem(hMenu,IDC_RESTORATION,MF_GRAYED);
            printf("=============\n");
            break;
        case IDC_ABORT:
            if (L==0)
            {
                TD[0].Side=who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
                TD[0].N=&MMM[0];
                L=1;
                _beginthread(Manager_Thread,0,(void*)&(TD[0]));
            }
            break;
        case IDC_FIRST:
            F=(F==1?0:1);
            CheckMenuItem(hMenu,IDC_FIRST,(F==1)?MF_CHECKED:MF_UNCHECKED);
            break;
        case IDC_BENCHMARK:
            _beginthread(Benchmark_Thread,0,NULL);
            break;
        case IDC_LEVELEASY:
            DepthLimit=9;
            CheckMenuItem(hMenu,IDC_LEVELEASY,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_LEVELSTANDARD,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_LEVELHARD,MF_UNCHECKED);
            ResetGame();
            EnableMenuItem(hMenu,IDC_RESTORATION,MF_GRAYED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_LEVELSTANDARD:
            DepthLimit=10;
            CheckMenuItem(hMenu,IDC_LEVELEASY,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_LEVELSTANDARD,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_LEVELHARD,MF_UNCHECKED);
            ResetGame();
            EnableMenuItem(hMenu,IDC_RESTORATION,MF_GRAYED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_LEVELHARD:
            DepthLimit=11;
            CheckMenuItem(hMenu,IDC_LEVELEASY,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_LEVELSTANDARD,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_LEVELHARD,MF_CHECKED);
            ResetGame();
            EnableMenuItem(hMenu,IDC_RESTORATION,MF_GRAYED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_WHITE:
            ResetGame();
            who_play=PLAYER1;
            who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
            RandomMove(who_play);
            Round++;
            who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
            CheckMenuItem(hMenu,IDC_BLACK,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_WHITE,MF_CHECKED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_BLACK:
            ResetGame();
            who_play=PLAYER2;
            CheckMenuItem(hMenu,IDC_BLACK,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_WHITE,MF_UNCHECKED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_COREAUTO:
            ParallelLevel=siSysInfo.dwNumberOfProcessors;
            CheckMenuItem(hMenu,IDC_COREAUTO,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_1CORE,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_2CORES,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_4CORES,MF_UNCHECKED);
            break;
        case IDC_1CORE:
            ParallelLevel=1;
            CheckMenuItem(hMenu,IDC_COREAUTO,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_1CORE,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_2CORES,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_4CORES,MF_UNCHECKED);
            break;
        case IDC_2CORES:
            ParallelLevel=2;
            CheckMenuItem(hMenu,IDC_COREAUTO,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_1CORE,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_2CORES,MF_CHECKED);
            CheckMenuItem(hMenu,IDC_4CORES,MF_UNCHECKED);
            break;
        case IDC_4CORES:
            ParallelLevel=4;
            CheckMenuItem(hMenu,IDC_COREAUTO,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_1CORE,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_2CORES,MF_UNCHECKED);
            CheckMenuItem(hMenu,IDC_4CORES,MF_CHECKED);
            break;
        case IDC_RESTORATION:
            Round-=2;
            RestoreMyTable();
            EnableMenuItem(hMenu,IDC_RESTORATION,MF_GRAYED);
            InvalidateRect(hwnd,NULL,1);
            UpdateWindow(hwnd);
            break;
        case IDC_VISION:
            ShellExecute(NULL,"open","ReadMe.txt",NULL,NULL,SW_SHOW);
            MessageBox(hwnd,TEXT("Author: Jia-Sen Wu(a.k.a. Netsphere)\nE-mail:netsphere@xuite.net\n\n"),TEXT("Hello!"),MB_OK);
            break;
        }
        return 0;
    }
    return DefWindowProc(hwnd,msg,wParam,lParam);
}

void PaintBoard(HDC hdc,RECT rt,POINT pt)//繪製棋盤
{
    HBRUSH hbr;
    RECT rt2;
    int i;
    int Offset=-25;
    TCHAR *szText[]= {"Color","Player","Level"};
    hbr=CreateSolidBrush(RGB(0,0,0));
    SelectObject(hdc,hbr);
    Rectangle(hdc,0,0,500,500);
    DeleteObject(hbr);
    hbr=CreateSolidBrush(RGB(0,150,0));//定義棋盤使用畫刷顏色
    FillRect(hdc,&rt,hbr);//繪製棋盤
    DeleteObject(hbr);
    hbr=CreateSolidBrush(BKCOLOR);
    SelectObject(hdc,hbr);
    SetBkColor(hdc,BKCOLOR);
    SetRect(&rt2,350,10,440,30);
    SetTextColor(hdc,RGB(0,0,0));
    for(i=1; i<3; i++)
    {
        SetRect(&rt2,400,70*i-30,500,40+70*i);
        DrawText(hdc,szText[i],-1,&rt2,DT_CENTER);
    }
    DeleteObject(hbr);
    hbr=CreateSolidBrush(RGB(0,0,0));//定義棋盤使用畫刷顏色
    SelectObject(hdc,hbr);
    Ellipse(hdc,95,95,105,105);
    Ellipse(hdc,200+95,95,200+105,105);
    Ellipse(hdc,95,200+95,105,200+105);
    Ellipse(hdc,200+95,200+95,200+105,200+105);
    DeleteObject(hbr);
    for(i=0; i<8; i++) //畫橫線
    {
        MoveToEx(hdc,pt.x+Offset,pt.y+BOARD_WIDTH*i+Offset,NULL);
        LineTo(hdc,BOARD_WIDTH*8+pt.x+Offset,pt.y+BOARD_WIDTH*i+Offset);
    }
    for(i=0; i<8; i++) //畫縱線
    {
        MoveToEx(hdc,pt.x+Offset+BOARD_WIDTH*i,pt.y+Offset,NULL);
        LineTo(hdc,pt.x+BOARD_WIDTH*i+Offset,BOARD_WIDTH*8+pt.y+Offset);
    }

}

void PaintMyTable(HWND hwnd,RECT rt,Move pt,int who)//畫棋子
{
    HDC hdc;
    HBRUSH hbr;
    hdc=GetDC(hwnd);
    RECT rt_text;
    HPEN hPen;
    if (!((pt.x >=0 && pt.x<=7 ) && (pt.y >=0 && pt.y<=7 )))
        return;
    if(who==PLAYER1)
    {
        hbr=CreateSolidBrush(RGB(255,255,255));//黑棋
        SelectObject(hdc,hbr);
    }
    else if (who==PLAYER2)
    {
        hbr=CreateSolidBrush(RGB(0,0,0));//白棋
        SelectObject(hdc,hbr);
    }
    else if (who==3)
    {
        if (who_play==PLAYER1)
        {
            hbr=CreateSolidBrush(RGB(255,255,255));//黑棋
            SelectObject(hdc,hbr);
        }
        else
        {
            hbr=CreateSolidBrush(RGB(0,0,0));//白棋
            SelectObject(hdc,hbr);
        }
        hPen = CreatePen(PS_DASH, 2, RGB(255, 0, 0));
        SelectObject(hdc,hPen);
    }
    Ellipse(hdc,rt.left+BOARD_WIDTH*pt.x,rt.top+BOARD_WIDTH*pt.y,
            rt.right+BOARD_WIDTH*pt.x,rt.bottom+BOARD_WIDTH*pt.y);
    DeleteObject(hbr);
    DeleteObject(hPen);
    ReleaseDC(hwnd,hdc);
}

void Manager_Thread(void * MyThreadData)
{
    HANDLE hThreadAI;
    int I;
    for (I=40001; I<=40022; I++)
    {
        EnableMenuItem(hMenu,I,MF_GRAYED);
    }
    hThreadAI=(HANDLE)_beginthread(AI_Thread,0,MyThreadData);
    WaitForSingleObject(hThreadAI,INFINITE);
    for (I=40002; I<=40022; I++)
    {
        EnableMenuItem(hMenu,I,MF_ENABLED);
    }
    if (0==AvailableMove(who_play)&&Round<64&&0!=GetBlackNumber()&&0!=GetWhiteNumber())
    {
        MessageBox(hwnd,TEXT("There is no place you can move.\nPress \"OK\" to skip this turn."),TEXT("Hello!"),MB_OK);
        PostMessage(hwnd,WM_COMMAND,(WPARAM)IDC_ABORT,NULL);
    }
    L=0;
    _endthread();
}

void AI_Thread(void * MyThreadData)
{
    int I;
    printf("AI Thread running!!\n");
    *(((ThreadData*)MyThreadData)->N)=NextMove(((ThreadData*)MyThreadData)->Side);
    Round++;
    who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
    InvalidateRect(hwnd,NULL,0);
    UpdateWindow(hwnd);
    who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
    PaintMyTable(hwnd, {0,0,50,50},*(((ThreadData*)MyThreadData)->N),3);
    who_play=(who_play==PLAYER1?PLAYER2:PLAYER1);
    printf("AI Thread ending!!\n");
    _endthread();
}

void Benchmark_Thread(void * MyThreadData)
{
    printf("Benchmark Thread running!!\n");
    HANDLE hThreadBenchmark;
    LARGE_INTEGER t1, t2, ts;
    int TK,i,j,k;
    TCHAR text[20];
    TK=DepthLimit;
    k=who_play;
    ParallelLevel;
    SendMessage(hwnd,WM_COMMAND,(WPARAM)IDC_LEVELSTANDARD,NULL);
    //DepthLimit=10;
    L=1;
    F=0;
    ResetGame();
    for (i=40002; i<=40022; i++)
    {
        EnableMenuItem(hMenu,i,MF_GRAYED);
    }
    QueryPerformanceFrequency(&ts);
    QueryPerformanceCounter(&t1);
    who_play=PLAYER1;
    for (i=0,j=who_play; i<64; i++,j*=-1)
    {
        TD[i].Side=j;
        TD[i].N=&MMM[i];
        hThreadBenchmark=(HANDLE)_beginthread(AI_Thread,0,(void*)&(TD[i]));
        WaitForSingleObject(hThreadBenchmark,INFINITE);
    }
    QueryPerformanceCounter(&t2);
    DepthLimit=TK;
    who_play=k;
    for (i=40002; i<=40022; i++)
    {
        EnableMenuItem(hMenu,i,MF_ENABLED);
    }
    sprintf(text,"Parallel Level: %d\nTime: %.2lf s",ParallelLevel,(t2.QuadPart-t1.QuadPart)/(double)(ts.QuadPart));
    MessageBox(hwnd,text,"Test Result",MB_OK);
    printf("Benchmark Thread ending!!\n");
    L=0;
    _endthread();
}
