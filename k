void handlingSigRT(int numer, siginfo_t *sa, void *context)
{
    int index = sa->si_value.sival_int;
    int zaplaconyFIFO = allStockInfo[index].fd_nr;
    allFIFOsInfo[zaplaconyFIFO].czy_zaplacono = 1;
}

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handlingSigRT;
    sa.sa_flags = SA_SIGINFO;
    sigaction(num_signal, &sa, NULL);