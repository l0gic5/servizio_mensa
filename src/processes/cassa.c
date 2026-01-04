#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/logger.h"
#include "common/config.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv; // Silenzia warning per parametri inutilizzati
    
    // Esempio di utilizzo del Logger
    LOG_INFO("CASSA", "Processo Cassa avviato (PID: %d)", getpid());

    // Qui andrà il loop principale della cassa
    // while(running) { ... }

    LOG_INFO("CASSA", "Terminazione processo");
    return 0;
}