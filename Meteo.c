#include <stdio.h>
#include <string.h>
#include <clib.h>
#include <rtos.h>
#include "decla.h"

Data g_meteo = {0};     // structure globale stocke données météo
int g_mutex;        // sémaphore

// Stacks  
#define STACK_SIZE 1024
unsigned int stack_decode[STACK_SIZE];
// unsigned int stack_tcp[STACK_SIZE];
unsigned int stack_affiche[STACK_SIZE];

// Définition des tâches 
TaskDefBlock def_decode = { 
    task_decode, 
    {'T','D','E','C'}, 
    stack_decode,        // début de la pile
    STACK_SIZE, 
    0,                   // options
    1,                   // priorité
    0,0,0,0,0 
};

// TaskDefBlock def_tcp = { 
//     task_tcp, 
//     {'T','T','C','P'}, 
//     stack_decode,        // début de la pile
//     STACK_SIZE, 
//     0,                   // options
//     5,                   // priorité
//     0,0,0,0,0 
// };

TaskDefBlock def_affiche = { 
    task_affiche, 
    {'T','A','F','F'}, 
    stack_affiche,       // début de la pile
    STACK_SIZE, 
    0,                   // options
    20,                  // priorité 
    0,0,0,0,0 
};

// Main 
int main(void) {
    int id_decode, id_aff, id_tcp;
    int result;

    // Configue
    fossil_init(FOSSIL_EXT);
    fossil_setbaud(FOSSIL_EXT, 9600, FOSSIL_PARITY_NO, 8, 1);
    fossil_set_flowcontrol(FOSSIL_EXT, FOSSIL_FLOWCTRL_RTSCTS);

    printf("\n--- Demarrage Meteo RTOS ---\n");

    // Création de la sémaphore
    result = RTX_Create_Sem(&g_mutex, "SEM_MET", 1);
    if(result != 0) {
        printf("Erreur creation semaphore %d\n", result);
        return 0;
    }

    //initiation
    RTX_Wait_Sem(g_mutex, 0);
    memset(&g_meteo, 0, sizeof(Data));
    g_meteo.flag_date = 0;  // pas de date reçue
    RTX_Release_Sem(g_mutex);

    // Création de la tâche decode
    result = RTX_Create_Task(&id_decode, &def_decode);
    if(result != 0) {
        printf("Erreur creation task_decode %d\n", result);
        RTX_Delete_Sem(g_mutex);
        return 0;
    }
    // Création de la tâche SERVER TCP
    // result = RTX_Create_Task(&id_tcp, &def_tcp);
    // if(result != 0) {
    //     printf("Erreur creation task_tcp %d\n", result);
    //     RTX_Delete_Sem(g_mutex);
    //     return 0;
    // }

    // Création de la tâche affiche
    result = RTX_Create_Task(&id_aff, &def_affiche);
    if(result != 0) {
        printf("Erreur creation task_affiche %d\n", result);
        RTX_Delete_Task(id_decode);
        RTX_Delete_Sem(g_mutex);
        return 0;
    }

    // Boucle principale pour ne pas quitter main
    while(1) {
        RTX_Sleep_Time(1000);  // main dort, les tâches tournent
    }
}

