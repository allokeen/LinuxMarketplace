//
// Created by nik on 11.09.2020.

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

int id_stock = 0;
int stockList_size=1;

struct flyer{
    int pid_seller;           //PID sprzedawcy
    short int num_signal;     //oczekiwany sygnał
    int id_flyer;             //id
};
struct allFlyers_info{
    int how_much;
};

int* statistics = NULL;
int* allFIFOs = NULL;
int flyers_num=0;
struct allFlyers_info* allFlyersInfo = NULL;

void manageGetOpt(int argc, char* argv[], int* s, int* r);
int readNumOfNonBlankLines(char* path_file);
char** saveAllLines(char* path_file, char **allLines);
char ** arrayConfigurationFile(char* path_file);
int checkIfFifoFile(char* path_fifo);
void writeFifoFiles(char** allLines, int num_lines, int num_signal, int flyers_num);
void handlingSigRT(int flyers_num, siginfo_t *sa, void *context);
void handlingSigUSR();
char *mySprintf(long i, char b[]);

int main(int argc, char* argv[])
{
    int signal_num=0;
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handlingSigRT;
    sa.sa_flags = SA_SIGINFO;

    struct sigaction saUSR;
    memset(&saUSR, '\0', sizeof(saUSR));
    saUSR.sa_sigaction = handlingSigUSR;
    saUSR.sa_flags = 0;

    if( argc != 6) {
        perror("[ZŁA LICZBA PARAMETRÓW]\nProszę wprowadzić parametry wedlug wzorca: \n "
               "- <sciezka do pliku konfiguracyjnego> -r <ilosc reklam> -s <numer sygnału> \n "
               "- <sciezka do pliku konfiguracyjnego> -s <numer sygnału> -r <ilosc reklam>\n "
               "- -s <numer sygnału> <sciezka do pliku konfiguracyjnego> -r <ilosc reklam>\n"
               "- -s <numer sygnału> -r <ilosc reklam> <sciezka do pliku konfiguracyjnego>\n"
               "- -r <ilosc reklam> -s <numer sygnału> <sciezka do pliku konfiguracyjnego>\n"
               "- -r <ilosc reklam> <sciezka do pliku konfiguracyjnego> -s <numer sygnału> \n");
        exit(EXIT_FAILURE);
    }
    char *conf_file;
    if( (argv[2][1] == 's' && argv[4][1] == 'r') || (argv[4][1] == 's' && argv[2][1] == 'r') )
        conf_file = argv[1];
    else if( (argv[1][1] == 's' && argv[3][1] == 'r') || (argv[3][1] == 's' && argv[1][1] == 'r') )
        conf_file = argv[5];
    else if( (argv[1][1] == 's' && argv[4][1] == 'r') || (argv[4][1] == 's' && argv[1][1] == 'r') )
        conf_file = argv[3];

    manageGetOpt(argc, argv, &signal_num, &flyers_num );
    //printf("S: %d, R: %d", signal_num, flyers_num);
    sigaction(signal_num, &sa, NULL);
    sigaction(SIGUSR1, &saUSR, NULL);;
    char **allLines = arrayConfigurationFile(conf_file);
    int num_allLines = readNumOfNonBlankLines(conf_file);
    writeFifoFiles(allLines, num_allLines, signal_num, flyers_num);
}

void manageGetOpt(int argc, char* argv[], int* s, int* r)
{
    char opt;
    while((opt=getopt(argc, argv, "s:r:")) != -1)
    {
        switch(opt)
        {
            case 's':
            {
                int arg = (int) strtol(optarg, (char **) NULL, 10);
                if( arg>64 || arg<34) {
                    printf("Sygnal musi miescic się w przedziale <32,63>\n");
                }
                else
                    *s = arg;
                break;
            }
            case 'r':
            {
                int arg = (int) strtol(optarg, (char **) NULL, 10);
                *r = arg;
                break;
            }
            case '?':
            {
                perror("Nie podano wartosci parametru!\n");
                exit(EXIT_FAILURE);
            }
            default:
            {
                perror("[ZŁA LICZBA PARAMETRÓW]\nProszę wprowadzić parametry wedlug wzorca: \n "
                       "- <sciezka do pliku konfiguracyjnego> -r <ilosc reklam> -s <numer sygnału> \n "
                       "- <sciezka do pliku konfiguracyjnego> -s <numer sygnału> -r <ilosc reklam>\n "
                       "- -s <numer sygnału> <sciezka do pliku konfiguracyjnego> -r <ilosc reklam>\n"
                       "- -s <numer sygnału> -r <ilosc reklam> <sciezka do pliku konfiguracyjnego>\n"
                       "- -r <ilosc reklam> -s <numer sygnału> <sciezka do pliku konfiguracyjnego>\n"
                       "- -r <ilosc reklam> <sciezka do pliku konfiguracyjnego> -s <numer sygnału> \n");
                exit(EXIT_FAILURE);
            }
        }
    }
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
        perror("Nie udało sie otworzyć pliku konfiguracyjnego!");
        exit(EXIT_FAILURE);
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
void writeFifoFiles(char** allLines, int num_lines, int num_signal, int flyers_num)
{
   struct flyer new_flyer;
    new_flyer.pid_seller = getpid();
    new_flyer.num_signal = num_signal;
    int tablica[flyers_num];
    int one = 1;
    for(int i=0; i<flyers_num; i++) {
        tablica[i] = one;
        one = one + 2;
    }
    int index = rand()%flyers_num;
    new_flyer.id_flyer = tablica[index];

    struct timespec  ts, ts1;
    ts1.tv_nsec = 500000000;
    ts1.tv_sec = 0;
    ts.tv_nsec = 500000000;
    ts.tv_sec = 0;

    statistics = (int*)calloc(flyers_num,sizeof(int));
    allFlyersInfo = (struct allFlyers_info*)malloc(num_lines*sizeof(struct allFlyers_info));
    allFIFOs = (int*)malloc(sizeof(int));
    int allFIFOs_size = 1;
    while(1)
    {
        for(int i=0; i<num_lines; i++)
        {
            if (checkIfFifoFile(allLines[i]))
            {
                //num_ofFifos++;
                int fd = open(allLines[i], O_WRONLY | O_NONBLOCK);
                if (fd > 0)
                {
                    if(i>0)
                        allFIFOs = (int*)realloc(allFIFOs, allFIFOs_size*sizeof(int));
                    allFIFOs[i] = fd;
                    allFIFOs_size++;
                }
            }
        }
        for (int i = 0; i < allFIFOs_size-1; i++)
        {
            if ( allFIFOs[i] > 0)       //JEŻELI NIEPUSTY //todo
            {
                int bytes_written = -1;
                while(bytes_written==-1)
                    bytes_written = write(allFIFOs[i], &new_flyer, sizeof(new_flyer));
                if (bytes_written >0)
                {
                    close(allFIFOs[i]);
                    printf("Wyslano reklame %d do %s", new_flyer.id_flyer, allLines[i]);
                    int index = rand()%flyers_num;
                    new_flyer.id_flyer = tablica[index];
                }
                nanosleep(&ts1, &ts);
            }
        }
    }
}
void handlingSigRT(int flyers_num, siginfo_t *sa, void *context)
{
        statistics[flyers_num]+=1;
}
void handlingSigUSR()
{
    write(1, "\n--------------------Raport--------------------", sizeof("\n--------------------Raport--------------------"));
    write(1, "\nWydane reklamy: \n(ID) --- LICZBA\n", sizeof("\nWydane reklamy: \n(ID) --- LICZBA\n"));
    for( int i=0; i<flyers_num; i++)
    {
        char number[4];
        mySprintf(2*i+1, number);

        char idStock[4];
        mySprintf(statistics[i], idStock);
        write(1, number, sizeof(number)/sizeof(char));
        write(1, "   ", sizeof("   "));
        write(1, idStock, sizeof(idStock)/sizeof(char));
        write(1, "\n", sizeof("\n"));
    }
    write(1, "\n---------------Koniec Raportu---------------\n", sizeof("\n---------------Koniec Raportu---------------\n"));
}

char *mySprintf(long i, char b[])
{
    char const number[] = "0123456789";
    char* p = b;
    if(i<0)
    {
        *p++ = '-';
        i *= -1;
    }
    long x = i;
    do
    {
        ++p;
        x = x/10;
    }while(x);

    *p = '\0';
    do
    {
        *--p = number[i%10];
        i = i/10;
    }while(i);
    return b;
}