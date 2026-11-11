#include "cli.h"
#include "pcb.h"
#include "batch_loader.h"
#include "metrics_gantt.h"
#include <ctype.h>

#define MAX_INPUT_LEN 256
#define MAX_ARGS 16

static void print_help(void) {
    printf("\n==================================================\n");
    printf("         COMANDOS DISPONIBLES EN LA CLI\n");
    printf("==================================================\n");
    printf(" create <nombre> <burst> [prioridad]\n");
    printf("   Crea un nuevo proceso y lo ingresa a LISTOS.\n");
    printf(" load <archivo>\n");
    printf("   Carga masiva de procesos desde un archivo de lote.\n");
    printf(" ps\n");
    printf("   Muestra la tabla con el estado de todos los procesos.\n");
    printf(" pcb <PID>\n");
    printf("   Muestra el detalle del PCB del proceso indicado.\n");
    printf(" set_sched <RR|SRTF> [quantum]\n");
    printf("   Cambia la politica de planificacion (en caliente).\n");
    printf(" set_speed <ms_por_tick>\n");
    printf("   Ajusta la velocidad del reloj discreto (50-5000 ms).\n");
    printf(" start\n");
    printf("   Inicia o reanuda la simulación del procesador.\n");
    printf(" pause\n");
    printf("   Pausa temporalmente el avance del reloj de CPU.\n");
    printf(" gantt\n");
    printf("   Imprime el Diagrama de Gantt acumulado hasta ahora.\n");
    printf(" stats\n");
    printf("   Muestra las estadísticas globales de ejecucion.\n");
    printf(" help\n");
    printf("   Muestra esta lista de comandos disponibles.\n");
    printf(" exit\n");
    printf("   Detiene los hilos limpiamente y sale del programa.\n");
    printf("==================================================\n");
}

static void cmd_ps(ProcessQueue* all_procs, Process* current_proc) {
    printf("\nPID   NOMBRE           ESTADO        BURST RESTANTE   PRIORIDAD\n");
    printf("--------------------------------------------------------------\n");

    queue_lock(all_procs);
    Process* curr = all_procs->head;
    if (!curr) {
        printf("[No hay procesos registrados en el sistema]\n");
    }

    while (curr) {
        const char* st = pcb_state_to_string(curr->state);
        // Si es el proceso actual en CPU
        if (current_proc && curr->pid == current_proc->pid && curr->state != PROCESS_STATE_TERMINATED) {
            st = "RUNNING";
        }
        printf("%-5d %-16s %-13s %ds              %d\n",
               curr->pid, curr->name, st, curr->remaining_time, curr->priority);
        curr = curr->next;
    }
    queue_unlock(all_procs);
    printf("\n");
}

static void cmd_pcb(ProcessQueue* all_procs, const char* pid_str, Process* current_proc) {
    // Validar que sea un número entero positivo
    for (int i = 0; pid_str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)pid_str[i])) {
            printf("[ERROR DE TIPO]: El PID debe ser un numero entero positivo.\n");
            return;
        }
    }

    int pid = atoi(pid_str);
    if (pid <= 0) {
        printf("[ERROR DE TIPO]: El PID debe ser un numero entero positivo.\n");
        return;
    }

    queue_lock(all_procs);
    Process* found = NULL;
    Process* curr = all_procs->head;
    while (curr) {
        if (curr->pid == pid) {
            found = curr;
            break;
        }
        curr = curr->next;
    }

    if (!found) {
        queue_unlock(all_procs);
        printf("[ERROR]: No existe ningun proceso con PID %d en el sistema.\n", pid);
        return;
    }

    // Si está actualmente en ejecución, asegurar que refleje RUNNING
    if (current_proc && found->pid == current_proc->pid && found->state != PROCESS_STATE_TERMINATED) {
        found->state = PROCESS_STATE_RUNNING;
    }

    pcb_print_details(found);
    queue_unlock(all_procs);
}

void cli_run(CpuScheduler* scheduler, ProcessQueue* all_processes) {
    char input[MAX_INPUT_LEN];
    printf("\n==================================================================\n");
    printf("  SIMULADOR DE KERNEL Y SCHEDULER CLI - SISTEMAS OPERATIVOS UCAB\n");
    printf("  Escriba 'help' para ver la lista de comandos disponibles.\n");
    printf("==================================================================\n\n");

    while (1) {
        printf("kernel-cli> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break; // EOF
        }

        // Quitar salto de línea
        input[strcspn(input, "\r\n")] = '\0';

        // Parsear argumentos
        char* args[MAX_ARGS];
        int argc = 0;
        char* token = strtok(input, " \t");
        while (token && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " \t");
        }

        if (argc == 0) {
            continue;
        }

        const char* cmd = args[0];

        if (strcmp(cmd, "exit") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'exit' no requiere argumentos.\n");
                continue;
            }
            printf("[CLI]: Deteniendo el sistema de forma limpia...\n");
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "start") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'start' no requiere argumentos.\n");
                continue;
            }
            scheduler_start(scheduler);
        } else if (strcmp(cmd, "pause") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'pause' no requiere argumentos.\n");
                continue;
            }
            scheduler_pause(scheduler);
        } else if (strcmp(cmd, "ps") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'ps' no requiere argumentos adicionales.\n");
                continue;
            }
            cmd_ps(all_processes, scheduler_get_current(scheduler));
        } else if (strcmp(cmd, "pcb") == 0) {
            if (argc != 2) {
                printf("[ERROR DE SINTAXIS]: Parametros insuficientes.\nUso correcto: pcb <PID>\n");
                continue;
            }
            cmd_pcb(all_processes, args[1], scheduler_get_current(scheduler));
        } else if (strcmp(cmd, "create") == 0) {
            if (argc < 3 || argc > 4) {
                printf("[ERROR DE SINTAXIS]: Parametros insuficientes.\nUso correcto: create <nombre> <burst_time> [prioridad]\n");
                continue;
            }

            const char* name = args[1];
            // Validar burst
            bool burst_valid = true;
            for (int i = 0; args[2][i] != '\0'; i++) {
                if (!isdigit((unsigned char)args[2][i])) { burst_valid = false; break; }
            }
            int burst = burst_valid ? atoi(args[2]) : 0;
            if (!burst_valid || burst <= 0) {
                printf("[ERROR DE TIPO]: La rafaga (burst) debe ser un entero positivo mayor a 0.\n");
                continue;
            }

            int priority = DEFAULT_PRIORITY;
            if (argc == 4) {
                bool prio_valid = true;
                for (int i = 0; args[3][i] != '\0'; i++) {
                    if (!isdigit((unsigned char)args[3][i])) { prio_valid = false; break; }
                }
                priority = prio_valid ? atoi(args[3]) : -1;
                if (!prio_valid || priority < MIN_PRIORITY || priority > MAX_PRIORITY) {
                    printf("[ERROR DE PARAMETRO]: La prioridad debe ser un entero entre 1 y 5.\n");
                    continue;
                }
            }

            int arrival = scheduler_get_tick(scheduler);
            Process* proc = pcb_create(name, burst, priority, arrival);
            if (proc) {
                proc->state = PROCESS_STATE_READY;
                queue_enqueue(scheduler->ready_queue, proc);
                queue_enqueue(all_processes, proc);
                printf("[OK]: Proceso '%s' creado con PID %d (Burst: %d, Prioridad: %d, Llegada: tick %d) e ingresado a LISTOS.\n",
                       proc->name, proc->pid, proc->burst_time, proc->priority, proc->arrival_time);
            }
        } else if (strcmp(cmd, "load") == 0) {
            if (argc != 2) {
                printf("[ERROR DE SINTAXIS]: Falta ruta del archivo.\nUso correcto: load <ruta_archivo>\n");
                continue;
            }
            batch_load_file(args[1], scheduler->ready_queue, all_processes);
        } else if (strcmp(cmd, "set_sched") == 0) {
            if (argc < 2 || argc > 3) {
                printf("[ERROR DE SINTAXIS]: Parametros incorrectos.\nUso correcto: set_sched <RR|SRTF> [quantum]\n");
                continue;
            }

            SchedulerPolicy pol;
            if (strcmp(args[1], "RR") == 0) {
                pol = POLICY_RR;
            } else if (strcmp(args[1], "SRTF") == 0) {
                pol = POLICY_SRTF;
            } else {
                printf("[ERROR DE PARAMETRO]: Algoritmo no reconocido. Algoritmos validos: RR, SRTF.\n");
                continue;
            }

            int quantum = 2; // Por defecto según requerimiento
            if (argc == 3) {
                bool q_valid = true;
                for (int i = 0; args[2][i] != '\0'; i++) {
                    if (!isdigit((unsigned char)args[2][i])) { q_valid = false; break; }
                }
                quantum = q_valid ? atoi(args[2]) : 0;
                if (!q_valid || quantum <= 0) {
                    printf("[ERROR DE PARAMETRO]: El quantum debe ser un numero entero positivo.\n");
                    continue;
                }
            }
            scheduler_set_policy(scheduler, pol, quantum);
        } else if (strcmp(cmd, "set_speed") == 0) {
            if (argc != 2) {
                printf("[ERROR DE SINTAXIS]: Falta el parametro de milisegundos.\nUso correcto: set_speed <ms_por_tick>\n");
                continue;
            }
            bool speed_valid = true;
            for (int i = 0; args[1][i] != '\0'; i++) {
                if (!isdigit((unsigned char)args[1][i])) { speed_valid = false; break; }
            }
            int ms = speed_valid ? atoi(args[1]) : 0;
            if (!speed_valid || ms < 50 || ms > 5000) {
                printf("[ERROR DE PARAMETRO]: Velocidad invalida (%s). Debe ser un entero entre 50 y 5000 ms.\n", args[1]);
                continue;
            }
            scheduler_set_speed(scheduler, ms);
        } else if (strcmp(cmd, "gantt") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'gantt' no requiere argumentos.\n");
                continue;
            }
            gantt_print(&scheduler->gantt_tracker, scheduler_get_tick(scheduler));
        } else if (strcmp(cmd, "stats") == 0) {
            if (argc > 1) {
                printf("[ERROR]: El comando 'stats' no requiere argumentos.\n");
                continue;
            }
            stats_print(all_processes, scheduler_get_tick(scheduler), scheduler->gantt_tracker.total_idle_ticks);
        } else {
            printf("[ERROR]: Comando '%s' no reconocido. Escriba 'help' para ver la lista de comandos válidos.\n", cmd);
        }
    }
}
