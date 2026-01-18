# Progetto "Oasi del Golfo" - Simulazione Mensa

**Corso:** Sistemi Operativi 2025/2026  
**Università:** Università degli Studi di Torino  
**Autore:** André Marguerettaz (Matricola: 1152060)

---

## Descrizione

Il progetto **"Oasi del Golfo"** è un'applicazione concorrente scritta in C (standard C99) che simula la gestione operativa di una mensa aziendale.
Il sistema orchestra processi indipendenti (`Utenti`, `Operatori`, `Cassa`, `Responsabile`) che competono per risorse limitate (cibo, tavoli, postazioni) utilizzando primitive **System V IPC** (*Semafori*, *Shared Memory*, *Message Queues*).

Il simulatore implementa algoritmi adattivi per la riallocazione della forza lavoro in base allo stress delle stazioni e gestisce scenari complessi come gruppi di utenti, priorità ticket, scioperi e refill delle scorte.

### Documentazione Completa

Per approfondimenti architetturali e dettagli sulla configurazione, consultare i documenti allegati:

* **📄 [Relazione Tecnica](./RELAZIONE.md)**: Dettagli su architettura, scelte implementative, algoritmi e IPC.
* **⚙️ [Guida alla Configurazione](./CONFIG.md)**: Manuale completo dei parametri per file `.conf`.

---

## Struttura del Progetto

```text
.
├── src/                  # Codice sorgente
│   ├── common/           # Librerie condivise (IPC, Logger, Config)
│   ├── processes/        # Processi principali (Responsabile, Utente, Worker)
│   └── executables/      # Tool esterni (Sciopero, Generatore)
├── include/              # Header files (.h)
├── test/                 # Unit Tests (Unity Framework)
├── conf/                 # File di configurazione (.conf)
├── data/                 # Dataset (Menu, Nomi)
├── docs/                 # Documentazione Doxygen
├── bin/                  # Eseguibili compilati
│   ├── common/
│   ├── processes/
│   └── executables/
├── build/                # File oggetto intermedi
├── makefile              # Script di build
└── README.md             # Questo file

```

---

## Build & Esecuzione

### Prerequisiti

* Ambiente Linux (o POSIX compliant con estensioni GNU).
* Compilatore `gcc`.
* `make`.

### Compilazione

Per compilare l'intero progetto (processi, tool e test):

```bash
make
```

### Esecuzione della Simulazione

Il punto di ingresso è il processo **Responsabile**. È possibile passare un file di configurazione specifico o usare quello di default.

**Scenario Standard:**

```bash
# `conf/default.conf` è la configurazione di default
./bin/processes/responsabile
```

**Scenario Stress Test (Overload):**

```bash
./bin/processes/responsabile conf/overload.conf
```

**Scenario Lunga Durata (Safe Load):**

```bash
./bin/processes/responsabile conf/safe_load.conf
```

---

## Strumenti Aggiuntivi

A simulazione avviata, è possibile interagire con il sistema tramite terminali separati:

1. **Indire uno sciopero (Communication Disorder):**

    Blocca temporaneamente specifici operatori.

    ```bash
    ./bin/executables/sciopero conf/default.conf
    ```

2. **Iniettare nuovi utenti:**

    Aggiunge clienti a simulazione in corso.

    ```bash
    ./bin/executables/generatore_utenti conf/default.conf 10
    ```

---

## Testing e Debug

### Esecuzione Unit Test

Per verificare la correttezza delle primitive IPC, del parser e delle statistiche:

```bash
make test
```

### Pulizia

Per rimuovere file oggetto, binari e file temporanei (IPC cleanup):

```bash
make clean
```

---

## Documentazione Codice

Il codice è commentato secondo lo standard **Doxygen**. Per generare la documentazione HTML navigabile:

```bash
make docs
```

Aprire `docs/html/index.html` nel browser per visualizzare call graph e descrizioni delle funzioni.

---

## Contatti

**André Marguerettaz** [andre.marguerettaz@edu.unito.it](mailto:andre.marguerettaz@edu.unito.it)

Repository: [https://github.com/l0gic5/servizio_mensa](https://github.com/l0gic5/servizio_mensa)
