#ifndef DECLA_H
#define DECLA_H

#include <rtos.h>

typedef struct {
    // CAPTEUR
    int temp;      
    int temp_ext;   
    int vent_kmh;   
    int angle;      
    int pression;   
    int humidite;  
    int humidite_ext; 
    char dir_nom[5];

    // HORLOGE 
    int heure;
    int minute;
    int jour;
    int mois;
    int annee;
    int flag_date;
} Data;

// Variables globales
extern Data g_meteo; 
extern int g_mutex;

// Prototypes fonctions
int check_somme(unsigned char *trame, int longueur);
void decode_thb(unsigned char *trame);
void decode_vent(unsigned char *trame);
void decode_mushroo(unsigned char *trame);
void decode_date(unsigned char *trame);

// Prototypes tâches
void huge task_decode(void);
// void huge task_tcp(void);
void huge task_affiche(void);

#endif
