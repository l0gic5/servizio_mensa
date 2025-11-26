# Progetto Finale SO - Servizio Mensa

---

## Struttura del Progetto

```
.
├── src/              codice sorgente (hash_table.c, main.c)
├── test/             Unit test (hash_table_test.c) e dataset
├── unity/
├── docs/             documentazione
├── bin/              output eseguibili (main_ex2, test_ex2)
├── build/            output file oggetto (*.o)
├── makefile
└── README.md
```

---

## Build & Esecuzione

### Compilazione

Per compilare l'intero progetto:

```bash
make
```

### Esecuzione Unit Test

Per eseguire la suite di test di correttezza (compila e lancia `...`):

```bash
make test
```

### Esecuzione Programma Principale

```bash
$ ./bin/...
```

**Esempio di utilizzo:**

```bash
$ ./bin/...
```

### Pulizia

Per rimuovere tutti i file compilati (`bin/`, `build/`):

```bash
make clean
```

---

## Generazione Documentazione

Per generare la documentazione del codice sorgente (richiede Doxygen):

```bash
make docs
```

La documentazione sarà disponibile in `docs/html/index.html`.


---

## Autore

**André Marguerettaz**

_UNITO - Sistemi Operativi 2025-2026_
[andre.marguerettaz@edu.unito.it](mailto:andre.marguerettaz@edu.unito.it)
