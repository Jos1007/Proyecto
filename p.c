#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NOMBRE 60
#define MAX_TRAMITE 80
#define ARCHIVO_DATOS "expedientes.dat"

typedef enum {
    RIDICULO_BAJO    = 1,
    RIDICULO_MEDIO   = 2,
    RIDICULO_ALTO    = 3,
    RIDICULO_EXTREMO = 4
} NivelRidiculez;

typedef enum {
    PENDIENTE   = 0,
    APROBADO    = 1,
    RECHAZADO   = 2,
    EN_REVISION = 3
} EstadoTramite;

typedef struct {
    int  id;
    char nombre[MAX_NOMBRE];
    int  edad;
    int  activo;
} Ciudadano;

typedef struct {
    int            id;
    int            id_ciudadano;
    char           descripcion[MAX_TRAMITE];
    NivelRidiculez ridiculez;
    int            urgencia;
    float          costo;
    EstadoTramite  estado;
    float          puntuacion;
} Solicitud;

typedef struct {
    int           id_solicitud;
    EstadoTramite estado_final;
    float         puntuacion;
    char          motivo[100];
    long          timestamp;
} Resolucion;

void limpiar_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int leer_entero(const char *msg, int min, int max) {
    int v;
    while (1) {
        printf("%s", msg);
        if (scanf("%d", &v) == 1 && v >= min && v <= max) {
            limpiar_buffer();
            return v;
        }
        printf("  [!] Entrada invalida. Rango [%d - %d].\n", min, max);
        limpiar_buffer();
    }
}

float leer_flotante(const char *msg, float min, float max) {
    float v;
    while (1) {
        printf("%s", msg);
        if (scanf("%f", &v) == 1 && v >= min && v <= max) {
            limpiar_buffer();
            return v;
        }
        printf("  [!] Valor invalido. Rango [%.2f - %.2f].\n", min, max);
        limpiar_buffer();
    }
}

int registrar_ciudadano(Ciudadano **lista, int *n, int *cap) {
    if (*n >= *cap) {
        int nueva_cap = (*cap == 0) ? 4 : (*cap * 2);
        Ciudadano *tmp = realloc(*lista, nueva_cap * sizeof(Ciudadano));
        if (!tmp) {
            fprintf(stderr, "Error: realloc fallo en ciudadanos\n");
            return -1;
        }
        *lista = tmp;
        *cap = nueva_cap;
    }

    Ciudadano *c = &(*lista)[*n];
    c->id = *n + 1;
    c->activo = 1;

    printf("Nombre: ");
    if (!fgets(c->nombre, MAX_NOMBRE, stdin)) return -1;
    c->nombre[strcspn(c->nombre, "\n")] = 0;
    if (c->nombre[0] == '\0') strcpy(c->nombre, "Anonimo");

    c->edad = leer_entero("Edad: ", 1, 120);
    (*n)++;
    printf("  [OK] Ciudadano #%d registrado.\n", c->id);
    return c->id;
}

int buscar_ciudadano_por_id(Ciudadano *lista, int n, int id) {
    for (int i = 0; i < n; i++) {
        if (lista[i].id == id && lista[i].activo) return i;
    }
    return -1;
}

void listar_ciudadanos(Ciudadano *lista, int n) {
    printf("\n--- CIUDADANOS ---\n");
    for (int i = 0; i < n; i++) {
        if (!lista[i].activo) continue;
        printf("  #%d  %-25s  %d anios\n", lista[i].id, lista[i].nombre, lista[i].edad);
    }
}

float calcular_puntuacion(Solicitud *s) {
    float rid  = (float)s->ridiculez / 4.0f;
    float urg  = (float)s->urgencia  / 10.0f;
    float cost = s->costo / 1000.0f;
    if (cost > 1.0f) cost = 1.0f;
    float econ = 1.0f - cost;
    return 40.0f * rid + 35.0f * urg + 25.0f * econ;
}

void clasificar_solicitud(Solicitud *s) {
    s->puntuacion = calcular_puntuacion(s);
    if (s->puntuacion >= 75.0f) s->estado = APROBADO;
    else if (s->puntuacion >= 45.0f) s->estado = EN_REVISION;
    else s->estado = RECHAZADO;

    if (s->costo > 5000.0f && s->estado == APROBADO) {
        s->estado = EN_REVISION;
    }
}

int crear_solicitud(Solicitud **sols, int *n, int *cap, int id_ciudadano) {
    if (*n >= *cap) {
        int nueva_cap = (*cap == 0) ? 4 : (*cap * 2);
        Solicitud *tmp = realloc(*sols, nueva_cap * sizeof(Solicitud));
        if (!tmp) {
            fprintf(stderr, "Error: realloc fallo en solicitudes\n");
            return -1;
        }
        *sols = tmp;
        *cap = nueva_cap;
    }

    Solicitud *s = &(*sols)[*n];
    s->id = *n + 1;
    s->id_ciudadano = id_ciudadano;

    printf("Descripcion del tramite: ");
    if (!fgets(s->descripcion, MAX_TRAMITE, stdin)) return -1;
    if (strchr(s->descripcion, '\n') == NULL) limpiar_buffer();
    s->descripcion[strcspn(s->descripcion, "\n")] = 0;

    s->ridiculez = (NivelRidiculez)leer_entero("Ridiculez (1=Bajo 2=Medio 3=Alto 4=Extremo): ", 1, 4);
    s->urgencia  = leer_entero("Urgencia (1-10): ", 1, 10);
    s->costo     = leer_flotante("Costo (0-99999): ", 0, 99999);

    clasificar_solicitud(s);
    (*n)++;
    printf("  [OK] Solicitud #%d creada. Estado: %d, Score: %.2f\n", s->id, s->estado, s->puntuacion);
    return s->id;
}

int buscar_solicitud_por_id(Solicitud *sols, int n, int id) {
    for (int i = 0; i < n; i++) {
        if (sols[i].id == id) return i;
    }
    return -1;
}

static void agregar_resolucion(Resolucion **hist, int *nh, int *caph, int id_sol, EstadoTramite est, float score, const char *motivo) {
    if (*nh >= *caph) {
        int nc = (*caph == 0) ? 4 : (*caph * 2);
        Resolucion *tmp = realloc(*hist, nc * sizeof(Resolucion));
        if (!tmp) {
            fprintf(stderr, "Error: realloc fallo en historial\n");
            return;
        }
        *hist = tmp;
        *caph = nc;
    }

    Resolucion *r = &(*hist)[*nh];
    r->id_solicitud = id_sol;
    r->estado_final = est;
    r->puntuacion   = score;
    strncpy(r->motivo, motivo, sizeof(r->motivo) - 1);
    r->motivo[sizeof(r->motivo) - 1] = 0;
    r->timestamp    = (long)time(NULL);
    (*nh)++;
}

void resolver_solicitudes(Solicitud *sols, int n, Resolucion **hist, int *nh, int *caph) {
    const char *motivos[] = {
        "Pendiente",
        "Score >= 75 y costo aceptable",
        "Score < 45",
        "Score intermedio o costo alto"
    };
    for (int i = 0; i < n; i++) {
        clasificar_solicitud(&sols[i]);
        agregar_resolucion(hist, nh, caph, sols[i].id, sols[i].estado, sols[i].puntuacion, motivos[sols[i].estado]);
    }
    printf("  [OK] %d solicitudes resueltas y archivadas en historial.\n", n);
}

void mostrar_estadisticas(Solicitud *sols, int n, Ciudadano *cs, int nc) {
    (void)cs;
    if (n == 0) {
        printf("  Sin solicitudes.\n");
        return;
    }

    int aprob = 0, rech = 0, rev = 0;
    float suma = 0.0f, max = -1.0f, min = 1e9f;

    for (int i = 0; i < n; i++) {
        switch (sols[i].estado) {
            case APROBADO:    aprob++; break;
            case RECHAZADO:   rech++;  break;
            case EN_REVISION: rev++;   break;
            default: break;
        }
        suma += sols[i].puntuacion;
        if (sols[i].puntuacion > max) max = sols[i].puntuacion;
        if (sols[i].puntuacion < min) min = sols[i].puntuacion;
    }

    printf("\n--- ESTADISTICAS ---\n");
    printf("  Total solicitudes: %d\n", n);
    printf("  Aprobadas:    %d (%.1f%%)\n", aprob, 100.0 * aprob / n);
    printf("  Rechazadas:   %d (%.1f%%)\n", rech,  100.0 * rech  / n);
    printf("  En revision:  %d (%.1f%%)\n", rev,   100.0 * rev   / n);
    printf("  Score promedio: %.2f\n", suma / n);
    printf("  Score maximo:   %.2f\n", max);
    printf("  Score minimo:   %.2f\n", min);
    printf("  Ciudadanos activos: %d\n", nc);
}

int guardar_todo(Ciudadano *cs, int nc, Solicitud *ss, int ns, Resolucion *hs, int nh) {
    FILE *f = fopen(ARCHIVO_DATOS, "wb");
    if (!f) {
        perror("fopen guardar");
        return -1;
    }

    if (fwrite(&nc, sizeof(int), 1, f) != 1 ||
        fwrite(&ns, sizeof(int), 1, f) != 1 ||
        fwrite(&nh, sizeof(int), 1, f) != 1) {
        perror("fwrite cabecera");
        fclose(f);
        return -1;
    }

    if (nc && fwrite(cs, sizeof(Ciudadano), nc, f) != (size_t)nc) { fclose(f); return -1; }
    if (ns && fwrite(ss, sizeof(Solicitud), ns, f) != (size_t)ns) { fclose(f); return -1; }
    if (nh && fwrite(hs, sizeof(Resolucion), nh, f) != (size_t)nh) { fclose(f); return -1; }

    if (fclose(f) != 0) {
        perror("fclose");
        return -1;
    }

    printf("  [OK] Datos guardados (%d ciudadanos, %d solicitudes, %d resoluciones).\n", nc, ns, nh);
    return 0;
}

int cargar_todo(Ciudadano **cs, int *nc, int *capc, Solicitud **ss, int *ns, int *caps, Resolucion **hs, int *nh, int *caph) {
    FILE *f = fopen(ARCHIVO_DATOS, "rb");
    if (!f) {
        printf("  (Sin archivo previo, iniciando vacio)\n");
        return 0;
    }

    int t_nc = 0, t_ns = 0, t_nh = 0;
    if (fread(&t_nc, sizeof(int), 1, f) != 1 ||
        fread(&t_ns, sizeof(int), 1, f) != 1 ||
        fread(&t_nh, sizeof(int), 1, f) != 1) {
        fclose(f);
        return -1;
    }

    if (t_nc < 0 || t_nc > 100000 || t_ns < 0 || t_ns > 1000000 || t_nh < 0 || t_nh > 1000000) {
        fclose(f);
        return -1;
    }

    int nueva_capc = t_nc ? t_nc : 4;
    int nueva_caps = t_ns ? t_ns : 4;
    int nueva_caph = t_nh ? t_nh : 4;

    Ciudadano *nuevo_cs = NULL;
    Solicitud *nuevo_ss = NULL;
    Resolucion *nuevo_hs = NULL;

    if (t_nc) {
        nuevo_cs = malloc(nueva_capc * sizeof(Ciudadano));
        if (!nuevo_cs) { fclose(f); return -1; }
        if (fread(nuevo_cs, sizeof(Ciudadano), t_nc, f) != (size_t)t_nc) {
            free(nuevo_cs); fclose(f); return -2;
        }
    }

    if (t_ns) {
        nuevo_ss = malloc(nueva_caps * sizeof(Solicitud));
        if (!nuevo_ss) { free(nuevo_cs); fclose(f); return -1; }
        if (fread(nuevo_ss, sizeof(Solicitud), t_ns, f) != (size_t)t_ns) {
            free(nuevo_cs); free(nuevo_ss); fclose(f); return -2;
        }
    }

    if (t_nh) {
        nuevo_hs = malloc(nueva_caph * sizeof(Resolucion));
        if (!nuevo_hs) { free(nuevo_cs); free(nuevo_ss); fclose(f); return -1; }
        if (fread(nuevo_hs, sizeof(Resolucion), t_nh, f) != (size_t)t_nh) {
            free(nuevo_cs); free(nuevo_ss); free(nuevo_hs); fclose(f); return -2;
        }
    }

    if (fclose(f) != 0) {
        free(nuevo_cs); free(nuevo_ss); free(nuevo_hs);
        perror("fclose");
        return -1;
    }

    *cs = nuevo_cs; *nc = t_nc; *capc = nueva_capc;
    *ss = nuevo_ss; *ns = t_ns; *caps = nueva_caps;
    *hs = nuevo_hs; *nh = t_nh; *caph = nueva_caph;

    printf("  [OK] Datos cargados: %d ciudadanos, %d solicitudes, %d resoluciones.\n", *nc, *ns, *nh);
    return 0;
}

void liberar_memoria(Ciudadano *cs, Solicitud *ss, Resolucion *hs) {
    free(cs);
    free(ss);
    free(hs);
}

static void menu(void) {
    printf("\n===== MINISTERIO DE TRAMITES ABSURDOS =====\n");
    printf(" 1. Registrar ciudadano\n");
    printf(" 2. Listar ciudadanos\n");
    printf(" 3. Crear solicitud\n");
    printf(" 4. Resolver solicitudes (generar historial)\n");
    printf(" 5. Ver estadisticas\n");
    printf(" 6. Guardar datos\n");
    printf(" 7. Salir (guarda automaticamente)\n");
}

int main(void) {
    Ciudadano  *ciudadanos  = NULL; int n_ciud = 0, cap_ciud = 0;
    Solicitud  *solicitudes = NULL; int n_sol  = 0, cap_sol  = 0;
    Resolucion *historial   = NULL; int n_hist = 0, cap_hist = 0;

    cargar_todo(&ciudadanos, &n_ciud, &cap_ciud,
                &solicitudes, &n_sol, &cap_sol,
                &historial, &n_hist, &cap_hist);

    int opcion = 0;
    do {
        menu();
        opcion = leer_entero("Opcion: ", 1, 7);

        switch (opcion) {
            case 1:
                registrar_ciudadano(&ciudadanos, &n_ciud, &cap_ciud);
                break;

            case 2:
                listar_ciudadanos(ciudadanos, n_ciud);
                break;

            case 3: {
                if (n_ciud == 0) {
                    printf("  [!] Primero registre ciudadanos.\n");
                    break;
                }
                listar_ciudadanos(ciudadanos, n_ciud);
                int id = leer_entero("ID ciudadano: ", 1, n_ciud);
                int pos = buscar_ciudadano_por_id(ciudadanos, n_ciud, id);
                if (pos < 0) {
                    printf("  [!] ID invalido.\n");
                    break;
                }
                crear_solicitud(&solicitudes, &n_sol, &cap_sol, id);
                break;
            }

            case 4:
                if (n_sol == 0) {
                    printf("  [!] Sin solicitudes.\n");
                    break;
                }
                resolver_solicitudes(solicitudes, n_sol,
                                     &historial, &n_hist, &cap_hist);
                break;

            case 5:
                mostrar_estadisticas(solicitudes, n_sol, ciudadanos, n_ciud);
                break;

            case 6:
                guardar_todo(ciudadanos, n_ciud, solicitudes, n_sol,
                             historial, n_hist);
                break;

            case 7:
                guardar_todo(ciudadanos, n_ciud, solicitudes, n_sol,
                             historial, n_hist);
                printf("Saliendo...\n");
                break;
        }
    } while (opcion != 7);

    liberar_memoria(ciudadanos, solicitudes, historial);
    return 0;
}
