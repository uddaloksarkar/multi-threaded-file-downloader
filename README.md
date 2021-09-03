# multi-threaded-file-downloader

It is a `wget` like file downloader allowing more than one thread. 

To compile use the following command: ```gcc -pthread multi-thread-dwnl.c -o mdnl -lncurses -lssl -lcrypto```
Note that, it uses the following libraries `POSIX-thread-libraries`, `ncurses` and _a portion of OpenSSL which supports TLS ( SSL and TLS Protocols )_.
So, you may want to install the dependencies. 

To run the executable: ```./mdnl [url] [# of treads]```
