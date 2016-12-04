# packt-claim-bot
Get your daily free book automatically using this bot on Packt
# Istructions:
usage:
```bash
BOTpackt [-l <login>] [-p <password>] [-d y|n] [-s]
```
**-d** you can automatically download the ebook in the .pdf format as soon as it claims it

**-s** the program returns immediately after
###In Linux:
```bash
crontab -e
mm hh * * * /path_to/packt-claim-bot/bin/BOTpkt
```
###In Windows:
* Create a .bat executable like the one provided and put in the same folder as the bot
* Start up Task Scheduler, create a new task specifying the .bat file
	 Specify an hour at which start daily and don't forget to enable the 
         ["Run task as soon as possible after a scheduled start is missed" option]
* Enjoy

If you use different credentials don't forget to remove the cookie.txt file
# Compile from source
You need some libraries to be available in your system:
* Libcurl
* [Htmlstreamparser-0.4](http://code.google.com/p/htmlstreamparser/)
* Openssl
* Zlib

### Ubuntu dynamic link example
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
gcc main.c $(pkg-config --libs --cflags libcurl) -lcrypto -lssl -lhtmlstreamparser -o BOTpkt
```
### Windows static link example using MinGW
```cmd
gcc main.c -static $(/local/bin/curl-config --static-libs --cflags) -lssl -lhtmlstreamparser -o BOTpkt
```
