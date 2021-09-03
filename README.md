# multi-threaded-file-downloader

It is a `wget` like file downloader allowing __more than one thread__. 

To compile use the following command: ```gcc -pthread multi-thread-dwnl.c -o mdnl -lncurses -lssl -lcrypto```


Note that, the code uses the following libraries _POSIX-thread-libraries_, _ncurses_ and _a portion of OpenSSL which supports TLS ( SSL and TLS Protocols )_.
So, you may want to install the dependencies. For _OpenSLL_ and _ncurse_ part you may install the following: 
```sudo apt install libssl-dev```
```sudo apt-get install libncurses5-dev libncursesw5-dev```


To run the compiled executable: 
```./mdnl [url] [# of treads]```
