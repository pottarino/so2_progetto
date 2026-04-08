#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utility.h>
#include <errors.h>

int main(int numeroArgomenti, char** argomenti) {


    // DA FARE: Finire rilevamento e cominciare gestione errori

    // Vengono settate le flags dei parametri in input
    // e alocato lo spazio iniziale per lo store delle variabili
    StatusCode statusCode = NO_ERROR; // Viene inizializzato il codice di stato
    bool flagParametroInput = false;
    char* parametroInput = malloc(32);
    bool flagParametroOutput = false;
    char* parametroOutput = malloc(32);
    bool flagVerbose = false;



    // Vengono valutati gli argomenti dati in input per impostare correttamente le flags e gli argomenti

    for (int counter = 1; counter < numeroArgomenti; counter++) {

        // Converto un argomento nella stringa corrispondente
        char* argomento = argomenti[counter];


        // Valuto la stringa:

        // Se trovo il parametro input
        if (strcmp(argomento, "--i") == 0 || strcmp(argomento, "-i") == 0) {

            // devo valutare il prossimo input ( se presente) e in caso aggiungerlo come fileInput
            if (counter + 1 < numeroArgomenti) {

                // Controllo dimensioni per possibile riallocazione
                // con un backup realloc
                size_t numeroCaratteriNext = strlen(argomenti[counter+1])+1;
                if (numeroCaratteriNext > 32){
                    char* nuovoAlloc = realloc(parametroInput, numeroCaratteriNext);
                    if (nuovoAlloc != NULL) {
                        parametroInput = nuovoAlloc;
                    }
                }
                strcpy(parametroInput, argomenti[counter+1]);
                // imposto la flag una volta controllato che vi è un input valido
                flagParametroInput = true;
                // Salto la prossima iterazione
                counter++;
            }
        }

        // Se trovo il parametro output
        else if (strcmp(argomento, "--o") == 0 || strcmp(argomento, "-o") == 0) {

            // devo valutare il prossimo input( se presente) e in caso aggingerlo come fileOutput;
            if (counter + 1< numeroArgomenti) {
                // Controllo dimensioni stringa output per possibile riallocazione
                // con un backup realloc
                size_t numeroCaratteriNext = strlen(argomenti[counter+1])+1;
                if (numeroCaratteriNext > 32){
                    char* nuovoAlloc = realloc(parametroOutput, numeroCaratteriNext);
                    if (nuovoAlloc != NULL) {
                        parametroOutput = nuovoAlloc;
                    }
                }
                strcpy(parametroOutput, argomenti[counter+1]);
                // imposto la flag una volta controllato che vi è un input valido
                flagParametroOutput = true;
                // salto la prossima iterazione
                counter++;
            }

        }

        //Se trovo il parametro verboso
        else if (strcmp(argomento, "--v") == 0 || strcmp(argomento, "-v") == 0) {
            //imposto la flag
            flagVerbose = true;
        }


    }

    // Qui vengono tradotti i parametri di input e output e presi provvedimenti in base alle flags
    // Riporto errore all'handler per input non validi
    if (flagParametroInput == false) {
        statusCode = INPUT_ERROR;
        goto exitHandler;
    }
    // Provo a leggere con filereader il file dato in input
    // GESTIONE ECCEZIONI INTERNO A FILEREADER
    char* fileStringato = filereader(parametroInput);
    // Riporto errore all'handler se leggo file vuoto
    if (fileStringato == NULL) {
        statusCode = FILE_READ_ERROR;
        goto exitHandler;
    }


    // CODICE INTERMEDIO VA INSERITO QUI ( COSì SE NON C'è INPUT VALIDO FALLISCE IMMEDIATAMENTE)

    // placeholder di quelle che saranno le statistiche da scrivere in output
    char* statistiche;

    if (flagParametroOutput == true) {
        // si scrivono su file le statistiche
        FILE *fileDaScrivere = fopen(parametroOutput, "w");
        // Se il file letto è vuoto
        if (fileDaScrivere == NULL) {
            statusCode = FILE_OPEN_ERROR;
            goto exitHandler;
        }
        // Se non riesce a scrivere sul file ( e quindi fprintf non ha scritto nulla)
        if (fprintf(fileDaScrivere, statistiche) < 0) {
            statusCode = FILE_WRITE_ERROR;
            goto exitHandler;
        };
        if (fclose(fileDaScrivere) != 0) {
            statusCode = FILE_CLOSE_ERROR;
            goto exitHandler;
        };
    }

    if (flagVerbose == true) {
        // si visualizzano le statistiche anche su stdout
        printf(statistiche);
    }



    // Libera la memoria e gestisce gli errori
    exitHandler:
    free(parametroInput);
    free(parametroOutput);
    free(fileStringato);
    switch (statusCode) {
        case NO_ERROR: return 0;
        case INPUT_ERROR: printf("errore nell'inserimento parametri di input"); return 1;    // POTREI DOVER CHIUDERE QUI I FILES
        case FILE_OPEN_ERROR: printf( "errore nell'apertura file"); return 2;
        case FILE_CLOSE_ERROR: printf( " errore nella chiusura del file"); return 3;
        case FILE_READ_ERROR: printf( "errore nella lettura del file"); return 4;
        case FILE_WRITE_ERROR: printf( "errore nella scrittura del file"); return 5;
    }
}
