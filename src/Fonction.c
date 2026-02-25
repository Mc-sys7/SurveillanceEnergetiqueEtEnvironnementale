#include <stdio.h>
#include <clib.h>
#include <string.h>
#include "decla.h"
#include <rtos.h>

const char* DIRECTIONS[] = {"N", "NE", "E", "SE", "S", "SO", "O", "NO"};

int check_somme(unsigned char *trame, int longueur)
{
    int i;
    unsigned int sum = 0;

    sum += 0xFF;
    sum += 0xFF;

    for(i = 0; i < longueur - 1; i++)
        sum += trame[i];

    return ((unsigned char)(sum & 0xFF) == trame[longueur - 1]);
}

// DECODAGES

void decode_vent(unsigned char *trame)
{
    int dizaines = ((trame[2] >> 4) * 10) + (trame[2] & 0x0F);
    int centaines = (trame[3] & 0x0F) * 100;
    int angle = centaines + dizaines;
    int dir_index = ((angle + 22) / 45) % 8;

    int v_dizaines = (trame[4] >> 4);
    int v_unites   = (trame[4] & 0x0F);
    int v_dixiemes = (trame[3] >> 4);

    int vitesse_ms10 = (v_dizaines * 100) + (v_unites * 10) + v_dixiemes;
    int v_kmh_10 = ((vitesse_ms10 * 36)+5) / 10;

    RTX_Wait_Sem(g_mutex, 0);
        g_meteo.vent_kmh = v_kmh_10;
        g_meteo.angle = angle;
        strcpy(g_meteo.dir_nom, DIRECTIONS[dir_index]);
    RTX_Release_Sem(g_mutex);
}

void decode_thb(unsigned char *trame)
{
    int dizaines = trame[3];
    int unites   = (trame[2] >> 4);
    int dixieme  = (trame[2] & 0x0F);

    int t = (dizaines * 100) + (unites * 10) + dixieme;

    int h = ((trame[4] >> 4) * 10) + (trame[4] & 0x0F);
    int p = trame[6] + 856;

    RTX_Wait_Sem(g_mutex, 0);
        g_meteo.temp = t;
        g_meteo.humidite = h;
        g_meteo.pression = p;
    RTX_Release_Sem(g_mutex);
}

void decode_mushroo(unsigned char *trame)
{
    int dizaines = trame[3];
    int unites   = (trame[2] >> 4);
    int dixieme  = (trame[2] & 0x0F);

    int temp = (dizaines * 100) + (unites * 10) + dixieme;
    int hum  = ((trame[4] >> 4) * 10) + (trame[4] & 0x0F);

    RTX_Wait_Sem(g_mutex, 0);
        g_meteo.temp_ext = temp;
        g_meteo.humidite_ext = hum;
    RTX_Release_Sem(g_mutex);
}

void decode_date(unsigned char *trame){

    int min   = ((trame[1] >> 4) * 10) + (trame[1] & 0x0F);
    int heure = ((trame[2] >> 4) * 10) + (trame[2] & 0x0F);
    int jour  = ((trame[3] >> 4) * 10) + (trame[3] & 0x0F);
    int mois  = ((trame[4] >> 4) * 10) + (trame[4] & 0x0F);
    int annee = 2000 + ((trame[5] >> 4) * 10) + (trame[5] & 0x0F);

    RTX_Wait_Sem(g_mutex, 0);
        g_meteo.minute = min;
        g_meteo.heure  = heure;
        g_meteo.jour   = jour;
        g_meteo.mois   = mois;
        g_meteo.annee  = annee;
        g_meteo.flag_date = 1;  // date reçue
    RTX_Release_Sem(g_mutex);
}

//TACHE DECODE 

void huge task_decode(void) {
    int a;
    unsigned char c, trame[30];
    int etat = 0, i = 0, len = 0;
    int timeout = 0;

    printf("task_decode start\n");

    while(1) {
        int status = fossil_status_request(FOSSIL_EXT);
        if(status & 0x01) {
            a = fossil_getbyte(FOSSIL_EXT);
            if(a != -1) {
                c = (unsigned char)a;

                switch(etat) {
                    case 0: if(c==0xFF) etat=1; break;
                    case 1: if(c==0xFF) etat=2; else etat=0; break;
                    case 2: trame[0]=c; i=1; len=0; etat=3; break;
                    case 3:
                        trame[i++] = c;
                        if(trame[0]==0x06) len=12;
                        else if(trame[0]==0x00) len=9;
                        else if(trame[0]==0x03) len=7;
                        else if(trame[0]==0x0F) len=7;

                        if(len>0 && i>=len) {
                            if(check_somme(trame,i)) {
                                if(trame[0]==0x06) decode_thb(trame);
                                else if(trame[0]==0x00) decode_vent(trame);
                                else if(trame[0]==0x03) decode_mushroo(trame);
                                else if(trame[0]==0x0F) decode_date(trame);
                            }
                            etat=0; i=0; timeout=0;  // reset complet
                        }
                    break;
                }
                timeout = 0; // un byte reçu  reset timeout
            }
        } else {
            timeout++;
            if(timeout > 500) {
                // Si aucune donnée n'arrive depuis trop longtemps, reset état
                etat = 0;
                i = 0;
                len = 0;
                timeout = 0;
            }
        }
        RTX_Sleep_Time(1);
    }
}

// void huge task_tcp(void) {

// }

//TACHE AFFICHE 

void huge task_affiche(void)
{
    Data local;

    while(1)
    {
        RTX_Wait_Sem(g_mutex, 0);
            local = g_meteo;
        RTX_Release_Sem(g_mutex);

        if(local.flag_date){
        printf("\n============= STATION METEO | %02d/%02d/%d %02d:%02d ============",local.jour, local.mois, local.annee,local.heure, local.minute);
        } else {
        printf("\n============= STATION METEO | ---/--/---- --:-- =============");
        }
        
        printf("\n================ CAPTEUR BARO-TERMO-HYGRO  ================");
        printf("\n   Temp : %d.%d C", local.temp / 10, local.temp % 10);
        printf("\n   Hum  : %d %%", local.humidite);
        printf("\n   Pres : %d hPa", local.pression);

        printf("\n===================== CAPTEUR MUSHROOM =====================");
        printf("\n   Temp Ext : %d.%d C", local.temp_ext / 10, local.temp_ext % 10);
        printf("\n   Hum Ext  : %d %%", local.humidite_ext);

        printf("\n======================= CAPTEUR VENT ======================= ");
        printf("\n   Vitesse : %d.%d km/h", local.vent_kmh / 10, local.vent_kmh % 10);
        printf("\n   Angle : %d", local.angle);
        printf("\n   Direction : %s", local.dir_nom);

        RTX_Sleep_Time(1000);
    }
}
