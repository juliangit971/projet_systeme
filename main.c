#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>



// Max size for a Log Message
#define LOG_MSG_LENGTH 128

// Max size of a File Name
#define MAX_FILENAME_LENGTH 256


// [#] Directories representing the servers and their files
// /!\ CAREFUL /!\ Create those 2 directories before starting the code
    // Production Server Directory
#define PROD_DIR "./prod"
    // Backup Server Directory
#define BACKUP_DIR "./backup"


// Structure représentant un Serveur (Production ou Backup)
typedef struct {
    char ** fileList;
    time_t * modTimes;
    int fileCount;
} Server;


// Structure permettant de "simuler" les serveurs de Production et Backup
Server prodServer;
Server backupServer;


// Fichier faisant référence à "server_stats.log", fichier où est stocké les informations envoyé à la fonction "statUpdate"
FILE * statFile;


pthread_mutex_t prodMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t backupMutex = PTHREAD_MUTEX_INITIALIZER;



// Thread simulant le serveur d'Intégration
void * servIntegr(void * arg);

// "test server" Module
void testServer(const char * prodDir, const char * backupDir);

// "synchro list" module, renommé en "synchroServers" plus rendre la fonction plus parlante
void * syncServers(void * arg);

// "copy list" Module
int copyList(const char * srcDir, const char * destDir, char ** fileList, int fileCount);


// Permet d'obtenir la liste des fichiers d'un répertoire avec leur nom, leur date de modification, et le nombre de fichiers dans ce répertoire
char ** getFileList(const char * dir, time_t ** modTimes, int * fileCount);

// Permet de comparer 2 listes de fichiers de 2 dossiers et renvoie la liste des fichiers qui diffère dans les 2 listes
char ** syncList(Server * server1, Server * server2, int * diffCount);


// "stat" Module
// Permet de stocker les messages envoyés par chaque fonction dans le fichier "server_stats.log"
void statUpdate(const char * module, const char * stat);

// "log" Module
// Permet d'écrire un message dans la console
void logMessage(const char * msg);


// Utils
int randInt(int min, int max);




int main(void) {

	srand(time(NULL));

    pthread_t integrThread;

	// Ouverture du fichier "server_stats.log"
    statFile = fopen("server_stats.log", "a");
    if (statFile == NULL) {
        perror("fopen");
        return 1;
    }

	// Ouverture du Serveur d'Intégration
    pthread_create(&integrThread, NULL, servIntegr, NULL);
    pthread_join(integrThread, NULL);

    fclose(statFile);
    return 0;
}





// Thread simulant le serveur d'Intégration
void* servIntegr(void *arg) {

    char prodDir[] = PROD_DIR;
    char backupDir[] = BACKUP_DIR;
    pthread_t syncThread;
    int choice;

    printf("Integration Server, A.K.A \"IntServ\", Frirmware v0.0.1\n");
    printf("Welcome to the Integration Server !!!\n");
    

    while (1) {
        printf("\n");
        printf("What do you want to do?\n");
        printf("1. Check server availability\n");
        printf("2. Synchronize servers\n");
        printf("3. Quit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        printf("\n");

        switch (choice) {
            case 1:
				testServer(prodDir, backupDir);
				break;
            case 2:
                pthread_create(&syncThread, NULL, syncServers, (void*)prodDir);
                pthread_join(syncThread, NULL);
                break;
            case 3:
                fclose(statFile);
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return NULL;
}






// "test server" Module
void testServer(const char * prodDir, const char * backupDir) {

    if (access(prodDir, F_OK) == 0 && randInt(1, 3) == 1) {
		logMessage("Production server is available.");
    } else if (access(backupDir, F_OK) == 0 && randInt(1, 3) == 2) {
		logMessage("Production server is not available.");
		logMessage("Backup server is available.");
    } else {
		logMessage("Production server is not available.");
		logMessage("Backup server is not available.");
    }
}




// "synchro list" module, renommé en "synchroServers" plus rendre la fonction plus parlante
void * syncServers(void * arg) {

    char * prodDir = (char *) arg;
    char backupDir[] = BACKUP_DIR;
    int diffCount;
    char ** diffList;
    int totalCopiedFiles = 0;

	// Obtention de la liste des fichiers du Serveur de Production
    pthread_mutex_lock(&prodMutex);
    prodServer.fileList = getFileList(prodDir, &prodServer.modTimes, &prodServer.fileCount);
    pthread_mutex_unlock(&prodMutex);

	// Obtention de la liste des fichiers du Serveur Backup
    pthread_mutex_lock(&backupMutex);
    backupServer.fileList = getFileList(backupDir, &backupServer.modTimes, &backupServer.fileCount);
    pthread_mutex_unlock(&backupMutex);


	// [#] Copie de "Production Server" vers "Backup Server"
	
	logMessage("Copy from \"Prod\" to \"Backup\"...");

	// Obtention des fichiers qui diffèrent entre les 2 serveurs
    diffList = syncList(&prodServer, &backupServer, &diffCount);
	
	// Copie des fichier
    int copiedFiles;
    copiedFiles = copyList(prodDir, backupDir, diffList, diffCount);

    char statMsg[LOG_MSG_LENGTH];
    snprintf(statMsg, LOG_MSG_LENGTH, "Files copied from \"Prod\" to \"Backup\": %d", copiedFiles);
    statUpdate("syncServers", statMsg);

    totalCopiedFiles += copiedFiles;

    // [#] Copie de "Backup Server" vers "Production Server"
	logMessage("Copy from \"Backup\" to \"Prod\"...");
    diffList = syncList(&backupServer, &prodServer, &diffCount);
    copiedFiles = copyList(backupDir, prodDir, diffList, diffCount);

    snprintf(statMsg, LOG_MSG_LENGTH, "Files copied from \"Backup\" to \"Prod\": %d", copiedFiles);
    statUpdate("syncServers", statMsg);

    totalCopiedFiles += copiedFiles;

	// Vidange mémoire
    for (int i = 0; i < prodServer.fileCount; i++) {
        free(prodServer.fileList[i]);
    }
    free(prodServer.fileList);
    free(prodServer.modTimes);

    for (int i = 0; i < backupServer.fileCount; i++) {
        free(backupServer.fileList[i]);
    }
    free(backupServer.fileList);
    free(backupServer.modTimes);

    return NULL;
}



// "copy list" Module
int copyList(const char * srcDir, const char * destDir, char ** fileList, int fileCount) {

    char srcPath[MAX_FILENAME_LENGTH], destPath[MAX_FILENAME_LENGTH];
    FILE * srcFile, * destFile;
    char buffer[BUFSIZ];
    size_t bytesRead;
    int copiedFiles = 0;

    for (int i = 0; i < fileCount; i++) {
        snprintf(srcPath, MAX_FILENAME_LENGTH, "%s/%s", srcDir, fileList[i]);
        snprintf(destPath, MAX_FILENAME_LENGTH, "%s/%s", destDir, fileList[i]);

        srcFile = fopen(srcPath, "rb");
        if (srcFile == NULL) {
            perror("fopen src");
            continue;
        }

        destFile = fopen(destPath, "wb");
        if (destFile == NULL) {
            perror("fopen dest");
            fclose(srcFile);
            continue;
        }

        while ((bytesRead = fread(buffer, 1, BUFSIZ, srcFile)) > 0) {
            fwrite(buffer, 1, bytesRead, destFile);
        }

        fclose(srcFile);
        fclose(destFile);

        copiedFiles++;
    }

    char statMsg[LOG_MSG_LENGTH];
    snprintf(statMsg, LOG_MSG_LENGTH, "Files copied: %d", copiedFiles);
    statUpdate("copyList", statMsg);

	logMessage("File copied successfully.");

    return copiedFiles;
}




// Permet d'obtenir la liste des fichiers d'un répertoire avec leur nom et leur date de modification
char ** getFileList(const char * dir, time_t ** modTimes, int * fileCount) {

    DIR * dp;
    struct dirent * entry;
    struct stat statbuf;
    int count = 0;
    char ** fileList = NULL;
    time_t * times = NULL;
    char filePath[MAX_FILENAME_LENGTH];

    if ((dp = opendir(dir)) == NULL) {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(dp)) != NULL) {

        snprintf(filePath, sizeof(filePath) + 1, "%s/%s", dir, entry->d_name);

        if (stat(filePath, &statbuf) == 0 && S_ISREG(statbuf.st_mode)) {
            fileList = realloc(fileList, sizeof(char*) * (count + 1));
            times = realloc(times, sizeof(time_t) * (count + 1));
            fileList[count] = strdup(entry->d_name);
			// Stocker la date de modification
            times[count] = statbuf.st_mtime; 
            count++;
        }
    }
    closedir(dp);

    *fileCount = count;
    *modTimes = times;
    return fileList;
}



// Permet de comparer 2 listes de fichiers entre 2 dossiers
char ** syncList(Server * server1, Server * server2, int * diffCount) {

    int i, j, found;
    char ** diffList = NULL;
    * diffCount = 0;

    for (i = 0; i < server1->fileCount; i++) {
        found = 0;
        for (j = 0; j < server2->fileCount; j++) {
            if (strcmp(server1->fileList[i], server2->fileList[j]) == 0) {
                found = 1;
                if (server1->modTimes[i] > server2->modTimes[j]) {
                    diffList = realloc(diffList, sizeof(char*) * (*diffCount + 1));
                    diffList[*diffCount] = strdup(server1->fileList[i]);
                    (*diffCount)++;
                }
                break;
            }
        }
        if (!found) {
            diffList = realloc(diffList, sizeof(char*) * (*diffCount + 1));
            diffList[*diffCount] = strdup(server1->fileList[i]);
            (*diffCount)++;
        }
    }
    return diffList;
}




// "stat" Module
void statUpdate(const char * module, const char * stat) {

    if (statFile) {
        fprintf(statFile, "[%s]: %s\n", module, stat);
        fflush(statFile);
    } else {
        fprintf(stderr, "Failed to open statistics file.\n");
    }
}


// "log" Module
void logMessage(const char * msg) {
    printf("LOG: %s\n", msg);
}




// Donner un nombre aléatoire entre min et max
int randInt(int min, int max) {
  int number = (rand() % (max - min + 1)) + min;
  return number;
}



