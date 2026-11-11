#include "batch_loader.h"
#include <ctype.h>

static char* trim_whitespace(char* str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int batch_load_file(const char* filepath, ProcessQueue* destination_queue, ProcessQueue* all_processes_queue) {
    if (!filepath || !destination_queue) {
        printf("[ERROR]: Parámetros inválidos para carga por lotes.\n");
        return -1;
    }

    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("[ERROR]: No se pudo abrir el archivo '%s'. Verifique la existencia de la ruta.\n", filepath);
        return -1;
    }

    char line[256];
    int line_number = 0;
    int loaded_count = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char* trimmed = trim_whitespace(line);

        // Ignorar líneas vacías o comentarios que inician con '#'
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }

        char name[MAX_NAME_LEN];
        int burst = 0;
        int priority = DEFAULT_PRIORITY;
        int arrival = 0;

        // Parsear: NOMBRE, BURST_TIME, PRIORIDAD, ARRIVAL_TIME
        char* token = strtok(trimmed, ",");
        if (!token) continue;
        strncpy(name, trim_whitespace(token), MAX_NAME_LEN - 1);
        name[MAX_NAME_LEN - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) {
            printf("[ADVERTENCIA]: Línea %d incompleta (falta ráfaga). Omitida.\n", line_number);
            continue;
        }
        burst = atoi(trim_whitespace(token));

        token = strtok(NULL, ",");
        if (!token) {
            printf("[ADVERTENCIA]: Línea %d incompleta (falta prioridad). Omitida.\n", line_number);
            continue;
        }
        priority = atoi(trim_whitespace(token));

        token = strtok(NULL, ",");
        if (!token) {
            printf("[ADVERTENCIA]: Línea %d incompleta (falta arrival_time). Omitida.\n", line_number);
            continue;
        }
        arrival = atoi(trim_whitespace(token));

        if (burst <= 0) {
            printf("[ADVERTENCIA]: Línea %d con burst <= 0. Omitida.\n", line_number);
            continue;
        }

        Process* proc = pcb_create(name, burst, priority, arrival);
        if (proc) {
            proc->state = PROCESS_STATE_READY;
            queue_enqueue(destination_queue, proc);
            if (all_processes_queue) {
                // Registrar en lista global para consultas ps/pcb
                queue_enqueue(all_processes_queue, proc);
            }
            loaded_count++;
        }
    }

    fclose(file);
    printf("[BATCH]: Se cargaron e insertaron %d proceso(s) exitosamente desde '%s'.\n", loaded_count, filepath);
    return loaded_count;
}
