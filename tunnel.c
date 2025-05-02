#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_VOYAGES 20  // 10 allers + 10 retours = 20 voyages

sem_t acces_tunnel;  // bloque le tunnel pour un sens à la fois
sem_t file_x, file_y;  // pour que X et Y aient leur tour
int bus_x_dans_tunnel = 0;  // compte les bus X dans le tunnel
int bus_y_dans_tunnel = 0;  // compte les bus Y

// fonction pour entrer dans le tunnel
void entrer_tunnel(char ville) {
    if (ville == 'X') {
        sem_wait(&file_x);  // attends ton tour
        bus_x_dans_tunnel++;
        if (bus_x_dans_tunnel == 1) {
            sem_wait(&acces_tunnel);  // premier bus X prend le tunnel
        }
        sem_post(&file_x);  // libère pour les autres X
    } else {
        sem_wait(&file_y);
        bus_y_dans_tunnel++;
        if (bus_y_dans_tunnel == 1) {
            sem_wait(&acces_tunnel);  // premier bus Y prend le tunnel
        }
        sem_post(&file_y);
    }
}

// fonction pour sortir du tunnel
void sortir_tunnel(char ville) {
    if (ville == 'X') {
        sem_wait(&file_x);  // on touche au compteur
        bus_x_dans_tunnel--;
        if (bus_x_dans_tunnel == 0) {
            sem_post(&acces_tunnel);  // plus de bus X, on libère
        }
        sem_post(&file_x);
    } else {
        sem_wait(&file_y);
        bus_y_dans_tunnel--;
        if (bus_y_dans_tunnel == 0) {
            sem_post(&acces_tunnel);  // plus de bus Y, on libère
        }
        sem_post(&file_y);
    }
}

// ce que fait chaque bus
void* trajet_bus(void* arg) {
    int numero = *((int*)arg);
    char ville_origine = (numero <= NB_BUS_X) ? 'X' : 'Y';
    char ville_opposee = (ville_origine == 'X') ? 'Y' : 'X';

    for (int voyage = 1; voyage <= NB_VOYAGES; voyage++) {
        if (voyage % 2 == 1) {  // voyage impair = aller (X->Y ou Y->X)
            entrer_tunnel(ville_origine);
            printf("Bus %d de %c va de %c à %c (Aller, voyage %d)\n", numero, ville_origine, ville_origine, ville_opposee, voyage);
            usleep(1000000 + rand() % 500000);  // pause de 1 à 1.5s
            sortir_tunnel(ville_origine);
        } else {  // voyage pair = retour (Y->X ou X->Y)
            entrer_tunnel(ville_opposee);
            printf("Bus %d de %c va de %c à %c (Retour, voyage %d)\n", numero, ville_origine, ville_opposee, ville_origine, voyage);
            usleep(1000000 + rand() % 500000);  // pause de 1 à 1.5s
            sortir_tunnel(ville_opposee);
        }
    }
    return NULL;
}

int main() {
    srand(time(NULL));  // pour les pauses aléatoires

    // initialisation des sémaphores
    sem_init(&acces_tunnel, 0, 1);  // tunnel libre au début
    sem_init(&file_x, 0, 1);        // pour les bus X
    sem_init(&file_y, 0, 1);        // pour les bus Y

    pthread_t bus[NB_BUS_X + NB_BUS_Y];
    int numeros[NB_BUS_X + NB_BUS_Y];

    // créer les bus de X (numéros 1 à 5)
    for (int i = 0; i < NB_BUS_X; i++) {
        numeros[i] = i + 1;  // numéros 1, 2, 3, 4, 5
        pthread_create(&bus[i], NULL, trajet_bus, &numeros[i]);
    }

    // créer les bus de Y (numéros 6 à 9)
    for (int i = 0; i < NB_BUS_Y; i++) {
        numeros[NB_BUS_X + i] = NB_BUS_X + i + 1;  // numéros 6, 7, 8, 9
        pthread_create(&bus[NB_BUS_X + i], NULL, trajet_bus, &numeros[NB_BUS_X + i]);
    }

    // attendre que tous les bus finissent
    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(bus[i], NULL);
    }

    // nettoyer
    sem_destroy(&acces_tunnel);
    sem_destroy(&file_x);
    sem_destroy(&file_y);

    return 0;
}