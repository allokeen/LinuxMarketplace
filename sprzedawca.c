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

int id_stock = 0;
volatile int stockList_size=1;

struct stock{
    int pid_seller;           //PID sprzedawcy
    short int num_signal;     //oczekiwany sygnał
    int id_stock;             //id towaru
};
struct allFIFOs_info{
    int fd;
    int czy_wyslano_raz;
    int czy_zaplacono;
};
struct allStock_info{
    int fd_nr;
    int czy_zaplacono;
};
struct allFIFOs_info* allFIFOsInfo = NULL;
struct allStock_info* allStockInfo = NULL;

int manageGetOptSignal(int argc, char* argv[]);
int readNumOfNonBlankLines(char* path_file);
char** saveAllLines(char* path_file, char **allLines);
char ** arrayConfigurationFile(char* path_file);
int checkIfFifoFile(char* path_fifo);
void writeFifoFiles(char** allLines, int num_lines, int num_signal);
int toDeleteFile();
void handlingSigRT(int numer, siginfo_t *sa, void *context);
void handlingSigUSR();
char *mySprintf(long i, char b[]);

int main(int argc, char* argv[])
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handlingSigRT;
    sa.sa_flags = SA_SIGINFO;

    struct sigaction saUSR;
    memset(&saUSR, '\0', sizeof(saUSR));
    saUSR.sa_sigaction = handlingSigUSR;
    saUSR.sa_flags = 0;
    sigaction(SIGUSR1, &saUSR, NULL);;

    if(argc!=4) {
        printf("Proszę wprowadzić parametry wedlug wzorca: \n "
               "- -s <numer sygnalu> <sciezka do pliku konfiguracyjnego>\n "
               "- <sciezka do pliku konfiguracyjnego> -s <numer sygnalu>\n ");
        exit(EXIT_FAILURE);
    }
    char *conf_file;
    if(argv[1][0]=='-')
        conf_file= argv[3];
    else
        conf_file= argv[1];

    int num_signal = manageGetOptSignal(argc, argv );
    sigaction(num_signal, &sa, NULL);
    char **allLines = arrayConfigurationFile(conf_file);
    int num_allLines = readNumOfNonBlankLines(conf_file);
    writeFifoFiles(allLines, num_allLines, num_signal);
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
                if( arg>64 || arg<34) {
                    printf("Sygnal musi miescic się w przedziale <32,63>\n");
                    exit(EXIT_FAILURE);
                }
                else
                    return arg;
            }
            case '?':
            {
                perror("Nie podano numeru sygnalu RealTime!\n");
                exit(EXIT_FAILURE);
            }
            default:
            {
                printf("Proszę wprowadzić parametry wedlug wzorca: \n "
                       "- -s <numer sygnalu> <sciezka do pliku konfiguracyjnego>\n "
                       "- <sciezka do pliku konfiguracyjnego> -s <numer sygnalu>\n ");
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
void writeFifoFiles(char** allLines, int num_lines, int num_signal)
{
    struct stock new_stock;
    struct timespec  ts, ts1;
    ts1.tv_nsec = 400000000;
    ts1.tv_sec = 0;
    ts.tv_nsec = 400000000;
    ts.tv_sec = 0;
    new_stock.pid_seller = getpid();
    new_stock.num_signal = num_signal;
    new_stock.id_stock = id_stock;

    allFIFOsInfo = (struct allFIFOs_info*)malloc(num_lines*sizeof(struct allFIFOs_info));
    while(1) {
        while (1) {
            for (int i = 0; i < num_lines; i++) {
                if (checkIfFifoFile(allLines[i])) {
                    int fd = open(allLines[i], O_WRONLY | O_NONBLOCK);
                    if (fd > 0) {
                        allFIFOsInfo[i].fd = fd;
                        if (allFIFOsInfo[i].czy_wyslano_raz != 1) {
                            allFIFOsInfo[i].czy_wyslano_raz = 0;
                            allFIFOsInfo[i].czy_zaplacono = 0;
                        }
                    } else if (fd < 0) {
                        allFIFOsInfo[i].fd = 0;
                        allFIFOsInfo[i].czy_wyslano_raz = 0;
                        allFIFOsInfo[i].czy_zaplacono = 0;
                        if (toDeleteFile()) {
                            unlink(allLines[i]);
                        }
                    }
                }
            }

            for (int i = 0; i < num_lines; i++) {
                if (allFIFOsInfo[i].fd > 0 &&
                    (allFIFOsInfo[i].czy_wyslano_raz == 0 || allFIFOsInfo[i].czy_zaplacono == 1)) {
                    int bytes_written;
                    if ((bytes_written = write(allFIFOsInfo[i].fd, &new_stock, sizeof(new_stock))) < 0)
                        perror("[sprzedawca] Nie udalo sie wyslac towaru");
                    else if (bytes_written == 0)
                        printf("[sprzedawca] Nic nie udalo sie zapisac do pliku FIFO");
                    else {
                        if (stockList_size == 1)
                            allStockInfo = (struct allStock_info *) malloc(
                                    stockList_size * sizeof(struct allStock_info));
                        else
                            allStockInfo = (struct allStock_info *) realloc(allStockInfo, stockList_size *
                                                                                          sizeof(struct allStock_info));

                        allStockInfo[stockList_size - 1].fd_nr = i;
                        allStockInfo[stockList_size - 1].czy_zaplacono = 0;
                        allFIFOsInfo[i].czy_zaplacono = 0;
                        allFIFOsInfo[i].czy_wyslano_raz = 1;

                        close(allFIFOsInfo[i].fd);
                        printf("\nWydano %dkb (numer %d) do %s\n", bytes_written, id_stock, allLines[i]);
                        id_stock += 2;
                        new_stock.id_stock = id_stock;
                        stockList_size++;
                        nanosleep(&ts1, &ts);
                        //kill(0,SIGUSR1);
                    }
                }
            }
        }
    }
}

int toDeleteFile()
{
    srand( time( NULL ) );
    int death = rand()%50;     //szansa 1/50 na usunięcie
    return (death==0 ? 1 : 0);
}

void handlingSigRT(int numer, siginfo_t *sa, void *context)
{
    int index = sa->si_value.sival_int/2;
    int zaplaconyFIFO = allStockInfo[index].fd_nr;
    allStockInfo[index].czy_zaplacono = 1;
    allFIFOsInfo[zaplaconyFIFO].czy_zaplacono = 1;
}

void handlingSigUSR()
{
    write(1, "\n--------------------Raport--------------------", sizeof("\n--------------------Raport--------------------"));
    write(1, "\nWydano towarów: ", sizeof("\nWydano towarów: "));
    char towar[4];
    mySprintf(id_stock/2,towar);
    write(1, towar, sizeof(towar)/sizeof(char));
    write(1, "\nNieopłacone towary (ID): \n", sizeof("\nNieopłacone towary (ID): \n"));
    for( int i=0; i<=id_stock/2; i++)
        if( allStockInfo[i].czy_zaplacono == 0)
        {
            char idStock[4];
            mySprintf(i*2, idStock);
            write(1, idStock, sizeof(idStock)/sizeof(char));
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