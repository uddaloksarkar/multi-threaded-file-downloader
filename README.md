# Multi Threaded File Downloader

It is a `wget` like file downloader allowing __more than one thread__. 

## Building
You might need a unix-like system to build this program (NOT compatible with windows environment). To build use the following command: ```gcc -pthread multi-thread-dwnl.c -o mdnl -lncurses -lssl -lcrypto```

 

Note that, the code uses the following libraries __POSIX-thread-libraries__, __ncurses__ and __a portion of OpenSSL which supports TLS ( SSL and TLS Protocols )__.
So, you may want to install the dependencies. For _OpenSLL_ and _ncurse_ part you may install the followings: 

```sudo apt install libssl-dev```

```sudo apt-get install libncurses5-dev libncursesw5-dev```

 
## Running 
To run the compiled executable: 
```./mdnl [url] [# of threads]```
To pause/resume downloading you will beed to press `Enter`.
