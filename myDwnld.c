#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <curses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stddef.h>

#define BUFF 4096
#define Malloc(n,type) (type *) malloc( (unsigned) ((n)*sizeof(type)))

char out_buff[BUFF], in_buff[BUFF] = "";
int ENDFLAG = 0;
pthread_mutex_t mutex;
pthread_mutex_t lock;

typedef struct {

  char protocol[8];
  char version[4];
  char status[5];
  char statusName[10];
  char date[128];
  unsigned long size;
  char content_type[128];

}HEADER;


typedef struct{

  int sock;
  int fd;
  int size;

}INFO;


void
print_progress(char label[], int step, int total)
{
  //progress width
    const int pwidth = 50;

    //minus label len
    int width = pwidth - strlen( label );
    int pos = ( step * width ) / total ;

    
    int percent = ( step * 100 ) / total;

    printf( "%s[", label );

    //fill progress bar with =
    for ( int i = 0; i < pos-1; i++ )  printf( "%c", '=' );
    printf("%c", '>');

    //fill progress bar with spaces
    printf( "% *c", width - pos + 1, ']' );
    printf( " %3d%%\r", percent );

    fflush(stdout);
}


int extractHeaders(int sock){
  int bytesReceived, bytes = 0;

  in_buff[0] = '-';in_buff[1] = '-';in_buff[2] = '-';in_buff[3] = '\n'; //filling initial buffer
  char* p = in_buff + 4;
  
  while(bytesReceived = read(sock, p, 1)){
    if((p[-3]=='\r')  && (p[-2]=='\n' ) &&
          (p[-1]=='\r')  && (*p=='\n' )) break;
      p++;
    bytes += bytesReceived;
  }

return bytes;
}


HEADER* parseHeader(int sock){

  HEADER* header = Malloc(1, HEADER);
  
  int bytes = extractHeaders(sock);

  puts("---HEADER---");
  puts(in_buff);

  int fields = -1; //for 1 extra '\r\n' at the end
  for (int i=1; i< BUFF; i++){
    if (in_buff[i] == '\n' && in_buff[i-1] == '\r') fields++;
  }

  char* read_int = in_buff;
  char *token, *token2;
  token = strtok_r(read_int, "\n", &read_int);

  strcpy(header->protocol, strtok_r(read_int, "/", &read_int));
  strcpy(header->version, strtok_r(read_int, " ", &read_int));
  strcpy(header->status, strtok_r(read_int, " ", &read_int));
  strcpy(header->statusName, strtok_r(read_int, "\r\n", &read_int));

  // Header Parsing (More to add) :

  for(int i = 1; i < fields; i++){
    
    token = strtok_r(read_int, "\r\n", &read_int);
    token2 = strtok_r(token, " ", &token);
    
    // fprintf(stderr, "token2 %s\n", token2);
    // fprintf(stderr, "token %s\n", token);
    
    if (strcmp(token2, "Date:") == 0){
      strcpy(header->date, token); 
    }
    
    else if (strcmp(token2, "Content-Length:") == 0){
      header->size = atoi(token);
    }

    else if (strcmp(token2, "Content-Type:") == 0){
      strcpy(header->content_type, token);
    }
    
  }

  puts("parseHeader Done");

  return header;
}


void *download (void *arg){
  
  INFO *info = (INFO*) arg;

  int bytes, bytesReceived;
  bytes = 0;

  while(bytesReceived = read(info->sock, in_buff, BUFF)){

    pthread_mutex_lock(&lock);
    
    write(info->fd, in_buff, bytesReceived);
    bytes += bytesReceived;

    print_progress("Downloading..", (int)bytes, (int)info->size);
    // fprintf(stderr, "Thread Downloading...%f%\r", 100 * (float)bytes / (float)info->size);
    
    // header->size -= bytesReceived;
    // if (header->size <= 0)
    //   break;

    pthread_mutex_unlock(&lock);

  }
  puts("\n");

}


void *pauseDnld (void *arg){

  int play = 0;
  char c ;

  while(! (c = getchar() == 'p') ){
    
    pthread_mutex_lock(&lock);
    getchar();
    pthread_mutex_unlock(&lock);

    // puts("\r");

    if (ENDFLAG) break;
  }


}



int main(int ac, char* av[]){
  

  char* url = av[1];
  
  char* protocol;
  protocol = strtok_r(url, "://", &url);

  char* domain = strtok_r(url, "/", &url);
  char* path = url;


  // char domain[] = "web.stanford.edu", path[]="class/cs276/19handouts/lecture7-probir-6per.pdf";
  

  struct hostent* h = gethostbyname(domain);
  fprintf(stderr, "%s\n", h->h_addr);
  fprintf(stderr, "%ld\n", ((struct in_addr *)h->h_addr)->s_addr);
  fprintf(stderr, "%s\n\n", inet_ntoa(*(struct in_addr *)h->h_addr));

  /*
  struct sockaddr_in {
    short            sin_family;   // e.g. AF_INET
    unsigned short   sin_port;     // e.g. htons(3490)
    struct in_addr   sin_addr;     // see struct in_addr, below
    char             sin_zero[8];  // zero this if you want to
  };

  struct in_addr {
      unsigned long s_addr;  // load with inet_aton()
  };


  struct hostent {
   char *h_name;       /* official name of host * 
   char **h_aliases;   /* alias list *
   int h_addrtype;     /* host address type *
   int h_length;       /* length of address *
   char **h_addr_list; /* list of addresses *
  };

  */

  //socket creation
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  //specify the address
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(80);
  server_addr.sin_addr = *((struct in_addr *)h->h_addr);


  fprintf(stderr, "Connecting to server...\n");
  if (0 > connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))){
       perror("Unable to Connect");
       // exit(1);
    }
  fprintf(stderr, "Connected. \n");


  //send dnld information

  
  fprintf(stderr, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

  snprintf(out_buff, sizeof(out_buff), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

  fprintf(stderr, "Sending request: \n\n%s\n", out_buff);

  if (0 > send(sock, out_buff, strlen(out_buff), 0))
    perror("Error in sending to server");


  // Receiving data

  fprintf(stderr, "Receiving...\n\n");


  HEADER* header = Malloc(1, HEADER);
  header  = parseHeader(sock);

  printf("status: %s %s\t size: %ld\t date: %s\n", header->status, header->statusName, header->size, header->date);

  char* fname = "unknown.file"; int bytesReceived;
  char* token = url;

  // Deriving the name of the file
  while(token){
    fname = token;
    token = strtok_r(url, "/", &url);
  }

  fprintf(stderr, "Downloading file: %s  ...\n", fname);

  int fd = open(fname, O_WRONLY|O_CREAT);

  pthread_t tid, t;
  INFO *info = Malloc(1, INFO);

  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&lock, NULL);

  pthread_create(&t, NULL, pauseDnld, NULL);

  for(int i=0; i<1; i++){
    info->fd = fd;  //TODO make global
    // info[i]->offset = offset[i];
    info->size = header->size;
    info->sock = sock;
    // info[i]->t_id = i;
    printf("New thread %d\n", i);
    if (-1 == pthread_create(&tid, NULL, download, (void *) info))
          perror("pthread_create failed");
  }
  for(int i=0; i<1; i++){
    pthread_join(tid, NULL);
  }
  ENDFLAG = 1;
  // pthread_join(t, NULL); //joining this will give abrupt pause at end, so rather NOT


  // int bytes;

  // while(bytesReceived = read(sock, in_buff, BUFF)){
    
  //   write(fd, in_buff, bytesReceived);
  //   bytes += bytesReceived;

  //   print_progress("Downloading..", (int)bytes, (int)header->size);
  //   // fprintf(stderr, "\r %f", (float)bytes / (float)header->size);
    
  //   // header->size -= bytesReceived;
  //   // if (header->size <= 0)
  //   //   break;
  // }


  close(fd);


fprintf(stderr, "DONE :) \n");

}
