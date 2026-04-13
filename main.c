#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utility.h>
#include <errors.h>

// int runner(int numeroArgomenti, char** argomenti) {
//
//
//     // DA FARE: Finire rilevamento e cominciare gestione errori
//
//     // Vengono settate le flags dei parametri in input
//     // e alocato lo spazio iniziale per lo store delle variabili
//     StatusCode statusCode = NO_ERROR; // Viene inizializzato il codice di stato
//     bool flagParametroInput = false;
//     char* parametroInput = malloc(32);
//     bool flagParametroOutput = false;
//     char* parametroOutput = malloc(32);
//     bool flagVerbose = false;
//
//
//
//     // Vengono valutati gli argomenti dati in input per impostare correttamente le flags e gli argomenti
//
//     for (int counter = 1; counter < numeroArgomenti; counter++) {
//
//         // Converto un argomento nella stringa corrispondente
//         char* argomento = argomenti[counter];
//
//
//         // Valuto la stringa:
//
//         // Se trovo il parametro input
//         if (strcmp(argomento, "--in") == 0 || strcmp(argomento, "-i") == 0) {
//
//             // devo valutare il prossimo input ( se presente) e in caso aggiungerlo come fileInput
//             if (counter + 1 < numeroArgomenti) {
//
//                 // Controllo dimensioni per possibile riallocazione
//                 // con un backup realloc
//                 size_t numeroCaratteriNext = strlen(argomenti[counter+1])+1;
//                 if (numeroCaratteriNext > 32){
//                     char* nuovoAlloc = realloc(parametroInput, numeroCaratteriNext);
//                     if (nuovoAlloc != NULL) {
//                         parametroInput = nuovoAlloc;
//                     }
//                 }
//                 strcpy(parametroInput, argomenti[counter+1]);
//                 // imposto la flag una volta controllato che vi è un input valido
//                 flagParametroInput = true;
//                 // Salto la prossima iterazione
//                 counter++;
//             }
//         }
//
//         // Se trovo il parametro output
//         else if (strcmp(argomento, "--out") == 0 || strcmp(argomento, "-o") == 0) {
//
//             // devo valutare il prossimo input( se presente) e in caso aggingerlo come fileOutput;
//             if (counter + 1< numeroArgomenti) {
//                 // Controllo dimensioni stringa output per possibile riallocazione
//                 // con un backup realloc
//                 size_t numeroCaratteriNext = strlen(argomenti[counter+1])+1;
//                 if (numeroCaratteriNext > 32){
//                     char* nuovoAlloc = realloc(parametroOutput, numeroCaratteriNext);
//                     if (nuovoAlloc != NULL) {
//                         parametroOutput = nuovoAlloc;
//                     }
//                 }
//                 strcpy(parametroOutput, argomenti[counter+1]);
//                 // imposto la flag una volta controllato che vi è un input valido
//                 flagParametroOutput = true;
//                 // salto la prossima iterazione
//                 counter++;
//             }
//
//         }
//
//         //Se trovo il parametro verboso
//         else if (strcmp(argomento, "--verbose") == 0 || strcmp(argomento, "-v") == 0) {
//             //imposto la flag
//             flagVerbose = true;
//         }
//
//
//     }
//
//     // Qui vengono tradotti i parametri di input e output e presi provvedimenti in base alle flags
//     // Riporto errore all'handler per input non validi
//     if (flagParametroInput == false) {
//         statusCode = INPUT_ERROR;
//         goto exitHandler;
//     }
//     // Provo a leggere con filereader il file dato in input
//     // GESTIONE ECCEZIONI INTERNO A FILEREADER
//     FileRead file_read = file_reader(parametroInput);
//     // Riporto errore all'handler se leggo file vuoto
//     if (file_read.statusCode != 0) {
//         statusCode = FILE_READ_ERROR;
//         goto exitHandler;
//     }
//
//
//     // CODICE INTERMEDIO VA INSERITO QUI ( COSì SE NON C'è INPUT VALIDO FALLISCE IMMEDIATAMENTE)
//
//     // placeholder di quelle che saranno le statistiche da scrivere in output
//     char* statistiche;
//
//     if (flagParametroOutput == true) {
//         // si scrivono su file le statistiche
//         FILE *fileDaScrivere = fopen(parametroOutput, "w");
//         // Se il file letto è vuoto
//         if (fileDaScrivere == NULL) {
//             statusCode = FILE_OPEN_ERROR;
//             goto exitHandler;
//         }
//         // Se non riesce a scrivere sul file ( e quindi fprintf non ha scritto nulla)
//         if (fprintf(fileDaScrivere, statistiche) < 0) {
//             statusCode = FILE_WRITE_ERROR;
//             goto exitHandler;
//         };
//         if (fclose(fileDaScrivere) != 0) {
//             statusCode = FILE_CLOSE_ERROR;
//             goto exitHandler;
//         };
//     }
//
//     if (flagVerbose == true) {
//         // si visualizzano le statistiche anche su stdout
//         printf(statistiche);
//     }
//
//
//
//     // Libera la memoria e gestisce gli errori
//     exitHandler:
//     free(parametroInput);
//     free(parametroOutput);
//     switch (statusCode) {
//         case NO_ERROR: return 0;
//         case INPUT_ERROR: printf("errore nell'inserimento parametri di input"); return 1;    // POTREI DOVER CHIUDERE QUI I FILES
//         case FILE_OPEN_ERROR: printf( "errore nell'apertura file"); return 2;
//         case FILE_CLOSE_ERROR: printf( " errore nella chiusura del file"); return 3;
//         case FILE_READ_ERROR: printf( "errore nella lettura del file"); return 4;
//         case FILE_WRITE_ERROR: printf( "errore nella scrittura del file"); return 5;
//     }
// }

#include <stdio.h>
#include <stdlib.h>
#include "stats.h"
#include "parsers.h"

void test_stats_calculator(char* filename) {
    printf("--- AVVIO TEST PRECOMPILATORE: %s ---\n", filename);

    // 1. carichiamo il file (usando la tua utility file_reader)
    FileRead fr = file_reader(filename);
    if (fr.text == NULL) {
        printf("errore: non riesco a leggere %s\n", filename);
        return;
    }

    // 2. primo parsing (pulizia commenti e split headers/global/main)
    ParsedProgram pp = first_parsing(fr.text, filename);

    // 3. inizializzazione dizionari (fondamentale per is_keyword)
    init_syntax();

    // 4. calcolo delle stats
    Stats stats = stats_calculator(pp);

    // 5. visualizzazione risultati
    printf("\n--- RISULTATI STATISTICHE ---\n");
    printf("variabili totali trovate: %d\n", stats.variable_counter);
    printf("nomi illegali:            %d\n", stats.illegal_names_counter);
    printf("tipi errati:              %d\n", stats.wrong_type_counter);
    printf("variabili inutilizzate:   %d\n", stats.unused_variable_counter);
    printf("totale errori loggati:    %d\n", stats.error_counter);

    printf("\n--- DETTAGLIO ERRORI ---\n");
    for (int i = 0; i < stats.error_counter; i++) {
        char* err_name = "";
        if (stats.errors[i].type == VARIABLE_NAME_ERROR) err_name = "NOME_ILLEGALE";
        else if (stats.errors[i].type == VARIABLE_TYPE_ERROR) err_name = "TIPO_ERRATO";
        else if (stats.errors[i].type == VARIABLE_UNUSED_ERROR) err_name = "NON_USATA";

        printf("[%s] rigo %d nel file %s\n",
                err_name,
                stats.errors[i].line,
                stats.errors[i].filename);
    }

    // 6. cleanup finale (usa le funzioni che abbiamo scritto)
    // free_stats(&stats);
    // free_parsed_program(&pp);
    // free(fr.text);

    printf("\n--- TEST COMPLETATO ---\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("passami il file c come argomento, pirla!\n");
        return 1;
    }

    test_stats_calculator(argv[1]);

    return 0;
}
//int main(int numeroArgomenti, char** argomenti) {
  //  return runner(numeroArgomenti, argomenti);
//}
