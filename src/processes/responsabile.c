#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common/logger.h"
#include "common/config.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    
    LOG_INFO("RESPONSABILE", "Processo Responsabile avviato (PID: %d)", getpid());

    // 1. Parse Config
    // 2. Setup Shared Memory & Semaphores
    // 3. Exec altri processi
    
    return 0;
}