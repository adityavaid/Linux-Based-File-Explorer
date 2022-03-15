//-----------------------------------------------HEADER FILES--------------------------------------------------------------//
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <stack>
#include <unistd.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#define clearScreen() cout<<"\033c";  //printf("\033[H\033[J")
#define MAX 10                        //Change for Window Size
using namespace std;

//------------------------------------------------GLOBAL VARIABLES------------------------------------------------

char CWD[256];  //can only use getcwd() with char array
int sizeCWD=256;
string HomeDir;  // store main program directory
string RootDir;
vector<dirent*> DirectoryFiles;
vector<string> Dir_vect;

struct termios orig_term, norm_term, cmd_term;


stack<string> PrevDir,NextDir;


int top=1;  // For Terminal Window's Size
int offset=1;

struct TermConfig{
    int cx,cy;
    int s_rows,s_cols;
};
TermConfig T;




//--------------------command mode variables------------------//

vector<string> cmdtokens;
string main_command="";
string response="", Status="";









//---------------------------------------------------HELPER FUNCTIONS-----------------------------------------------------------//




void cursor_mover(char ch)
{
    switch(ch){
        case 'w':
        {
            if(T.cx == 1)
            {
                if(top==1)
                {
                    return;
                }
                else
                {
                    top--;
                    return;
                }

            }
            else
            {
                T.cx--;
                return ;
            }
        }

        case 's':
        {
            if(T.cx == T.s_rows)
            {
                top++;
                return;
            }
            else
            {
                T.cx++;
                return;
            }
        }

        case 'd':
        {
            if(T.cy == T.s_cols)
            {
                offset++;
                return;
            }
            else
            {
                T.cy++;
                return ;
            }
        }

        case 'a':
        {
            if(T.cy == 1)
            {
                if(offset==1)
                {
                    return;
                }
                else
                {
                    offset--;
                    return;
                }
            }
            else
            {
                T.cy--;
                return;
            }
        }
    }
}



void mvcursor(int x, int y)  //x->row y->col
{


    cout<<"\033[" << x<< ";" << y<< "H";
    fflush(stdout);

}



void printMode(string str)
{
    int x=MAX+5;
    int y=T.s_cols/2 - 8;

    mvcursor(x,y);
    cout<<"\033[1;46m"<<"MODE : "<<str<<"\033[0m";
    fflush(stdout);
    mvcursor(T.cx,T.cy);
}

void printResponse(string str)
{
    int x=T.s_rows-3;
    int y=T.s_cols/2 - 8;

    mvcursor(x,y);
    cout<<"\033[1;44m"<<"RESPONSE : "<<str<<"\033[0m";
    fflush(stdout);
    mvcursor(T.cx,T.cy);
}

void printCWD()
{
    int x=MAX+1;
    int y=0;

    mvcursor(x,y);
    cout<<"\033[1;44m"<<"CWD : "<<CWD<<"\033[0m";
    fflush(stdout);
    mvcursor(T.cx,T.cy);
}

void NormalModeDisable()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

void NormalModeEnable()
{
    struct termios noncanon=orig_term;
    noncanon.c_lflag &= ~(ECHO|ICANON);
    norm_term=noncanon;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &noncanon))
    {
        cout<<"ERROR: UNABLE TO SWITCH TO NORMAL MODE";
    }
}

void CommandModeEnable()
{
    struct termios noncanon=orig_term;
    noncanon.c_lflag &= ~(ICANON);
    cmd_term=noncanon;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &noncanon))
    {
        cout<<"ERROR: UNABLE TO SWITCH TO COMMAND MODE";
    }
}





//*****************************************************************************************************************//
//**************************************************DISPLAY DRIVERS************************************************//
string getPerm(struct stat fileStat)
{   string res="";

    ((S_ISDIR(fileStat.st_mode)) ? res=res+"d" : res=res+"-");
    ((fileStat.st_mode & S_IRUSR) ? res=res+"r" : res=res+"-");
    ((fileStat.st_mode & S_IWUSR) ? res=res+"w" : res=res+"-");
    ((fileStat.st_mode & S_IXUSR) ? res=res+"x" : res=res+"-");
    ((fileStat.st_mode & S_IRGRP) ? res=res+"r" : res=res+"-");
    ((fileStat.st_mode & S_IWGRP) ? res=res+"w" : res=res+"-");
    ((fileStat.st_mode & S_IXGRP) ? res=res+"x" : res=res+"-");
    ((fileStat.st_mode & S_IROTH) ? res=res+"r" : res=res+"-");
    ((fileStat.st_mode & S_IWOTH) ? res=res+"w" : res=res+"-");
    ((fileStat.st_mode & S_IXOTH) ? res=res+"x" : res=res+"-");

    return res;
}

string getDate(struct stat fileStat)
{   string res="";
    int file_size = fileStat.st_size;

    if((file_size/1048576) > 1)
        res= to_string(file_size/ 1048576)+" MB";
    else if(file_size/1024 > 1)
        res=to_string(file_size/1024)+" KB";
    else
        res= to_string(file_size)+" B";


    return res;
}


void CurDirProcessor()
{
    DIR *dr;
    struct dirent* di;
    getcwd(CWD,256);
    struct stat fileStat;

    dr=opendir(CWD);
    if(dr== nullptr)
    {
        cout<<"Error: No directory found";
        return;
    }
    DirectoryFiles.clear();
    Dir_vect.clear();
    int i=1;
    string res;
    for(di= readdir(dr);di!= nullptr;di= readdir(dr))
    {
        DirectoryFiles.push_back(di);
        res="$"+to_string(i++)+")"+"    " ;

        lstat(di->d_name,&fileStat);

        struct passwd *p=getpwuid(fileStat.st_uid);
        struct group *g=getgrgid(fileStat.st_gid);


        //printf("%-8s%-100s\n", di->d_name, (getPerm(fileStat)).c_str());

        res+=getPerm(fileStat)+"    " ;
        res+= string(p->pw_name)+ "    "+ string(g->gr_name)+ "    " ;

        res+=getDate(fileStat)+"    " ;

        string temp =string(ctime(&fileStat.st_mtime));

        res+= temp.substr(0,temp.length()-1)+ "    ";


        res+=(string)di->d_name;

        Dir_vect.push_back(res);
        res="";

    }
    closedir(dr);
}

void DisplayCurDir()
{   CurDirProcessor();
    clearScreen();
    dirent* di;
    struct stat fileStat;

    int n;

    if(top+MAX-1 < (Dir_vect.size()))
    {
        n=top+MAX-1;
    }
    else
    {
        n=Dir_vect.size();
    }

    for(int i=top-1;i < n;i++)
    {
        cout<<(Dir_vect[i]).substr(offset-1,T.s_cols)<<endl;
    }

    /*string res;


    for(int i=0;i<DirectoryFiles.size();i++)
    {
        di=DirectoryFiles[i];
        lstat(di->d_name,&fileStat);


        //printf("%-8s%-100s\n", di->d_name, (getPerm(fileStat)).c_str());
        cout<<"$"<<i+1<<")"<<"     ";
        cout<<getPerm(fileStat)<<"     ";
        cout<<getDate(fileStat)<<"     ";
        cout<<di->d_name;



        cout<<endl;
    }*/
}




//-----------------------------------------------------------------------------------------------------------

void GetTermDim()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);
    T.s_rows=ws.ws_row;
    T.s_cols=ws.ws_col;

}


void init()
{
    clearScreen();

    struct passwd *pw = getpwuid(getuid());

    const char *hmdir = pw->pw_dir;
    RootDir=string(hmdir);
    getcwd(CWD,256);
    HomeDir=string(CWD);
    GetTermDim();
    T.cx=1;T.cy=1;

    DisplayCurDir();
    printCWD();
    printMode("NORMAL MODE");
    Status="NORMAL MODE";


    tcgetattr(STDIN_FILENO,&orig_term);

}





//---------------------------------------------------Normal FUNCTIONS---------------------------------------------------------//

bool isDir(string str);





void EnterDirByPath(string str)
{
    PrevDir.push(string(CWD));
    chdir(str.c_str());
    CurDirProcessor();
    top=1;offset=1;
    DisplayCurDir();
}

void EnterByCursor()
{
    while(!NextDir.empty())
    {
        NextDir.pop();
    }

    PrevDir.push(string(CWD));
    int dirpos=(top+(T.cx)-2);

    struct dirent *di=DirectoryFiles[dirpos];


    string tempname=string(di->d_name);

    if(isDir(tempname))
    {
        tempname="./"+tempname;
        chdir(tempname.c_str());
        top=1;offset=1;
    }
    else
    {
        tempname=string(CWD)+ "/" +tempname;
        pid_t pid=fork();
        if(!pid)
        {
            execl("/usr/bin/xdg-open","xdg-open",(tempname.c_str()),NULL);
            exit(1);
        }
    }


}
//--------------------------------------------NORMAL MODE NAVIGATION---------------------------------




void GoForward()
{
    if(NextDir.empty())
    {
        return;
    }

    PrevDir.push(string(CWD));
    string next=NextDir.top();
    NextDir.pop();
    chdir(next.c_str());

}
void GoBackward()
{
    if(PrevDir.empty())
    {
        return;
    }

    NextDir.push(string(CWD));
    string prev=PrevDir.top();
    PrevDir.pop();
    chdir(prev.c_str());
}

void LevelUp()
{
    while(!NextDir.empty())
    {
        NextDir.pop();
    }
    PrevDir.push(string(CWD));
    chdir("..");

}

void Home()
{
    while(!NextDir.empty())
    {
        NextDir.pop();
    }
    PrevDir.push(string(CWD));

    chdir(HomeDir.c_str());

}

void ScrollUp()
{
    if(top-MAX>=1)
    {
        top=top-MAX;

    }


}

void ScrollDown()
{
    top=top+MAX;
}



//------------------------Commands----------------------------------------------
string PathResolver(string str)
{
    string res;

    if(str[0]=='~')
    {
        res=RootDir+str.substr(1,str.length()); //check

    }
    else if(str=="." )
    {
        res=string(CWD);
    }
    else if(str[0]=='/')
    {
        res=str;
    }
    else if(str[0]=='.' && str[1]=='/')
    {
        res=string(CWD) + str.substr(1,str.length());
    }
    else
    {
        res=string(CWD)+"/"+str;
    }

    return res;

}

string FilenameExtractor(string str)
{
    string res;
    string space_delimiter = "/";

    size_t pos = 0;
    while ((pos = str.find(space_delimiter)) != string::npos)
    {
        res=str.substr(0, pos);
        str.erase(0, pos + space_delimiter.length());
    }
    return str;
}

bool isabsolute(string str)
{
    if(str.substr(0,RootDir.length())==RootDir)
    {
        return true;
    }
    else
        return false;
}


bool isDir(string str)
{
    struct stat temp_stat;
    if (lstat(str.c_str(), &temp_stat) == -1) {
        response="error opening the file/directory";
        return false;
    }

    if (S_ISDIR(temp_stat.st_mode))
    {
        return true;

    }
    else
    {
        return false;
    }
}


//-------------------------------COPY-----------------------------------------------
void CopyFile(string source, string destination)
{
    struct stat s_fstat, d_fstat;

    if (lstat(source.c_str(),&s_fstat) == -1) {
        response = "error opening the file/directory";
        return;
    }


    int n=s_fstat.st_size, nr;
    char *filebuff=new char[n];

    int in = open(source.c_str(),O_RDONLY);
    int out = open(destination.c_str(),O_WRONLY|O_CREAT,s_fstat.st_mode);

    if(in<0 || out<0)
    {
        response="ERROR OPENING THE FILE";
        return;
    }

    while((nr=(read(in,filebuff,sizeof(filebuff)))) > 0)
    {
        write(out,filebuff,nr);
    }



    int chflag=chown(destination.c_str(),s_fstat.st_uid,s_fstat.st_gid);



    chflag=chmod(destination.c_str(),s_fstat.st_mode);

    close(in);
    close(out);

    return;




}



void CopyDir(string source,string destination)
{
    //cout<<source<<"    "<<destination<<endl;

    struct stat s_fstat, d_fstat, temp_stat;
    lstat(source.c_str(), &s_fstat);
    mkdir(destination.c_str(), s_fstat.st_mode);


    DIR *dr;
    struct dirent *di;
    vector<string> temp_dir;

    //cout<<"in loop now";

    dr = opendir(source.c_str());
    if (dr == nullptr) {
        response= "Error: No directory found";
        return;
    }

    for (di = readdir(dr); di != nullptr; di = readdir(dr)) {
        temp_dir.push_back(string(di->d_name));

    }

    closedir(dr);


    for (int i = 0; i < temp_dir.size(); i++) {

        if (temp_dir[i] == "." || temp_dir[i] =="..")                        //if(strcmp((temp_dir[i])->d_name, ".")==0 || strcmp((temp_dir[i])->d_name, "..")==0)
        {
            continue;
        }

        string f_name = temp_dir[i];
        string temppath_src = source + "/" + f_name;
        string temppath_dest = destination + "/" + f_name;

        //cout << temppath_src << endl;
        if (lstat(temppath_src.c_str(), &temp_stat) == -1) {
            continue;
        }

        if (S_ISDIR(temp_stat.st_mode))
        {
            CopyDir(temppath_src, temppath_dest);

        } else if (S_ISREG(temp_stat.st_mode))
        {
            CopyFile(temppath_src, temppath_dest);
        }


    }

}


void Copy()
{
    response="COPY SUCCESSFUL";
    int n=cmdtokens.size();
    string dest= PathResolver(cmdtokens[n-1]);
    string source;
    string destination;

    if(n<3)
    {
        response="ERROR - INVALID NUMBER OF ARGUMENTS";
        return;
    }


    for(int i=1;i<n-1;i++)
    {
        source= PathResolver(cmdtokens[i]);
        destination=dest+"/"+ FilenameExtractor(cmdtokens[i]);

        if(isDir(source))
        {
            CopyDir(source,destination);
        }
        else
        {
            CopyFile(source,destination);
        }
    }

}


//---------------------------------------------DELETE--------------------------------------------------------------------------//


    void DeleteFile(string src)
    {
        int err=remove(src.c_str());

        if(err!=0)
        {
            response="ERROR: FILE NOT DELETED";
        }


    }

    void DeleteDir(string src)
    {
        struct stat s_fstat, d_fstat, temp_stat;
        lstat(src.c_str(),&s_fstat);



        DIR* dr;
        struct dirent* di;
        vector<string> temp_dir;



        dr=opendir(src.c_str());
        if(dr== nullptr)
        {
            response="Error: No directory found";
            return;
        }

        for(di= readdir(dr);di!= nullptr;di= readdir(dr))
        {
            temp_dir.push_back(string(di->d_name));
        }

        closedir(dr);

        for(int i=0;i<temp_dir.size();i++)
        {
            if(temp_dir[i] == "." || temp_dir[i] == "..")
            {
                continue;
            }


            string temppath_src = src+"/"+ temp_dir[i];
            lstat(temppath_src.c_str(),&temp_stat);


            if(S_ISDIR(temp_stat.st_mode)==1)
            {
                DeleteDir(temppath_src);

            }
            else
            {
                DeleteFile(temppath_src);
            }

        }

        rmdir(src.c_str());


    }


    void Del()
    {
        response="DELETE SUCCESSFUL";
        int n=cmdtokens.size();
        string dest= PathResolver(cmdtokens[n-1]);
        string filename;
        if(n==2)
        {
            filename= PathResolver(cmdtokens[1]);
            if(isDir(filename))
            {
                DeleteDir(filename);
            }
            else
            {
                DeleteFile(filename);
            }

            return;
        }

        for(int i=1;i<n-1;i++)
        {
            filename=dest+"/"+cmdtokens[i];
            if(isDir(filename))
            {
                DeleteDir(filename);
            }
            else
            {
                DeleteFile(filename);
            }
        }



    }

///--------------------SEARCH ---------------------------------//


    bool SearchUtil(string directory, string filename )
    {

        struct stat temp_stat;

        DIR* dr;
        struct dirent* di;



        dr=opendir(directory.c_str());
        if(dr== nullptr)
        {
            return false;
        }

        for(di= readdir(dr);di!= nullptr;di= readdir(dr))
        {
            string temp_dir =string(di->d_name);
            if(temp_dir == "." || temp_dir == "..")
            {
                continue;
            }

            string temppath_src = directory+"/"+ string(di->d_name);
            lstat(temppath_src.c_str(),&temp_stat);

            if(S_ISDIR(temp_stat.st_mode))
            {

                if(temp_dir==filename)
                {
                    return true;
                }
                else if(SearchUtil(temppath_src,filename))
                {
                    return true;
                }
            }
            else
            {
                if(string(di->d_name) == filename)
                {
                    return true;
                }
            }

        }

        closedir(dr);

        return false;

    }


    void Search()
    {
        string src=cmdtokens[1];
        string directory=string(CWD);

        if(SearchUtil(directory,src))
        {
            response="TRUE : FOUND IN CURRENT DIRECTORY";
        }
        else
        {
            response="FALSE : NOT FOUND IN CURRENT DIRECTORY";
        }
    }




///----------------------MOVE AND RENAME-----------------------------//



    void MoveUtil(string source, string destination)
    {
        rename(source.c_str(),destination.c_str());
    }


    void Move()
    {
        int n=cmdtokens.size();
        string dest= PathResolver(cmdtokens[n-1]);
        string source;
        string destination;


        for(int i=1;i<n-1;i++)
        {
            source= PathResolver(cmdtokens[i]);
            destination=dest+"/"+ FilenameExtractor(cmdtokens[i]);

            MoveUtil(source,destination);
        }
    }


    void RenameUtil(string source, string destination)
    {
        rename(source.c_str(),destination.c_str());
    }

    void Rename()
    {
        int n=cmdtokens.size();
        if(n!=3)
        {
            response="INVALID NUMBER OF ARGUMENTS";
            return ;
        }
        string destination= PathResolver(cmdtokens[n-1]);
        string source= PathResolver(cmdtokens[1]);
        RenameUtil(source,destination);
    }

    void GotoUtil(string src)
    {
        chdir(src.c_str());

    }

    void Goto()
    {
        while(!NextDir.empty())
        {
            NextDir.pop();
        }
        PrevDir.push(string(CWD));
        string source= PathResolver(cmdtokens[1]);
        GotoUtil(source);

    }

///---------------------------------CREATE--------------------------------------------//

void CreateFileUtility(string filename, string destination )
{
    destination+="/"+filename;
    int out=open(destination.c_str(),O_WRONLY|O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);
    if(out<0)
    {
        response="ERROR: NO FILE CREATED";
        return;
    }
    close(out);

}

void CreateFile()
{
    int n=cmdtokens.size();
    string dest= PathResolver(cmdtokens[n-1]);
    response="FILE CREATED SUCCESSFULLY";
    if(n==2)
    {
        string filename= cmdtokens[1];
        string destination = string(CWD);
        CreateFileUtility(filename,destination);
        return;


    }
    string tempname;

    for(int i=1;i<n-1;i++)
    {
        tempname=cmdtokens[i];
        CreateFileUtility(tempname,dest);

    }
}

void CreateDirUtility(string dirname, string destination)
{
    destination+="/"+dirname;

    int out=mkdir(destination.c_str(),S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IXOTH);

    if(out!=0)
    {
        response="ERROR: INVALID DIRECTORY NAME";
    }

}

void CreateDir()
{
    int n=cmdtokens.size();
    string dest= PathResolver(cmdtokens[n-1]);
    response="DIRECTORY CREATED SUCCESSFULLY";

    if(n==2)
    {
        string dirname= cmdtokens[1];
        string destination=string(CWD);
        CreateDirUtility(dirname,destination);
        return;


    }
    string tempname;

    for(int i=1;i<n-1;i++)
    {
        tempname=cmdtokens[i];
        CreateDirUtility(tempname,dest);

    }

}








///------------------------------COMMAND MODE------------------------------------------///
void CmdRefresh()
{
    clearScreen();
    DisplayCurDir();

    printCWD();
    printMode(Status);
    printResponse(response);


    T.cx=T.s_rows;T.cy=1;
    mvcursor(T.s_rows,1);

    cout<<main_command;
    fflush(stdout);
}

void TokenMaker()
{
    cmdtokens.clear();
    string space_delimiter = " ";

    size_t pos = 0;
    while ((pos = main_command.find(space_delimiter)) != string::npos)
    {
        cmdtokens.push_back(main_command.substr(0, pos));
        main_command.erase(0, pos + space_delimiter.length());
    }
    main_command="";
}





void FinalizeTokens()
{
    for(int i=1;i<cmdtokens.size();i++)
    {
        cmdtokens[i]= PathResolver(cmdtokens[i]);
    }
}


void ExecuteCommand()
{
    response="";

    if(cmdtokens[0]=="delete")
    {
        Del();
    }
    else if(cmdtokens[0]=="copy")
    {
        Copy();
    }
    else if(cmdtokens[0]=="create_file")
    {
        CreateFile();
    }
    else if(cmdtokens[0]=="create_dir")
    {
        CreateDir();
    }
    else if( cmdtokens[0]=="goto")
    {
        Goto();
    }
    else if(cmdtokens[0]=="search")
    {
        Search();
    }
    else if(cmdtokens[0]=="move")
    {
        Move();
    }
    else if(cmdtokens[0]=="rename")
    {
        Rename();
    }
    else
    {
        response="INVALID COMMAND";
    }



}


void CommandModeMain()
{   CmdRefresh();




    char input=' ';

    while(input!=27)
    {
        read(STDIN_FILENO,&input,1);



        switch(input)
        {
            case 10 : {
                main_command.push_back(' ');
                TokenMaker();
                ExecuteCommand();
                break;
            }

            case 127 : {
                if(main_command!="")
                    main_command.pop_back();
                break;
            }

            case 'q' : {
                if(main_command=="")
                {
                    NormalModeDisable();
                    exit(0);
                }

            }


            default :{
                main_command.push_back(input);
            }

        }

        CmdRefresh();

    }




}


void CommandModeSwitcher()
{
    int tempx=T.cx,tempy=T.cy;
    Status="COMMAND MODE";

    CommandModeMain();
    T.cx=tempx;T.cy=tempy;
    mvcursor(T.cx,T.cy);
    Status="NORMAL MODE";

}



//---------------------------------------------------------------------------------------------
void refresh()
{
    clearScreen();
    DisplayCurDir();
    printCWD();
    printMode(Status);

    cout<<"\033[" << T.cx << ";" << T.cy << "H";
    fflush(stdout);

}



void statusprint()
{
    cout<<"CWD:"<<CWD<<endl;
    cout<<"HomeDir:"<<HomeDir<<endl;  // store main program directory

    cout<<"cursor_row:"<<T.cx<<endl;
    cout<<"cusor_col:"<<T.cy<<endl;
    cout<<"OFFSET:"<<offset<<endl;
    cout<<"TOP:"<<top<<endl;
    cout<<"TERMINAL HEIGHT:"<<T.s_rows<<endl;
    cout<<"TERMINAL WIDTH:"<<T.s_cols<<endl;

    cout<<"COMMAND TOKENS:"<<cmdtokens[0]<<"  "<<cmdtokens[1]<<" "<<cmdtokens[2]<<endl;
}
//---------------------------------------------------------------------------------------------------------
int main()
{   clearScreen();
    init();

    /*cout<<CWD<<endl;
    chdir("..");
    chdir("..");
    chdir("./terminal-based-file-explorer");
    getcwd(CWD,256);

    cout<<CWD;*/

    NormalModeEnable();
    char c=' ';
    while(c!='q')
    {
        read(STDIN_FILENO,&c,1);
        switch(c){
            case ':': CommandModeSwitcher();

            case 65: {
                cursor_mover('w');
                refresh();
                break;
            }

            case 66: {
                cursor_mover('s');
                refresh();
                break;
            }

            case 67: {
                GoForward();
                refresh();
                break;
            }

            case 68 : {
                GoBackward();
                refresh();
                break;
            }

            case 104:{
                Home();
                refresh();
                break;
            }

            case 127:{
                LevelUp();
                refresh();
                break;
            }

            case 'w':
            case 's':
            case 'd':
            case 'a': {
                cursor_mover(c);
                refresh();
                break;
            }
            case 'k':{
                ScrollUp();
                refresh();
                break;
            }

            case 'l':{
                ScrollDown();
                refresh();
                break;
            }

            case 'p':
                statusprint();
                break;
            case 10:
                EnterByCursor();
                refresh();
                break;
            default:
                break;
        }
    }

    atexit(NormalModeDisable);




    return 0;
}