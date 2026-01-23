# Sincronizzazione Barriera con Semaforo (SEM_INDEX_BARRIER)

## Panoramica

Questo documento descrive il pattern di sincronizzazione a barriera implementato nel progetto "Oasi del Golfo" utilizzando il semaforo `SEM_INDEX_BARRIER` per coordinare la fine della giornata lavorativa tra il processo Responsabile e tutti i processi figli attivi (Utenti, Operatori, Cassa).

## Pattern di Sincronizzazione

### Descrizione del Meccanismo

Il semaforo `SEM_INDEX_BARRIER` implementa una **barriera di sincronizzazione** per garantire che tutti i processi abbiano completato le loro operazioni giornaliere prima che il Responsabile generi il report finale.

### Processi Coinvolti

#### Lettore del Semaforo (Processo in Attesa)

**File:** `src/processes/responsabile.c` (linea 906)

Il processo **Responsabile** esegue un'operazione di **WAIT** (lettura) sul semaforo:

```c
struct sembuf sb = {SEM_INDEX_BARRIER, -1, 0};
struct timespec timeout = {g_config.day_end_barrier_wait_sec, 0};

if (semtimedop(g_sem_id, &sb, 1, &timeout) == -1) {
    // Gestione timeout o errori
}
```

**Operazione:** `sem_op = -1` (decrementa il semaforo, attende se valore è 0)

**Semantica:** Il Responsabile attende che ogni processo figlio attivo segnali la propria terminazione. Esegue questa operazione `active_children` volte (una per ogni processo attivo).

#### Scrittori del Semaforo (Processi Segnalanti)

I seguenti processi eseguono un'operazione di **SIGNAL** (scrittura) sul semaforo per notificare il loro completamento:

##### 1. Processo Utente

**File:** `src/processes/utente.c` (linea 78)

```c
void signal_end_of_day() {
  struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
  semop(g_sem_id, &sb, 1);
}
```

**Quando:** Chiamata alla fine del ciclo giornaliero dell'utente, dopo aver completato tutte le transazioni.

##### 2. Processo Operatore

**File:** `src/processes/operatore.c` (linea 103)

```c
void signal_end_of_day() {
  struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
  semop(g_sem_id, &sb, 1);
}
```

**Quando:** Chiamata quando l'operatore riceve il segnale di fine giornata e completa le operazioni pendenti.

##### 3. Processo Cassa

**File:** `src/processes/cassa.c` (linee 175, 216)

```c
if (g_day_signal) {
  struct sembuf sb = {SEM_INDEX_BARRIER, 1, 0};
  semop(g_sem_id, &sb, 1);
  g_day_signal = 0;
}
```

**Quando:** Chiamata quando la cassa riceve il segnale di fine giornata, sia durante l'attesa attiva che durante la ricezione bloccante di messaggi.

**Operazione:** `sem_op = +1` (incrementa il semaforo, sblocca un processo in attesa)

## Flusso di Sincronizzazione

### Sequenza Operativa

1. **Inizializzazione:** Il Responsabile inizializza `SEM_INDEX_BARRIER` a 0 (linea 220 in `responsabile.c`):
   ```c
   init_sem(g_sem_id, SEM_INDEX_BARRIER, 0);
   ```

2. **Fine Giornata:** Il Responsabile invia segnale `SIGUSR1` a tutti i processi figli per notificare la fine della giornata.

3. **Segnalazione dai Figli:** Ogni processo figlio (Utente, Operatore, Cassa) completa le operazioni correnti e chiama la propria funzione `signal_end_of_day()` o equivalente, incrementando il semaforo di 1.

4. **Attesa del Responsabile:** Il Responsabile esegue `active_children` operazioni di wait sul semaforo:
   - Ogni `semtimedop(..., -1, ...)` decrementa il semaforo di 1
   - Se il semaforo è 0, il processo si blocca fino a quando un figlio lo incrementa
   - Usa timeout per evitare attese infinite

5. **Sblocco Progressivo:** Man mano che i processi figli segnalano la fine, il Responsabile viene sbloccato e può procedere con:
   - Generazione del report giornaliero
   - Stampa delle statistiche
   - Eventuale inizio di un nuovo giorno

## Gestione dei Timeout

Il Responsabile usa `semtimedop` invece di `semop` per evitare blocchi indefiniti:

```c
struct timespec timeout = {g_config.day_end_barrier_wait_sec, 0};
```

**Comportamento in caso di timeout:**
- Se un processo figlio non segnala entro `day_end_barrier_wait_sec` secondi, `semtimedop` ritorna con `errno = EAGAIN`
- Il Responsabile logga un errore e forza lo shutdown del sistema
- Questo previene deadlock se un processo figlio termina prematuramente o si blocca

## Vantaggi di Questo Approccio

1. **Coerenza dei Dati:** Garantisce che tutte le statistiche siano completate prima della lettura
2. **Prevenzione Race Condition:** Evita letture parziali o inconsistenti durante la generazione del report
3. **Robustezza:** Il timeout previene blocchi indefiniti in caso di errori
4. **Scalabilità:** Funziona con un numero variabile di processi attivi

## Riferimenti nel Codice

| File | Linea | Operazione | Descrizione |
|------|-------|------------|-------------|
| `responsabile.c` | 220 | Inizializzazione | `init_sem(g_sem_id, SEM_INDEX_BARRIER, 0)` |
| `responsabile.c` | 906 | Wait (lettura) | `semtimedop(..., -1, ...)` - Attesa segnali dai figli |
| `utente.c` | 78 | Signal (scrittura) | `semop(..., +1, ...)` - Notifica fine operazioni utente |
| `operatore.c` | 103 | Signal (scrittura) | `semop(..., +1, ...)` - Notifica fine turno operatore |
| `cassa.c` | 175, 216 | Signal (scrittura) | `semop(..., +1, ...)` - Notifica fine operazioni cassa |

## Diagramma Concettuale

```
Responsabile (LETTORE)                    Processi Figli (SCRITTORI)
     |                                          |
     |  1. Invia SIGUSR1                       |
     |-----------------------------------> Utente 1
     |                                     Utente 2
     |                                     Operatore 1
     |                                     Cassa 1
     |                                          |
     |  2. Wait su SEM_INDEX_BARRIER            |
     |     (semtimedop, sem_op=-1)              |
     |                                          |
     |                                     3. Completano operazioni
     |                                          |
     |  <--------------------------------- signal_end_of_day()
     |  <--------------------------------- (semop, sem_op=+1)
     |  <--------------------------------- 
     |  <--------------------------------- 
     |                                          |
     |  4. Tutti i wait completati              |
     |                                          |
     |  5. Genera report finale                 |
     |                                          |
```

## Note di Implementazione

- Il contatore `active_children` determina quante volte il Responsabile deve attendere
- Ogni processo figlio deve chiamare la signal **esattamente una volta** per giornata
- Il valore iniziale del semaforo è 0, quindi il primo wait si blocca fino al primo signal
- La barriera si "azzera" ad ogni ciclo giornaliero con il reset del semaforo
