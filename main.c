#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utility.h>

int main(int numeroArgomenti, char** argomenti) {


    // PRO-MEM: riprendere da inserimento input e tokenizzazione (riga 98)
    // DA FARE: INSERIRE CORPO ISTRUZIONI PRINCIPALE, GESTIONE ECCEZIONI I/O; CONTROLLO MEMLEAKS

    // Vengono settate le flags dei parametri in input
    // e alocato lo spazio iniziale per lo store delle variabili
    int codiceUscita = 0; // Viene settato ad uno per chiusura dovuta ad errori
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
    if (flagParametroInput == false) {
        // Occorre simulare una "eccezione" su parametroOutput2/stdout
        printf("Inserire parametro di input valido");
        codiceUscita = 1;
        goto liberaMemoria;
    }

    // Altrimenti leggiamo il file in input e ci "facciamo cose" usando filereader delle utilities
    char* fileStringato = filereader(parametroInput);
    if (fileStringato == NULL) {
        printf("Il file di input non può essere letto. Per favore riprovare");
        codiceUscita = 1;
        goto liberaMemoria;
    }


    // CODICE INTERMEDIO VA INSERITO QUI ( COSì SE NON C'è INPUT VALIDO FALLISCE IMMEDIATAMENTE)

    // placeholder di quelle che saranno le statistiche da scrivere in output
    char* statistiche;

    if (flagParametroOutput == true) {
        // si scrivono su file le statistiche
        FILE *fileDaScrivere = fopen(parametroOutput, "w");
        fprintf(fileDaScrivere, statistiche);
        fclose(fileDaScrivere);
    }

    if (flagVerbose == true) {
        // si visualizzano le statistiche anche su stdout
        printf(statistiche);
    }



    // Libera la memoria per evitare leaks e restituire il codice di uscita
    liberaMemoria:
    free(parametroInput);
    free(parametroOutput);
    free(fileStringato);
    return codiceUscita;

}