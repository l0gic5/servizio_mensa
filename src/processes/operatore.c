#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/logger.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    LOG_INFO("OPERATORE", "Processo Operatore avviato");
    
    return 0;
}