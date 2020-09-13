//
// Created by nik on 11.09.2020.
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

int id_stock = 1;
struct stock{
    int pid_seller;      //PID sprzedawcy
    int num_signal;      //oczekiwany sygnał
    int id_stock;        //id towaru
};

int manageGetOptSignal(int argc, char* argv[]);
int readNumOfNonBlankLines(char* path_file);
char** saveAllLines(char* path_file, char **allLines);
char ** arrayConfigurationFile(char* path_file);
int checkIfFifoFile(char* path_fifo);
void checkAndWriteFifoFiles(char** allLines, int num_lines, int num_signal);
int toDeleteFile();

int main(int argc, char* argv[])
{
    if(argc<3)
        printf("Proszę wprowadzić parametry wedlug wzorca: \n "
               "- -s <numer sygnalu> <sciezka do pliku konfiguracyjnego>\n "
               "- <sciezka do pliku konfiguracyjnego> -s <numer sygnalu>\n ");
   char *conf_file;
    int num_signal;
   if(argv[1][0]=='-')
       conf_file= argv[3];
   else
       conf_file= argv[1];

   num_signal = manageGetOptSignal(argc, argv );
   char **allLines = arrayConfigurationFile(conf_file);
   int num_allLines = readNumOfNonBlankLines(conf_file);
   checkAndWriteFifoFiles(allLines, num_allLines, num_signal);
}

int manageGetOptSignal(int argc, char* argv[])
{
    char opt;
    while((opt=getopt(argc, argv, "s:")) != -1)
    {
        switch(opt)
        {
            case 's':
             {
                int arg = (int) strtol(optarg, (char **) NULL, 10);
                if( arg>64 || arg<34)
                    printf("Sygnal musi miescic się w przedziale <32,63>\n");
                else
                    return arg;
            }
            case '?':
            {
                printf("Nie podano numeru sygnalu RealTime!\n");
                break;
            }
        }
    }
    return 0;
}
int readNumOfNonBlankLines(char* path_file)
{
    char letter[1];
    int one_char = 0;
    int num_lines=0;
    int fd = open(path_file, O_RDONLY | O_NONBLOCK);
    if(fd<0) {
        //perror("Nie udało sie otworzyć pliku konfiguracyjnego!");
        return -1;
    }
    int first_letter=0;
    while( (one_char=read(fd, letter, 1))!=0)
    {
        first_letter++;
        if(letter[0] == '\n' && first_letter!=1)
        {
            num_lines++;
            first_letter=0;
            continue;
        }
        else if(letter[0] == '\n' && first_letter==1)
        {
            first_letter=0;
            continue;
        }
    }
    lseek(fd, 0, SEEK_SET);
    close(fd);
    return num_lines;
}
char** saveAllLines(char* path_file, char **allLines)
{
    int noLines = readNumOfNonBlankLines(path_file);
    allLines = (char**) calloc (noLines, sizeof(char*));
    char letter[1];
    int num_chars = 0;
    int num_lines = 0;

    int fd = open(path_file, O_RDONLY | O_NONBLOCK);
    if(fd<0) {
        //perror("Nie udało sie otworzyć pliku konfiguracyjnego!");
        return NULL;
    }
    while( read(fd, letter, 1) != 0  )
    {
        if(letter[0] == '\n')
        {
            allLines[num_lines] = (char*)malloc(num_chars * sizeof(char));
            num_chars = 0;
            num_lines++;
            continue;
        }
        num_chars++;
    }
    lseek(fd, 0, SEEK_SET);

    num_lines = 0;
    num_chars = 0;
    int first_letter=0;
    while( read(fd, letter, 1) != 0)
    {
        first_letter++;
        if(letter[0] == '\n' && first_letter!=1)
        {
            num_lines++;
            num_chars = 0;
            first_letter=0;
            continue;
        }
        else if((letter[0] == '\n' && first_letter==1) || (letter[0] == ' ' && first_letter==1))
        {
            first_letter=0;
            continue;
        }
        allLines[num_lines][num_chars] = letter[0];
        num_chars++;
    }
    lseek(fd, 0, SEEK_SET);
    close(fd);
    return allLines;
}
char ** arrayConfigurationFile(char* path_file)
{
    int num_allLines = readNumOfNonBlankLines(path_file);
    char **allLines = (char**)calloc(num_allLines, sizeof(char*));
    allLines = saveAllLines(path_file, allLines);
    return allLines;
}
int checkIfFifoFile(char* path_fifo)
{
    struct stat str_stat;
    return !(stat(path_fifo, &str_stat) && str_stat.st_mode==S_IFIFO);
}
void checkAndWriteFifoFiles(char** allLines, int num_lines, int num_signal)
{
    int *fd_fifos = (int*) calloc (num_lines, sizeof(int));
    struct stock new_stock;
    new_stock.pid_seller = getpid();
    new_stock.num_signal = num_signal;
    new_stock.id_stock = 1;
    int i;
    while(1) {
        for (i = 0; i < num_lines; i++)
            if (checkIfFifoFile(allLines[i])) {
                if ((fd_fifos[i] = open(allLines[i], O_WRONLY | O_NONBLOCK)) < 0)        //decyzja o usunięciu pliku
                {
                    if (toDeleteFile())
                        unlink(allLines[i]);
                }
                else if (fd_fifos[i] > 0)
                {
                    int bytes_written;
                    if ((bytes_written = write(fd_fifos[i], &new_stock, sizeof(new_stock))) < 0)
                        perror("[sprzedawca] Nie udalo sie wyslac towaru");
                    else if (bytes_written == 0)
                        printf("[sprzedawca] Nic nie udalo sie zapisac do pliku FIFO");
                    else
                    {
                        printf("Wydano %dkb do %s\n", bytes_written, allLines[i]);
                        id_stock +=1;
                        new_stock.id_stock = id_stock;
                        close(fd_fifos[i]);
                    }
                }
            }
    }
}
int toDeleteFile()
{
        srand( time( NULL ) );
        int death = rand()%10;     //szansa 1/10 na usunięcie
        return (death==0 ? 1 : 0);
}
