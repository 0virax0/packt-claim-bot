# packt-claim-bot
Get your daily free book automatically using this bot on Packt
# Istructions:
* Place this directory where you prefer and leave it there
* Start BOTpkt.exe for the first configuration
* Start up Task Scheduler, create a new task specifying either BOTpkt.exe or start_silently if you want no prompt.
	 Specify an hour at which start daily and don't forget to enable the 
         ["Run task as soon as possible after a scheduled start is missed" option]
* Enjoy

If you want to change your credentials or something goes wrong you have to delete BOTH "login.passwd" and "cookie.txt"
# Compile from source
You need some libraries to be available in your system:
* Libcurl
* Htmlstreamparser-0.4
* Openssl
* Zlib

### Ubuntu dynamic link example
```bash
gcc main.c $(pkg-config --libs --cflags libcurl) -lssl -lhtmlstreamparser -o BOTpkt
```
### Windows static link example using MinGW
```cmd
gcc main.c -static $(/local/bin/curl-config --static-libs --cflags) -lssl -lhtmlstreamparser -o BOTpkt
```
