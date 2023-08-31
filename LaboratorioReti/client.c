#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>


const int MAX_LENGTH = 512;


bool is_numeric(const char *str)
{
  size_t len = strlen(str);


  for (size_t i = 0; i < len; i++)
  {
    if (str[i] == '\n')
    continue;
    else if (str[i] < '0' || str[i] > '9')
    {
      return false;
    }
  }


  // Prova a convertire la stringa in intero usando sscanf
  int num;
  if (sscanf(str, "%d", &num) != 1)
  {
    return false;
  }


  return true;
}


int main(int argc, char *argv[])
{


  int clientSocket = 0;
  int serverPort = 0;
  int returnStatus = 0;
  struct sockaddr_in serverAddress;


  // Verifica che sia stata passata la porta e l'indirizzo come argomento sulla riga di comando
  if (3 != argc)
  {
    fprintf(stderr, "Usage: %s <server> <port>\n", argv[0]);
    exit(1);
  }


  // Creazione del socket
  clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (clientSocket == -1)
  {
    fprintf(stderr, "Impossibile creare la Socket!\n");
    exit(1);
  }
  else
  {
    fprintf(stderr, "Socket Creata!\n");
  }


  // Ottenimento del numero di porta dalla riga di comando
  serverPort = atoi(argv[2]);


  // Configura la struttura dell'indirizzo
  memset(&serverAddress, '\0', sizeof(serverAddress));
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr(argv[1]);
  serverAddress.sin_port = htons((uint16_t)serverPort);


  // Connessione all'indirizzo e alla porta con il socket
  returnStatus = connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  if (returnStatus == 0)
  {
    fprintf(stderr, "Connessione Accettata!\n");
  }
  else
  {
    fprintf(stderr, "Impossibile accettare la connessione!\n");
    close(clientSocket);
    exit(1);
  }


  char buffer[MAX_LENGTH];
  ssize_t status;


  // Riceve il messaggio di benvenuto dal server
  status = read(clientSocket, buffer, sizeof(buffer));
  buffer[status] = '\0';

  if (status > 0 && status < MAX_LENGTH)
  {
    // Rimuovi il delimitatore "OK START" dal messaggio
    if (strstr(buffer, "OK START"))
    {
      fprintf(stderr, "%s\n", buffer + strlen("OK START "));
    }
    else
    {
      fprintf(stderr,"%s\n", buffer);
    }


    printf("-----------------------------\nIstruzioni d'uso:\n");
    printf("Inserisci i dati di cui desideri calcolare la media e la varianza.\n");
    printf("I dati inseriti devono seguire il seguente formato:\n");
    printf("\t<Numero dati> <dato1> <dato2> <daton>\n");
    printf("Separare ciascun dato con uno spazio. Una volta inseriti tutti i valori desiderati,\n");
    printf("inserisci \"0\" e il programma calcolerà e restituirà i risultati.\n\n");


    int numDataTot = 0;


    while (1)
    {
      // Chiedi all'utente di inserire i dati
      printf("Inserisci i numeri separati da spazio (0 per terminare): ");
      fgets(buffer, sizeof(buffer), stdin);

      char copyBuffer[MAX_LENGTH];
      strcpy(copyBuffer,buffer);

      char *token = strtok(copyBuffer, " ");
      int index = 0;
      bool isNum;
      double data[MAX_LENGTH];


      while (token != NULL)
      {
        char temp[MAX_LENGTH];
        strcpy(temp, token);


        isNum = is_numeric(temp);
        if (!isNum)
        {
          // Invia un messaggio di errore se i dati inviati non siano validi
          fprintf(stderr,"\nERRORE I mesaggi trasmetti devono essere sintatticamente e semanticamente corretti! (Caratteri non validi)\n\n");
          break;
        }


        double num = atof(token);
        data[index] = num;
        index++;
        token = strtok(NULL, " ");
      }


      if (!isNum)
      continue;


      if (data[0] > index - 1)
      {
        // Invia un messaggio di errore se i dati inviati non siano validi
        fprintf(stderr,"\nERRORE I mesaggi trasmetti devono essere sintatticamente e semanticamente corretti! (Dati in eccesso)\n\n");
      }
      else if (data[0] < index - 1)
      {
        // Invia un messaggio di errore se i dati inviati non siano validi
        fprintf(stderr,"\nERRORE I mesaggi trasmetti devono essere sintatticamente e semanticamente corretti! (Dati mancanti)\n\n");
      }
      else
      {
        numDataTot += (int)data[0];

        if((int)data[0] == 0 && numDataTot == 1) fprintf(stderr,"\nERRORE Non si può calcolare la varianza con un solo campione! \n\n");
        else if((int)data[0] == 0 && numDataTot < 1) fprintf(stderr,"\nERRORE Non si può calcolare la varianza con nessun campione! \n\n");
        else
        {
          // Invia i dati al server
          write(clientSocket, buffer, strlen(buffer));


          // Ricevi la risposta dal server
          memset(buffer, 0, sizeof(buffer));
          status = read(clientSocket, buffer, sizeof(buffer));


          if (status > 0)
          {
            // Rimuovi i delimitatori dal messaggio e stampa la risposta
            if (strstr(buffer, "OK DATA") == buffer)
            {
              int numDataServer;
              sscanf(buffer, "OK DATA %d", &numDataServer);


              if (data[0] == numDataServer)
              printf("Dati correttamente ricevuti dal server.\n");
              else
              fprintf(stderr,"Dati ricevuti NON correttamente dal server.\n");
            }
            else if (strstr(buffer, "OK STATS") == buffer)
            {
              double media, varianza;
              int count;
              sscanf(buffer, "OK STATS %d %lf %lf", &count, &media, &varianza);
              if (numDataTot == count)
              {
                printf("\nMedia: %.1lf\nVarianza: %.1lf\n\n", media, varianza);
                break;
              }
              else
              {
                fprintf(stderr,"\nDati ricevuti NON correttamente dal server.\n\n");
                break;
              }
            }
            else if (strstr(buffer, "ERR") == buffer)
            {


              if (strstr(buffer, "DATA"))
              {
                fprintf(stderr,"Errore dal server: %s\n", buffer + strlen("ERR DATA "));
                break;
              }
              else if (strstr(buffer, "SYNTAX"))
              {
                fprintf(stderr,"Errore dal server: %s\n", buffer + strlen("ERR SYNTAX "));
                break;
              }
              else if (strstr(buffer, "STATS"))
              {
                fprintf(stderr,"Errore dal server: %s\n", buffer + strlen("ERR STATS "));
                break;
              }
            }
          }
          else
          {
            fprintf(stderr, "Errore nella ricezione dei dati dal server.\n");
            break;
          }
        }
      }
    }
  }
  else
  exit(1);


  close(clientSocket);
  fprintf(stderr, "Conessione chiusa!\n\n-----------------------------\n");
  return 0;
}
