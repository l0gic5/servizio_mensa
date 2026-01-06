# Guida al File di Configurazione (`.conf`)

Il comportamento della simulazione è controllato interamente tramite file di configurazione (es. `conf/default.conf`).
Il parser (`config.c`) legge queste chiavi e riempie la struttura `Config` definita in `common/config.h`.

> **Nota:** Se un parametro non è specificato nel file, viene utilizzato il suo **Valore di Default**.

---

## 1) Simulazione Globale (Tempo e Limiti)

Parametri che gestiscono lo scorrere del tempo, la durata della simulazione e le soglie di terminazione.

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`SIM_DURATION`** | `simulation_duration_days` | Numero di "giorni" virtuali da simulare. | `30` |
| **`N_NANO_SECS`** | `n_nanosecs_as_minute` | Nanosecondi reali corrispondenti a 1 minuto virtuale. | `100.000.000` (100ms) |
| **`DAILY_SERVICE_MINUTES`** | `daily_service_minutes` | Durata in minuti virtuali di un giorno lavorativo. | `120` (2 ore) |
| **`SYSTEM_STARTUP_DELAY_SEC`** | `system_startup_delay_sec` | Secondi reali di attesa all'avvio per lo spawn dei processi. | `2` |
| **`OVERLOAD_THRESHOLD`** | `overload_threshold` | Max utenti respinti/in coda a fine giorno prima di terminare per overload. | `10` |
| **`CASSA_POSITION`** | `cassa_position` | Posizione cassa: `INGRESSO` (paga subito) o `USCITA` (paga alla fine). | `INGRESSO` |

---

## 2) Popolazione e Risorse Fisiche

Definisce il numero di attori e la capacità della sala.

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`NOF_USERS`** | `nof_users` | Numero totale di processi Utente (clienti) lanciati. | `40` |
| **`NOF_WORKERS`** | `nof_workers` | Numero totale di processi Operatore (inclusa la cassa). | `6` |
| **`NOF_TABLE_SEATS`** | `nof_table_seats` | Numero totale di posti a sedere (tavoli) per mangiare. | `30` |

---

## 3) Configurazione Stazioni (Code e Postazioni)

Dimensionamento delle risorse per ogni tipologia di servizio.

> **Valori validi per `[TIPO]`: `PRIMI`, `SECONDI`, `CAFFE`, `CASSA`**

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`NOF_WK_SEATS_[TIPO]`** | `queue_capacity_[tipo]` | **Capacità Coda**: Max utenti in attesa. Se piena, l'utente va in timeout.<br>*(Es: `NOF_WK_SEATS_PRIMI`)* | `10` (Cibo)<br>`15` (Caffè/Cassa) |
| **`WORKSTATIONS_[TIPO]`** | `workstations_[tipo]` | **Postazioni Fisiche**: Numero di banconi dove gli operatori lavorano.<br>*(Es: `WORKSTATIONS_CASSA`)* | `5` (Cibo/Caffè)<br>`1` (Cassa) |

---

## 4) Metriche di Servizio (Time & Money)

Attributi relativi all'erogazione del servizio.

| Chiave Config (`.conf`) | Variabile Struct C | Suffix `[TIPO]` Accettati | Descrizione | Default |
| --- | --- | --- | --- | --- |
| **`AVG_SRVC_[TIPO]`** | `avg_service_[tipo]` | `PRIMI`<br>`SECONDI`<br>`CAFFE`<br>`CASSA` | Tempo medio base per servire una richiesta (ns simulati). | `5000` (Primi)<br>`6000` (Secondi)<br>`2000` (Caffè)<br>`3000` (Cassa) |
| **`VARIABILITY_[TIPO]`** | `variability_[tipo]` | `PRIMI`<br>`SECONDI`<br>`CAFFE`<br>`CASSA` | Percentuale (0-100) di variabilità random. | `50` (Cibo)<br>`80` (Caffè)<br>`10` (Cassa) |
| **`PRICE_[TIPO]`** | `price_[tipo]` | `PRIMI`<br>`SECONDI`<br>`CAFFE` | Prezzo di vendita del prodotto. | `5`, `8`, `1` |

---

## 5) Comportamento Operatori (Pause)

Configurazione della "resistenza" e delle abitudini lavorative degli operatori.

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`MAX_PAUSES_PER_DAY`** | `max_pauses_per_day` | Numero massimo di pause consentite per operatore al giorno. | `3` |
| **`PAUSE_DURATION_NS`** | `pause_duration_ns` | Durata di una pausa (in nanosecondi reali). | `0.5s` (5e8 ns) |
| **`PAUSE_PROBABILITY_PERCENT`** | `pause_probability_percent` | Probabilità % (0-100) che un operatore faccia pausa dopo un servizio. | `10` |

---

## 6) Comportamento e Preferenze Utenti

Rende la simulazione dinamica controllando le decisioni e la pazienza degli utenti.

> **Valori validi per `[TIPO]`: `PRIMO`, `SECONDO`, `CAFFE` (Notare il singolare)**

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`USER_QUEUE_TIMEOUT_SEC`** | `user_queue_timeout_sec` | Secondi reali max attesa in coda prima di rinunciare. | `2` |
| **`USER_MEAL_DURATION_NS`** | `user_meal_duration_ns` | Tempo impiegato per consumare il pasto (occupazione tavolo). | `0.05s` (5e7 ns) |
| **`USER_MAX_ARRIVAL_DELAY_US`** | `user_max_arrival_delay_us` | Ritardo max casuale (microsecondi) arrivo giornaliero. | `0.5s` (5e5 us) |
| **`PROBABILITY_USER_WANTS_[TIPO]`** | `probability_user_wants_[tipo]` | % Probabilità che un utente decida di ordinare quel piatto.<br>*(Es: `PROBABILITY_USER_WANTS_PRIMO`)* | `70` (Primo)<br>`60` (Secondo)<br>`30` (Caffè) |

---

## 7) Logistica (Rifornimenti)

Parametri per la gestione delle scorte (funzionalità future/avanzate).

> **Valori validi per `[TIPO]`: `PRIMI`, `SECONDI`**

| Chiave Config (`.conf`) | Variabile Struct C | Descrizione | Default |
| --- | --- | --- | --- |
| **`AVG_REFILL_[TIPO]`** | `avg_refill_[tipo]` | Tempo necessario per rifornire la stazione. | `50.000` |
| **`MAX_PORZIONI_[TIPO]`** | `max_porzioni_[tipo]` | Quantità massima di cibo disponibile prima del refill. | `100` |

---

Legenda File Utilizzatori 

* `responsabile.c`: **Processo Padre**. Inizializza i semafori, calcola la durata del giorno, spawna i figli. 


* `operatore.c`: **Processo Lavoratore**. Usa i tempi di servizio, pause e variabilità per servire le richieste. 


* `utente.c`: **Processo Cliente**. Usa le probabilità di scelta, i timeout code e i tempi pasto per simulare il flusso.