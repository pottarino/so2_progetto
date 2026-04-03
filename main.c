#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int numeroArgomenti, char** argomenti) {



    // DA FARE: INSERIRE CORPO ISTRUZIONI, GESTIONE ECCEZIONI

    // Vengono settate le flags dei parametri in input
    // e alocato lo spazio iniziale per lo store delle variabili
    bool flagParametroInput = false;
    char* parametroInput = malloc(32);
    bool flagParametroOutput = false;
    char* parametroOutput = malloc(32);
    char parametroOutput2[] = {"s", "t", "d", "o", "u", "t", "\0"};
    bool flagVerbose = false;




    // Vengono valutati gli argomenti dati in input per impostare correttamente le flags e gli argomenti

    for (int counter = 1; counter < numeroArgomenti; counter++) {

        // Converto un argomento nella stringa corrispondente
        char* argomento = argomenti[counter];
        strcpy(argomento, argomenti[counter]);

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
        // Per liberare memoria a fine main
        free(parametroInput);
        free(parametroOutput);
        return 1;
    }

    if (flagParametroOutput == true) {
        // si scrivono su file le statistiche
    }
    //In ogni caso le statistiche vanno visualizzate su stdout.




    // Per liberare memoria a fine main
    free(parametroInput);
    free(parametroOutput);

}