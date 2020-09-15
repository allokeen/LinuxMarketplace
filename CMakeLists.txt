cmake_minimum_required(VERSION 3.15)
project(covid C)

set(CMAKE_C_STANDARD 99)

add_executable(covid main.c klient.c agent.c sprzedawca.c start.c start.h confFileFunctions.cpp confFileFunctions.h Hsprze.c obslugaPliku.c obslugaPliku.h)