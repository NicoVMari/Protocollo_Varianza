#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>


const int MAX_LENGTH = 512;


bool is_numeric(const char *str, char *errorChar)
{
  size_t len = strlen(str);

  for (size_t i = 0; i < len; i++)
  {
    if (str[i] == '\n')
    continue;
    else if (str[i] < '0' || str[i] > '9')
    {
      *errorChar = str[i];
      return false;
    }
  }

  // Prova a convertire la stringa in intero usando sscanf
  int num;
  if (sscanf(str, "%d", &num) != 1) {
    *errorChar = '\0'; // Nessun carattere errato specifico
    return false;
  }

  return true;
}


int main(int argc, char *argv[])
{


  int varianzaSocket = 0;
  int varianzaPort = 0;
  int returnStatus = 0;
  struct sockaddr_in varianzaServer;


  // Verifica che sia stata passata la porta come argomento sulla riga di comando
  if (2 != argc)
  {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }


  // Creazione del socket
  varianzaSocket = socket(AF_INET, SOCK_STREAM, 0); // IPv4, Trasferimento affidabile bidirezionale, TCP
  if (varianzaSocket == -1)
  {
    fprintf(stderr, "Impossibile creare la Socket!\n");
    exit(1);
  }
  else
  {
    fprintf(stderr, "Socket Creata!\n");
  }


  // Ottenimento del numero di porta dalla riga di comando
  varianzaPort = atoi(argv[1]);


  // Configurazione della struttura dell'indirizzo per il bind
  memset(&varianzaServer, '\0', sizeof(varianzaServer));
  varianzaServer.sin_family = AF_INET;
  varianzaServer.sin_addr.s_addr = htonl(INADDR_ANY);      // host to network long
  varianzaServer.sin_port = htons((uint16_t)varianzaPort); // host to network short


  // Binding del socket all'indirizzo e alla porta
  returnStatus = bind(varianzaSocket, (struct sockaddr *)&varianzaServer, sizeof(varianzaServer));
  if (returnStatus != 0)
  {
    fprintf(stderr, "Impossibile fare il Bind! (Cambiare porta)\n");
    close(varianzaSocket);
    exit(1);
  }


  // Mette in ascolto il socket per connessioni in arrivo (1 viene presa in considerazione e le altre 4 attendono)
  returnStatus = listen(varianzaSocket, 1);
  if (returnStatus == -1)
  {
    fprintf(stderr, "Impossibile ascoltare sulla Socket!\n");
    close(varianzaSocket);
    exit(1);
  }
  else
  {
    fprintf(stderr, "Binding completato!\n-----------------------------\n");
  }


  while (1)
  {


    struct sockaddr_in clientName = {0};
    int childSocket = 0;
    socklen_t clientNameLength = sizeof(clientName);


    // Accetta una connessione in arrivo
    childSocket = accept(varianzaSocket, (struct sockaddr *)&clientName, &clientNameLength);
    if (childSocket == -1)
    {
      fprintf(stderr, "Impossibile accettare la connessione!\n");
      close(varianzaSocket);
      exit(1);
    }
    else
    fprintf(stderr, "Connessione Accettata!\n\n");


    // Converti l'indirizzo IP del client in una stringa
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientName.sin_addr), clientIP, INET_ADDRSTRLEN);


    // Crea un messaggio di benvenuto con l'indirizzo IP del client
    char welcomeMessage[MAX_LENGTH];
    snprintf(welcomeMessage, sizeof(welcomeMessage), "OK START Benvenuto %s. Io calcolo media e varianza!\n", clientIP);


    // Invia il messaggio di benvenuto al client
    write(childSocket, welcomeMessage, strlen(welcomeMessage));


    int countTot = 0;
    double sumTot = 0.0f;
    double sumQuadro = 0.0f;


    while (1)
    {
      char buffer[MAX_LENGTH];
      ssize_t bytesRead = read(childSocket, buffer, sizeof(buffer) - 1);


      if ((bytesRead > 0 && bytesRead < MAX_LENGTH - 1))
      {
        buffer[bytesRead] = '\0';


        fprintf(stderr,"Dati ricevuti: %s", buffer);


        char *token = strtok(buffer, " "), errorChar;
        int index = 0;
        bool isNum;
        double data[MAX_LENGTH];


        while (token != NULL)
        {
          char temp[MAX_LENGTH];
          strcpy(temp, token);



          isNum = is_numeric(temp, &errorChar);
          if (!isNum)
          {
            // Invia un messaggio di errore se i dati inviati non siano validi
            fprintf(stderr,"\nERRORE Input non valido!\n\n");
            char errCharMessage[MAX_LENGTH];
            if(errorChar == '\0') snprintf(errCharMessage, sizeof(errCharMessage), "ERR SYNTAX Troppi spazi nella riga\n");
            else snprintf(errCharMessage, sizeof(errCharMessage), "ERR SYNTAX Carattere '%c' non valido\n", errorChar);
            write(childSocket, errCharMessage, strlen(errCharMessage));
            break;
          }


          double num = atof(token);
          data[index] = num;
          index++;
          token = strtok(NULL, " ");
        }


        if (!isNum)
        break;


        if (data[0] > index - 1)
        {
          // Invia un messaggio di errore se il numero di dati letti non corrisponde a quello dichiarato
          fprintf(stderr,"\nERRORE Dati mancanti rispetto al previsto!\n\n");
          char errMancantiMessage[MAX_LENGTH];
          snprintf(errMancantiMessage, sizeof(errMancantiMessage), "ERR DATA Campioni mancanti %d<>%d\n", index - 1, (int)data[0]);
          write(childSocket, errMancantiMessage, strlen(errMancantiMessage));
          break;
        }
        else if (data[0] < index - 1)
        {
          // Invia un messaggio di errore se il numero di dati letti non corrisponde a quello dichiarato
          fprintf(stderr,"\nERRORE Dati in eccesso rispetto al previsto!\n\n");
          char errEccessoMessage[MAX_LENGTH];
          snprintf(errEccessoMessage, sizeof(errEccessoMessage), "ERR DATA Campioni in eccesso %d<>%d\n", index - 1, (int)data[0]);
          write(childSocket, errEccessoMessage, strlen(errEccessoMessage));
          break;
        }
        else
        {
          if (data[0] > 0)
          {
            countTot += index - 1;


            for (int i = 1; i <= data[0]; i++)
            {
              sumTot += data[i];
              sumQuadro += (data[i] * data[i]);
            }


            // Invia la risposta con la somma dei dati
            char response[MAX_LENGTH];
            snprintf(response, sizeof(response), "OK DATA %d\n", (int)data[0]);
            write(childSocket, response, strlen(response));
          }
          else if (data[0] == 0)
          {
            if (countTot > 1)
            {
              // Calcola la media e la varianza e invia la risposta
              double media = sumTot / (double)countTot;
              double varianza = (sumQuadro - (sumTot * sumTot) / countTot) / (countTot - 1);
              char response[MAX_LENGTH];
              snprintf(response, sizeof(response), "OK STATS %d %f %f\n", countTot, media, varianza);
              fprintf(stderr,"\nDati mandati al client: %d %f %f\n\n", countTot, media, varianza);
              write(childSocket, response, strlen(response));
              break;
            }
            else if(countTot == 1)
            {
              // Invia un messaggio di errore se non è possibile calcolare la media e la varianza
              fprintf(stderr,"\nERRORE Impossibile calcolare la varianza con un solo campione!\n\n");
              write(childSocket, "ERR STATS Solo 1 campione - La varianza non può essere calcolata\n", strlen("ERR STATS Solo 1 campione - La varianza non può essere calcolata\n"));
              break;
            }
            else
            {
              // Invia un messaggio di errore se non è possibile calcolare la media e la varianza
              fprintf(stderr,"\nERRORE Impossibile calcolare la varianza con nessun dato!\n\n");
              write(childSocket, "ERR STATS Nessun campione\n", strlen("ERR STATS Nessun campione\n"));
              break;
            }
          }
        }
      }
      else
      break;
    }


    // Chiude la connessione con il client
    close(childSocket);
    fprintf(stderr, "Conessione chiusa!\n-----------------------------\n");
  }


  // Chiude il socket principale
  close(varianzaSocket);
  return 0;
}
