#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <signal.h>



int g = 0; 


int randInt(int min, int max);


int main (void) {

    
    srand(time(NULL));

    int procServProduction;
    int procServBackup;


    procServProduction = fork();



    if (procServProduction == 0) {

        // [#] Production Server [#]

        printf("[Prod.] I'm the Production Server ! || (PID : \"%d\" ; PPID : \"%d\")\n", getpid(), getppid());

        // Production Server Code


    } else {

    
        procServBackup = fork();
    
        if (procServBackup == 0) {

            // [#] Backup Server [#]

            printf("[Backup] I'm the Backup Server ! || (PID : \"%d\" ; PPID : \"%d\")\n", getpid(), getppid());

            // Backup Server Code
            
        } else if (procServProduction != 0) {

            // [#] Integration Server [#]

            sleep(1);  // Le "sleep()" ici n'est pas nécessaire

            printf("{TEST} I'm the Integration Server ! || (PID : \"%d\" ; PPID : \"%d\")\n", getpid(), getppid());
            printf("{TEST} Production server (PID) @ \"%d\" || Backup server (PID) @ \"%d\"\n", procServProduction, procServBackup);
        


            /* 
                Le code ci-dessous devait permettre de vérifier si un processus est vivant ou mort.
                Le problème est que je n'arrive pas à faire en sorte de pouvoir tuer le processus fils
                pour de bon avant le père à cause de l'état zombie dans lequel il rentre. Au final,
                même si on fait un "exit()" dans le fils (Serveur Production OU Serveur Backup) et qu'on
                met un "sleep()" dans le père (Serveur Intégration); "kill(PID, 0)", permettant de vérifier
                si un processus est vivant ou non, va supposer le fils (Serveur Production OU Serveur Backup)
                est toujours vivant, à cause du fait qu'il en en mode zombie. J'ai même fait en sorte de faire
                un envoi de signal pour kill le fils (Serveur Production OU Serveur Backup), mais ça tue par
                le même occasion le père (Serveur Intégration)...

                La seule solution simple est efficace que j'ai trouvée serait de créer une variable dans les
                Serveur Production et Serveur Backup qui changerait aléatoirement d'état dans une boucle infini
                afin de définir si ces dernier sont En ligne ou Hors ligne. On peut par exemple utiliser des "pipe()"
                pour obtenir cette variable des Serveur Production et Serveur Backup. Le Serveur d'Intégration pourra
                vérifier ces valeurs grâce aux "pipe()" et effectuer les tâche qu'il faut.
            */

            // if(kill(procServProduction, 0) == 0) {
            //     printf("Production server exists !!!, PID SERV : %d\n", procServProduction);
            // } else {
            //     printf("Damn... Production server doesn't exist...\n");
            // }

            // Test Server Code
        }
    }

    return 0;
}



int randInt(int min, int max) {
  int number = (rand() % (max - min + 1)) + min;
  return number;
}
