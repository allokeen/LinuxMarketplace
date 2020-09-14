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

struct Flyer{
    int pid_agent;      //PID sprzedawcy
    int num_signal;      //oczekiwany sygnał
    int id_flyer;        //id towaru
};

int readNumOfNonBlankLines(char* path);
char** saveAllLines(char* path, char **allLines);

int main(int argc, char* argv[]) {
    if(argc<4)
    {
        printf("Proszę wprowadzić parametry wedlug wzorca: \n");
    }
    char** all_paths = saveAllLines();
    return 0;
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
