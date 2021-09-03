#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <curses.h>
#include <unistd.h>
#include <sys/time.h>
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
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFF 4096
#define Malloc(n,type) (type *) malloc( (unsigned) ((n)*sizeof(type)))

char out_buff[BUFF], in_buff[BUFF] = "";

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
  char location[2048];

}HEADER;


typedef struct{

  SSL* ssl;
  int fd;
  int size;
  int id;
  unsigned long end;
  unsigned long offset;

}INFO;


void log_ssl()
{
    int err;
    while (err = ERR_get_error()) {
        char *str = ERR_error_string(err, 0);
        if (!str)
            return;
        printf(str);
        printf("\n");
        fflush(stdout);
    }
}


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


int extractHeaders(SSL* sock){
  int bytesReceived, bytes = 0;

  in_buff[0] = '-';in_buff[1] = '-';in_buff[2] = '-';in_buff[3] = '\n'; //filling initial buffer
  char* p = in_buff + 4;
  
  while(bytesReceived = SSL_read(sock, p, 1)){
    if((p[-3]=='\r')  && (p[-2]=='\n' ) &&
          (p[-1]=='\r')  && (*p=='\n' )) break;
      p++;
    bytes += bytesReceived;
  }

return bytes;
}


HEADER* parseHeader(SSL* sock){

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

    else if (strcmp(token2, "Location:") == 0){
      strcpy(header->location, token);
    }
    
  }

  puts("parseHeader Done");

  return header;
}


void *download (void *arg){
  
  pthread_mutex_lock(&mutex);

  INFO *info = (INFO*) arg;

  int bytes, bytesReceived;
  bytes = 0;
  
  int load = info->end - info->offset;

  fprintf(stderr, "Thread %d -- with workload %d bytes...\n", info->id, load);

  while(bytesReceived = SSL_read(info->ssl, in_buff, BUFF)){

    pthread_mutex_lock(&lock);
    
    write(info->fd, in_buff, bytesReceived);
    bytes += bytesReceived;

    print_progress("Downloading...", (int)bytes, (int)load);
    // fprintf(stderr, "Thread Downloading...%d\r", bytes);
  

    pthread_mutex_unlock(&lock);

    if(bytes >= load) break;
  }

  fprintf(stderr, "\nThread %d Downloaded %d bytes\n", info->id, bytes);

  if(bytes < load) puts("File Downloaded Successfully before last thread ends!");
  
  puts("\n");
  
  pthread_mutex_unlock(&mutex);

}


void *pauseDnld (void *arg){

  int play = 0;
  char c ;

  while(! (c = getchar() == 'p') ){
    
    pthread_mutex_lock(&lock);
    getchar();
    pthread_mutex_unlock(&lock);

    // puts("\r");

  }


}


// void parseURL(char* url, char* domain, char* path){

// }


int main(int ac, char* av[]){

  struct timeval strt, stop;

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
  server_addr.sin_port = htons(443);
  server_addr.sin_addr = *((struct in_addr *)h->h_addr);


  fprintf(stderr, "Connecting to server...\n");
  if (0 > connect(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))){
       perror("Unable to Connect");
       // exit(1);
    }
  fprintf(stderr, "Connected. \n");


  // SSL connection

  SSL *ssl;
  SSL_library_init();
  SSLeay_add_ssl_algorithms();
  SSL_load_error_strings();
  const SSL_METHOD *meth = TLSv1_2_client_method();
  SSL_CTX *ctx = SSL_CTX_new (meth);
  ssl = SSL_new (ctx);
  if (!ssl) {
      printf("Error creating SSL.\n");
      log_ssl();
      return -1;
  }
  // sock = SSL_get_fd(ssl);
  SSL_set_fd(ssl, sock);
  int err = SSL_connect(ssl);
  if (err <= 0) {
      printf("Error creating SSL connection.  err=%x\n", err);
      log_ssl();
      fflush(stdout);
      return -1;
  }
  printf ("SSL connection using %s\n", SSL_get_cipher (ssl));



  //send dnld information

  
  fprintf(stderr, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

  snprintf(out_buff, sizeof(out_buff), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);

  fprintf(stderr, "Sending request: \n\n%s\n", out_buff);

  if (0 > SSL_write(ssl, out_buff, strlen(out_buff)))
    perror("Error in sending to server");


  // Receiving data

  fprintf(stderr, "Receiving...\n\n");

  HEADER* header = Malloc(1, HEADER);
  header  = parseHeader(ssl);

  printf("status: %s %s\t size: %ld\t date: %s\n", header->status, header->statusName, header->size, header->date);
  

  while(strcmp(header->status,"302") == 0){

    strcpy(url, header->location);

    fprintf(stderr,"redirecting to:  %s\n",header->location);
    
    // parseURL(header->location, &domain, &path);
    
    protocol = strtok_r(url, "://", &url);

    domain = strtok_r(url, "/", &url);
    path = url;
    
    puts(path);
    snprintf(out_buff, sizeof(out_buff), "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", path, domain);
    if (0 > SSL_write(ssl, out_buff, strlen(out_buff)))
      perror("Error in sending to server");
    header  = parseHeader(ssl);
    fprintf(stderr, "status: %s %s\t size: %ld\t date: %s\n", header->status, header->statusName, header->size, header->date);
  
  }

  
  // Deriving the name of the file
  if(strcmp(header->status, "200") &&  strcmp(header->status, "302")) {
    puts("request could not be completed, exiting...");
    exit(1);
  }

  char* fname = "unknown.file"; int bytesReceived;
  char* token = path;

  while(token){
    fname = token;
    token = strtok_r(path, "/", &path);
  }

  fprintf(stderr, "Downloading file: '%s' ...\n\n", fname);

  int fd = open(fname, O_WRONLY|O_CREAT);


  // Create threads and distribute

  pthread_t *tid, t;

  int N = atoi(av[2]);
  // INFO *info = Malloc(N, INFO);
  tid = Malloc(N, pthread_t);

  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&lock, NULL);

  pthread_create(&t, NULL, pauseDnld, NULL);

  unsigned long thread_capacity = header->size / N;
  unsigned long* offset = Malloc(N, unsigned long);
  unsigned long* end = Malloc(N, unsigned long);
    offset[0] = 0; end[0] = thread_capacity;
    for (int i=1; i<N; i++){
      offset[i] = end[i-1];
      end[i] =  offset[i] + thread_capacity;
    }
    end[N-1] = header->size;

    INFO** info;
    info = Malloc(N, INFO*);
    for (int i = 0; i<N; i++){
      info[i] = Malloc(1, INFO);
    }

  gettimeofday(&strt, NULL);

  for(int i=0; i<N; i++){
    info[i]->fd = fd;  //TODO make global
    info[i]->offset = offset[i];
    info[i]->end = end[i];
    info[i]->ssl = ssl;
    info[i]->id = i;
    // printf("New thread %d\n", i);
    if (-1 == pthread_create(&tid[i], NULL, download, (void *) info[i]))
          perror("pthread_create failed");
  }
  for(int i=0; i<N; i++){
    pthread_join(tid[i], NULL);
  }
  gettimeofday(&stop, NULL);
  
  // pthread_join(t, NULL); //joining this will give abrupt pause at end, so rather NOT

  close(fd);


fprintf(stderr, "'%s' Successfully Downloaded :) \n\n", fname);


float secs = (float)(stop.tv_usec - strt.tv_usec) / 1000000 + (float)(stop.tv_sec - strt.tv_sec);

fprintf(stderr, "Time taken : %fs\t Speed : %.2f MB/s\n\n", secs, (float)header->size/( 1024 * 1024 * secs));

}
