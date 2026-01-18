# Guida al File di Configurazione (`.conf`)

Il comportamento della simulazione è controllato interamente a runtime. Il parser (`config.c`) legge le coppie `CHIAVE=VALORE` e popola la struttura globale `Config`.

> **Nota Tecnica:** Se un parametro viene omesso o se il file non esiste, il sistema utilizza automaticamente i **Valori di Default** (definiti in `set_default_values` in `config.c`) per garantire la stabilità dell'esecuzione.

---

## 1. Simulazione Globale (Tempo e Limiti)

Controlla il "motore del tempo" della simulazione.

| Chiave Config (`.conf`)        | Variabile C                | Descrizione                                                                      | Default               |
| ------------------------------ | -------------------------- | -------------------------------------------------------------------------------- | --------------------- |
| **`SIM_DURATION`**             | `simulation_duration_days` | Numero totale di giorni virtuali da simulare.                                    | `30`                  |
| **`N_NANO_SECS`**              | `n_nanosecs_as_minute`     | Durata reale (ns) di 1 minuto virtuale.                                          | `100.000.000` (100ms) |
| **`DAILY_SERVICE_MINUTES`**    | `daily_service_minutes`    | Minuti virtuali di apertura mensa giornaliera.                                   | `120`                 |
| **`SYSTEM_STARTUP_DELAY_SEC`** | `system_startup_delay_sec` | Secondi di attesa all'avvio (spawn processi).                                    | `2`                   |
| **`OVERLOAD_THRESHOLD`**       | `overload_threshold`       | Max utenti respinti/in coda a fine giornata prima dell'arresto per sovraccarico. | `50`                  |

---

## 2. Popolazione e Risorse Fisiche

Dimensionamento degli attori e della capacità strutturale.

| Chiave Config           | Variabile C         | Descrizione                                                                 | Default |
| ----------------------- | ------------------- | --------------------------------------------------------------------------- | ------- |
| **`NOF_USERS`**         | `nof_users`         | Numero di processi Utente generati all'avvio.                               | `50`    |
| **`NOF_WORKERS`**       | `nof_workers`       | Numero totale di processi Operatore (inclusa Cassa).                        | `6`     |
| **`NOF_TABLE_SEATS`**   | `nof_table_seats`   | Posti a sedere (semaforo tavoli) per consumare il pasto.                    | `40`    |
| **`AVG_USER_W_TICKET`** | `avg_user_w_ticket` | Percentuale (0-100) di utenti dotati di ticket (sconto + coda prioritaria). | `80`    |

---

## 3. Configurazione Stazioni (Code e Postazioni)

Definisce la capacità dei buffer (code) e il parallelismo dei worker.

### Code di Attesa (Capacità Semafori)

| Chiave Config              | Variabile C              | Descrizione                       | Default |
| -------------------------- | ------------------------ | --------------------------------- | ------- |
| **`NOF_WK_SEATS_PRIMI`**   | `queue_capacity_primi`   | Max utenti in coda per i Primi.   | `15`    |
| **`NOF_WK_SEATS_SECONDI`** | `queue_capacity_secondi` | Max utenti in coda per i Secondi. | `15`    |
| **`NOF_WK_SEATS_DOLCI`**   | `queue_capacity_dolci`   | Max utenti in coda per i Dolci.   | `15`    |
| **`NOF_WK_SEATS_CAFFE`**   | `queue_capacity_caffe`   | Max utenti in coda per Caffè.     | `20`    |
| **`NOF_WK_SEATS_CASSA`**   | `queue_capacity_cassa`   | Max utenti in coda alla Cassa.    | `20`    |

### Risorse Fisiche e Lettore Ticket

| Chiave Config                  | Variabile C                | Descrizione                                         | Default         |
| ------------------------------ | -------------------------- | --------------------------------------------------- | --------------- |
| **`TICKET_READER_CAPACITY`**   | `ticket_reader_capacity`   | Numero di lettori badge (parallelismo ingresso).    | `3`             |
| **`TICKET_READER_TIMEOUT_NS`** | `ticket_reader_timeout_ns` | Tempo impiegato per validare il ticket.             | `5.000.000`     |
| **`WORKSTATIONS_PRIMI`**       | `workstations_primi`       | Postazioni fisiche disponibili per servire Primi.   | `2`             |
| **`WORKSTATIONS_SECONDI`**     | `workstations_secondi`     | Postazioni fisiche disponibili per servire Secondi. | `2`             |
| **`WORKSTATIONS_CAFFE`**       | `workstations_caffe`       | Postazioni fisiche disponibili per Caffè/Dolci.     | `2`             |
| **`WORKSTATIONS_CASSA`**       | `workstations_cassa`       | Postazioni fisiche per la Cassa.                    | `2`             |
| **`MENU_FILE_PATH`**           | `menu_file_path`           | Percorso relativo del file menu.                    | `data/menu.txt` |

---

## 4. Metriche di Servizio (Tempi e Prezzi)

Parametri economici e temporali del servizio.

### Tempi di Servizio e Variabilità

| Chiave Config            | Variabile C          | [Type]                               | Descrizione                                         | Default |
| ------------------------ | -------------------- | ------------------------------------ | --------------------------------------------------- | ------- |
| **`AVG_SRVC_[TYPE]`**    | `avg_service_[type]` | `PRIMI`, `SECONDI`, `CAFFE`, `CASSA` | Tempo base (ns simulati) per servire una richiesta. | ~100ms  |
| **`VARIABILITY_[TYPE]`** | `variability_[type]` | `PRIMI`, `SECONDI`, `CAFFE`, `CASSA` | Variabilità percentuale (+/-) sul tempo base.       | 20-30%  |

### Prezzi e Sconti

| Chiave Config                 | Variabile C               | [Type]                               | Descrizione                                         | Default   |
| ----------------------------- | ------------------------- | ------------------------------------ | --------------------------------------------------- | --------- |
| **`PRICE_[TYPE]`**            | `price_[type]`            | `PRIMI`, `SECONDI`, `DOLCI`, `CAFFE` | Costo unitario (€).                                 | Variabile |
| **`TICKET_DISCOUNT_PERCENT`** | `ticket_discount_percent` |                                      | Percentuale di sconto applicata a chi ha il ticket. | `25.0`    |

---

## 5. Comportamento Operatori (Pause)

Configurazione della "resistenza" e delle abitudini lavorative.

| Chiave Config                   | Variabile C                 | Descrizione                                                   | Default       |
| ------------------------------- | --------------------------- | ------------------------------------------------------------- | ------------- |
| **`MAX_PAUSES_PER_DAY`**        | `max_pauses_per_day`        | Max pause per operatore al giorno.                            | `2`           |
| **`PAUSE_DURATION_NS`**         | `pause_duration_ns`         | Durata reale di una pausa (ns).                               | `300.000.000` |
| **`PAUSE_PROBABILITY_PERCENT`** | `pause_probability_percent` | Probabilità % che un operatore faccia pausa dopo un servizio. | `10`          |
| **`DAY_END_BARRIER_WAIT_SEC`**  | `day_end_barrier_wait_sec`  | Timeout attesa sincronizzazione fine giornata.                | `30`          |

---

## 6. Comportamento e Preferenze Utenti

Logica decisionale e impazienza dei clienti.

| Chiave Config                       | Variabile C                     | [Type]                               | Descrizione                                                          | Default         |
| :---------------------------------- | :------------------------------ | :----------------------------------- | :------------------------------------------------------------------- | :-------------- |
| **`USER_QUEUE_TIMEOUT_SEC`**        | `user_queue_timeout_sec`        |                                      | Max secondi reali attesa in coda prima di rinunciare (`semtimedop`). | `5`             |
| **`USER_MEAL_DURATION_NS`**         | `user_meal_duration_ns`         |                                      | Tempo occupazione tavolo per il pasto.                               | `2.000.000.000` |
| **`USER_COFFEE_DURATION_NS`**       | `user_coffee_duration_ns`       |                                      | Tempo consumo caffè al bancone.                                      | `10.000.000`    |
| **`USER_MAX_ARRIVAL_DELAY_US`**     | `user_max_arrival_delay_us`     |                                      | Ritardo max casuale (µs) ingresso giornaliero.                       | `9.600.000`     |
| **`PROBABILITY_USER_WANTS_[TYPE]`** | `probability_user_wants_[type]` | `PRIMO`, `SECONDO`, `DOLCE`, `CAFFE` | Probabilità ordinazione.                                             | Variabile       |
| **`USER_BUDGET_MIN`**               | `user_budget_min`               |                                      | Valore minimom del range iniziale budget utente (€).                 | `10`            |
| **`USER_BUDGET_MAX`**               | `user_budget_max`               |                                      | Valore massimo del range iniziale budget utente (€).                 | `50`            |
| **`USER_MIN_DAILY_SALARY`**         | `user_min_daily_salary`         |                                      | Budget minimo di ricarica giornaliera (€).                           | `5`             |
| **`USER_MAX_DAILY_SALARY`**         | `user_max_daily_salary`         |                                      | Budget massimo di ricarica giornaliera (€).                          | `15`            |
| **`MAX_GROUPS`**                    | `max_groups`                    |                                      | Numero massimo di gruppi sociali gestibili.                          | `30`            |
| **`MAX_USERS_PER_GROUP`**           | `max_users_per_group`           |                                      | Dimensione massima di un gruppo.                                     | `4`             |

---

## 7. Logistica (Rifornimenti e Menu)

Parametri per il refill delle scorte in cucina.

| Chiave Config                 | Variabile C               | [Type]                               | Descrizione                                          | Default      |
| ----------------------------- | ------------------------- | ------------------------------------ | ---------------------------------------------------- | ------------ |
| **`DAILY_[TYPE]_COUNT`**      | `daily_[type]_count`      | `PRIMI`, `SECONDI`, `DOLCI`, `CAFFE` | Varietà di piatti nel menu giornaliero.              | 3-5          |
| **`REFILL_INTERVAL_MINUTES`** | `refill_interval_minutes` |                                      | Intervallo (minuti virtuali) tra i refill periodici. | `10`         |
| **`REFILL_VARIANCE_PERCENT`** | `refill_variance_percent` |                                      | Variabilità quantità rifornita.                      | `20`         |
| **`AVG_REFILL_TIME_NS`**      | `avg_refill_time_ns`      |                                      | Tempo impiegato per rifornire (blocco cucina).       | `50.000.000` |
| **`AVG_REFILL_[TYPE]`**       | `avg_refill_[type]`       | `PRIMI`, `SECONDI`, `DOLCI`, `CAFFE` | Quantità media aggiunta ad ogni refill.              | Variabile    |
| **`MAX_PORZIONI_[TYPE]`**     | `max_porzioni_[type]`     | `PRIMI`, `SECONDI`, `DOLCI`, `CAFFE` | Capienza massima contenitori cibo.                   | 50-1000      |

---

## 8. Tools e Reporting

Configurazione funzionalità extra e output.

| Chiave Config                        | Variabile C                      | Descrizione                                                | Default             |
| ------------------------------------ | -------------------------------- | ---------------------------------------------------------- | ------------------- |
| **`DEFAULT_SCIOPERO_STOP_DURATION`** | `default_sciopero_stop_duration` | Durata default sciopero (sec) se non specificata da input. | `60`                |
| **`EXPORT_FOLDER_PATH`**             | `export_folder_path`             | Cartella destinazione report CSV.                          | `reports/`          |
| **`EXPORT_DAILY_REPORTS_CSV`**       | `export_daily_reports_csv`       | Abilita export CSV giornaliero (1/0 o true/false).         | `true`              |
| **`CREATE_DAILY_SINGLE_FILES`**      | `create_daily_single_files`      | Se true, crea un file separato per ogni giorno.            | `false`             |
| **`DAILY_REPORTS_FILENAME_CSV`**     | `daily_reports_filename_csv`     | Prefisso file report giornaliero.                          | `days/daily_report` |
| **`EXPORT_FINAL_STATS_CSV`**         | `export_final_stats_csv`         | Abilita export CSV finale cumulativo.                      | `true`              |
| **`FINAL_STATS_FILENAME_CSV`**       | `final_stats_filename_csv`       | Nome file report finale.                                   | `final_stats`       |
