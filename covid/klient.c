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

struct Stock{
   short pid_seller;      //PID sprzedawcy
   short num_signal;      //oczekiwany sygnał
   short id_stock;        //id towaru
};

int manageStockAmount(int argc, char* argv[]);
int readNumOfNonBlankLines(char* path);
char** saveAllLines(char* path, char **allLines);
char* pickRandomFifo(char **allLines, int num_lines);
char* getRandomPathToFifo(char* path);
void openFifoFile(char* path, int stockAmount);

int main(int argc, char* argv[]) {
//    signal( SIGTERM, SIG_IGchar **allLinesN);			//ignorowanie sygnalow term i usr1 przez kontroler
//    signal(SIGUSR1, SIG_IGN);
    if(argc<2)
    {
        printf("Proszę wprowadzić parametry wedlug wzorca: \n "
               "- -k<ilosc towarow> <sciezka do pliku konfiguracyjnego>\n "
               "- <sciezka do pliku konfiguracyjnego> -k<ilosc towarow>\n " "- <sciezka do pliku konfiguracyjnego>\n");
    }
    char *conf_file;
    if(argv[1][0]=='-')
        conf_file= argv[2];
    else
        conf_file=argv[1];

    int num_stock = manageStockAmount(argc, argv);
    char* pickedFifo = getRandomPathToFifo(conf_file);
    printf("wybrana sciezka: %s, towar: %d\n", pickedFifo, num_stock);
    openFifoFile(pickedFifo, num_stock);
    return 0;
}

int manageStockAmount(int argc, char* argv[])
{
    char opt;
    while((opt=getopt(argc, argv, "k::")) != -1)
    {
        switch(opt)
            case 'k':
                return (int)strtol(optarg, (char **)NULL, 10);
    }
    return 1;
}
int readNumOfNonBlankLines(char* path)
{
    char letter[1];
    int one_char = 0;
    int num_lines=0;
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if(fd<0) {
        perror("[klient]Nie udało sie otworzyć pliku konfiguracyjnego!");
        exit(EXIT_FAILURE);
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
char** saveAllLines(char* path, char **allLines)
{
    int noLines = readNumOfNonBlankLines(path);
    allLines = (char**) calloc (noLines, sizeof(char*));
    char letter[1];
    int num_chars = 0;
    int num_lines = 0;

    int fd = open(path, O_RDONLY /*O_NONBLOCK*/);
    if(fd<0) {
        perror("[klient] Nie udało sie otworzyć pliku konfiguracyjnego!");
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
char* pickRandomFifo(char **allLines, int num_lines)
{
    srand( time( NULL ) );
    int random_line = rand()%num_lines;
    return allLines[random_line];
}
char* getRandomPathToFifo(char* path)
{
    int num_allLines = readNumOfNonBlankLines(path);
    char **allLines = (char**)calloc(num_allLines, sizeof(char*));
    allLines = saveAllLines(path, allLines);
    return pickRandomFifo(allLines, num_allLines);
}
void openFifoFile(char* path, int stockAmount)
{
    int fd;
    struct stat str_stat;
    //umask(0);
    if((fd = mknod(path, S_IFIFO | 0777, 0))<0 && errno!=EEXIST)
    {
        perror("[Klient] Nie udalo sie utworzyc pliku FIFO\n");              //tworzę ale nie istnieje
        exit(EXIT_FAILURE);
    }

    if(!(stat(path, &str_stat) && str_stat.st_mode==S_IFIFO))
    {
        if ((fd = open(path, 0, O_RDONLY)) < 0)
        {
            perror("[Klient] Nie udalo sie otworzyc pliku FIFO\n");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < stockAmount; i++)
        {
            struct Stock *deliveredStock = (struct Stock *) malloc(sizeof(struct Stock));
            int num_bytes = 0;


            while( num_bytes==0 )
            {
                num_bytes = read(fd, deliveredStock, sizeof(*deliveredStock));
                if (num_bytes  == -1)
                {
                    perror("[Klient] Nie udalo sie odczytac towaru");
                    exit(EXIT_FAILURE);
                    printf("[Klient] Nie udalo sie odczytac towaru\n");
                }
            }

            printf("[Klient] Odebrano paczkę: ID - %d, SIG - %d, PID - %d\n", deliveredStock->id_stock,
                   deliveredStock->num_signal, deliveredStock->pid_seller);
        }
        close(fd);
    }
}




