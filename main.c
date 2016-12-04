#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include <htmlstreamparser.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

char href[256]="", formID[256]="", path[256] = "", title[512] = "", href_book[256] = "";

size_t disable_write(void *buffer, size_t size, size_t nmemb, void *userp){
   return size * nmemb;
}
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
size_t parse_href(void *buffer, size_t size, size_t nmemb, FILE *hsp) {
    size_t realsize = size * nmemb, p;
    if(strlen(href)>0 && strlen(formID)>0) return realsize; //quit if already found
    char tmpHr[128], tmpId[128];
    int watchForId = 0 ;
    for(p = 0; p < realsize; p++) {
    html_parser_char_parse(hsp, ((char *)buffer)[p]);
    //search for href
    if(html_parser_cmp_tag(hsp, "a", 1)){
      if(html_parser_cmp_attr(hsp, "href", 4)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
          strcpy(tmpHr, html_parser_val(hsp));
        }
      }else if(html_parser_cmp_attr(hsp,"class",5)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
          if(!strcmp(html_parser_val(hsp), "twelve-days-claim")){
            //we are in the right class so we found our href
            strcpy(href,"https://www.packtpub.com");
            strcat(href, tmpHr);
            printf("href found: %s\n", href);
          }
        }
      }
      //search for form id
    }else if(html_parser_cmp_tag(hsp, "input", 5)){
      if(html_parser_cmp_attr(hsp, "id", 2)){
       if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
         html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
         if(!strcmp(html_parser_val(hsp), "edit-packt-user-login-form")){
           strcpy(formID, tmpId);
           printf("form_build_id found: %s\n", formID);
         }
         //remember all ids
         strcpy(tmpId, html_parser_val(hsp));
       }
     }
    }
  }
    return realsize;
}
int listen_title=0; char nid[128]="";
size_t verify_success_download(void *buffer, size_t size, size_t nmemb, FILE *hsp){
  size_t realsize = size * nmemb, p;
  if(listen_title<-1)return realsize;  //avoid restart in successive chunks
  for(p = 0; p < realsize; p++) {
    char currCh = ((char *)buffer)[p];
    html_parser_char_parse(hsp, currCh);
    if(html_parser_cmp_tag(hsp, "div", 3)){
      if(html_parser_cmp_attr(hsp, "class", 5)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
          if(strstr(html_parser_val(hsp), "product-line") != NULL && listen_title==0){
            //set listener
            listen_title = 1;
          }
        }
      }else if(listen_title==1 && html_parser_cmp_attr(hsp, "nid", 3)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
            //get title
            strcpy(nid, html_parser_val(hsp));
        }
      }else if(listen_title==1 && html_parser_cmp_attr(hsp, "title", 5)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
            //get title
            strcpy(title, html_parser_val(hsp));
            listen_title = -1;
        }
      }
    }else if(html_parser_cmp_tag(hsp, "a", 1)&&listen_title==-1){
      if(html_parser_cmp_attr(hsp, "href", 4)){
        if(html_parser_is_in(hsp, HTML_VALUE_ENDED)) {
          html_parser_val(hsp)[html_parser_val_length(hsp)] = '\0';
          //check if string contains pdf
          if(strstr(html_parser_val(hsp), "pdf") != NULL && strstr(html_parser_val(hsp), nid) != NULL){
            listen_title = -2;
            strcpy(href_book, "https://www.packtpub.com");
            strcat(href_book, html_parser_val(hsp));
            return realsize;
          }
        }
      }
    }
  }
  return realsize;
}
int request(CURL *curl){
     CURLcode res;
     res = curl_easy_perform(curl);
       if(res != CURLE_OK){
          fprintf(stderr, "curl perform failed: \n%s\n",
              curl_easy_strerror(res));
          system("pause");
       return 0;
       }
       return 1;
}
static CURLcode sslctx_function(CURL * curl, void * sslctx, void * parm){
       FILE *fp;
       //reopen the file for reading
       fp = fopen(path, "r+");
       X509_STORE * store = SSL_CTX_get_cert_store((SSL_CTX *)sslctx); //get store
       char mypem[7000], tmp[128]; mypem[0]='\0'; tmp[128]='\0';
       int reading = 0;
       while(fgets (tmp, 128, fp)!=NULL ){
            //add to tmp until end of certificate
            if(reading) strcat(mypem, tmp);
            if(!strcmp(tmp, "GeoTrust Global CA\n"))
              reading = 1;

            //if we reached end of a cert
            if(reading && !strcmp(tmp, "-----END CERTIFICATE-----\n")){
              reading = 0;
                 /* get a BIO, create cert */
                  BIO * bio = BIO_new_mem_buf(mypem, -1);
                  X509 * cert = NULL;
                  PEM_read_bio_X509(bio, &cert, 0, NULL);
                  if(cert == NULL) printf("PEM_read_bio_X509 failed...\n");
                  //add to the store
                  if(!X509_STORE_add_cert(store, cert)) printf("error adding certificate\n");
                  /* decrease reference counts */
                  X509_free(cert);
                  BIO_free(bio);
                //reset cert string
                mypem[0]='\0';
            }
       }
       printf("ssl certificate loaded\n");
       fclose(fp);
       return CURLE_OK;
}
int init(int argc, char *argv[], char* location, char* login, char* passwd, int* download, int* silent){
int suc=0;
#ifdef __linux__
  int opt;
  while ((opt = getopt(argc, argv, "l:p:d:s")) != -1) {
    switch (tolower(opt)) {
      case 'l':
        suc |= 1;
        strcpy(login, optarg);
      break;
      case 'p':
        suc |= 2;
        strcpy(passwd, optarg);
      break;
      case 'd':
        *download = optarg[0]=='y';
      break;
      case 's':
        *silent = 1;
      break;
      default:
        return 0;
    }
  }
#elif _WIN32
  //manually parse win arguments
  int i = 1, stArg=0;
  for (; i<argc; i++){
    if(argv[i][0]=='-'){
      stArg = tolower(argv[i][1]);
      //arguments without parameters
      switch(stArg){
        case 's':
          *silent = 1;
        break;
      }
    }else if(stArg){
      //parse argument
      switch (stArg) {
        case 'l':
          suc |= 1;
          strcpy(login, argv[i]);
        break;
        case 'p':
          suc |= 2;
          strcpy(passwd, argv[i]);
        break;
        case 'd':
          *download = argv[i][0]=='y';
        break;
        default:
          return 0;
      }
      stArg = 0;
    }
  }
#endif
  return suc==3;
}
int main(int argc, char *argv[]){
    CURL *curl;
    char login[128]="", passw[128]="";
    int a=0, b=0; int *download = &a, *silent= &b;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl){
//Download certificates
       FILE *fp;
    #ifdef __linux__
       readlink("/proc/self/exe", path, sizeof(path));
    #elif _WIN32
       _fullpath(path, argv[0], strlen(argv[0])+1);
    #endif
       while(path[strlen(path)-1]!='\\' && path[strlen(path)-1]!='/') path[strlen(path)-1] = '\0';  //remove executable name
       char basePath[128];
       strcpy(basePath,path);
       strcat (path,"cacert.pem");

       //init logpassw
       if(!init(argc, argv, basePath, login, passw, download, silent)){
         fprintf(stderr, "Usage: %s [-l login] [-p password] [-d y|n](download book) [-s](silent mode)\n", argv[0]);
         return -1;
       }

        //---save ca certificates in exe location
       fp = fopen(path, "wb");
       curl_easy_setopt(curl, CURLOPT_URL, "https://curl.haxx.se/ca/cacert.pem");
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
       if(!request(curl))return 0;
       fclose(fp);
//Main request : get page href
       //initialize stream and attributes to htmlParser
       HTMLSTREAMPARSER *hsp = html_parser_init();
       char tag[5], attr[5], val[256];

       html_parser_set_tag_to_lower(hsp, 1);
       html_parser_set_attr_to_lower(hsp, 1);
       html_parser_set_tag_buffer(hsp, tag, sizeof(tag));
       html_parser_set_attr_buffer(hsp, attr, sizeof(attr));
       html_parser_set_val_buffer(hsp, val, sizeof(val)-1);

       curl_easy_setopt(curl, CURLOPT_URL, "https://www.packtpub.com/packt/offers/free-learning");
       curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
       curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
       curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, *sslctx_function);
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, parse_href);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, hsp);
       curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
       if(!request(curl))return 0;

//Log in to site and remember using cookies
       curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, 0L);
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, disable_write);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0L);
       char post[1024]="";
        strcpy(post, "email=");
        strcat(post, login);
        strcat(post, "&password=");
        strcat(post, passw);
        strcat(post, "&op=Login&form_build_id=") ;
        strcat(post, formID);
        strcat(post, "&form_id=packt_user_login_form");

       curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
       curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookie.txt");  //cookies to remember Login
       curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "cookie.txt");
       if(!request(curl))return 0;

//Get to found href to get free ebook
       curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, verify_success_download);
       curl_easy_setopt(curl, CURLOPT_WRITEDATA, hsp);
       curl_easy_setopt(curl, CURLOPT_URL, href);
       if(!request(curl))return 0;
       //check success
    #ifdef __linux__
       if(strlen(title)) {printf("\x1B[32mLogin successful\nBook successfully acquired: %s\n", title);}
       else {printf("\x1B[31mERROR: login wasn't successful, try changing the login.psswd file or deleting cookie.txt\n");}
    #elif _WIN32
      if(strlen(title)) {system("color 02"); printf("Login successful\nBook successfully acquired: %s\n", title);}
      else {system("color 04"); printf("ERROR: login wasn't successful, try deleting cookie.txt\n");}
    #endif
//download Book
      if(*download){
        printf("Downloading pdf\n");
        char tmpPath[128]; strcpy(tmpPath,basePath); strcat(tmpPath,strcat(title,".pdf"));
        fp = fopen(tmpPath, "wb");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_URL, href_book);
        if(!request(curl))return 0;
        fclose(fp);
        printf("File downloaded\n");
      }

       //cleanup
       html_parser_cleanup(hsp);
       curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    if((*silent && !strlen(title))||!*silent)
  #ifdef __linux__
    getchar();
  #elif _WIN32
    system("pause");
  #endif
}
