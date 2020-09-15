
//
// Created by just on 13.02.2020.
//

#include "start.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CLIENTS 3
void setEnvironmentVariables();
void createPrograms();

//int main()
//{
//    setEnvironmentVariables();
//    createPrograms();
//
//    return 0;
//}
//
//void setEnvironmentVariables()
//{
//    const char* path = "../FIFO_files";
//
//    char* recruit_dly = (char*) calloc (7, sizeof(char));
//    gcvt(1.0, 2, recruit_dly);
//
//    char* recruit_max = (char*) calloc (7, sizeof(char));
//    gcvt(3.0, 1, recruit_max);
//
//    char* ring_reg = (char*) calloc (7, sizeof(char));
//    gcvt(1.0, 2, ring_reg);
//
//    char* totem_op = (char*) calloc (7, sizeof(char));
//    gcvt(1.0, 2, totem_op);
//
//    char* totem_cl = (char*) calloc (7, sizeof(char));
//    gcvt(1.0, 2, totem_cl);
//
//
//
//    if(setenv("CAMP", path, 0) == -1)
//    {
//        printf("Cannot set CAMP\n");
//        exit(-1);
//    }
//
//    if(setenv("RECRUIT_DLY", recruit_dly, 0) == -1)
//    {
//        printf("Cannot set RECRUIT_DLY\n");
//        exit(-1);
//    }
//
//    if(setenv("RECRUIT_MAX", recruit_max, 0) == -1)
//    {
//        printf("Cannot set RECRUIT_DLY\n");
//        exit(-1);
//    }
//
//    if(setenv("RING_REG", ring_reg, 0) == -1)
//    {
//        printf("Cannot set RING_REG\n");
//        exit(-1);
//    }
//
//    if(setenv("TOTEM_OP", totem_op, 0) == -1)
//    {
//        printf("Cannot set TOTEM_OP\n");
//        exit(-1);
//    }
//
//    if(setenv("TOTEM_CL", totem_cl, 0) == -1)
//    {
//        printf("Cannot set TOTEM_CL\n");
//        exit(-1);
//    }
//}
//
//
//void createPrograms()
//{
//
//    // forkowanie
//    switch(fork())
//    {
//        case -1:
//        {
//            printf("Fork1 error\n");
//            exit(-1);
//        }
//
//        case 0:
//        {
//            if (execl("../serwer/serwer", "serwer", (char*)NULL) == -1 )
//            {
//                printf("execl1 error\n");
//                exit(-1);
//            }
//
//            break;
//        }
//
//        default:
//            break;
//    }
//
//    for(int i = 0; i < CLIENTS; i++)
//    {
//        switch(fork())
//        {
//            case -1:
//            {
//                printf("fork2 error\n");
//                exit(-1);
//            }
//
//            case 0:
//            {
//                if (execl("../klient/klient", "klient", (char*)NULL) == -1 )
//                {
//                    printf("execl2 error\n");
//                    exit(-1);
//                }
//
//                break;
//            }
//
//            default:
//                break;
//
//        }
//    }
//}
