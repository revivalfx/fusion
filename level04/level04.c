#include "../common/common.c"

// original code from micro_httpd_12dec2005.tar.gz -- acme.com. added vulnerabilities etc ;)

/* micro_httpd - really small HTTP server
**
** Copyright (c) 1999,2005 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/


#define SERVER_NAME "level04.c"
#define SERVER_URL "https://gist.github.com/b69116098bcc6ef7dfb4"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"


/* Forwards. */
static void file_details( char* dir, char* name );
static void send_error( int status, char* title, char* extra_header, char* text );
static void send_headers( int status, char* title, char* extra_header,
  char* mime_type, off_t length, time_t mod );
static char* get_mime_type( char* name );
static void strdecode( char* to, char* from );
static int hexit( char c );
static void strencode( char* to, size_t tosize, const char* from );

int webserver(int argc, char **argv);

// random decoder 
void build_decoding_table();

char *password;
int password_size = 16;

int main(int argc, char **argv)
{
  int fd, i;
  char *args[6];

  /* Securely generate a password for this session */

  secure_srand();
  password = calloc(password_size, 1);
  for(i = 0; i < password_size; i++) {
      switch(rand() % 3) {
          case 0: password[i] = (rand() % 25) + 'a'; break;
          case 1: password[i] = (rand() % 25) + 'A'; break;
          case 2: password[i] = (rand() % 9) + '0'; break;
      }
  }

  // printf("password is %s\n", password);

  background_process(NAME, UID, GID); 
  fd = serve_forever(PORT);
  set_io(fd);
  alarm(15);

  args[0] = "/opt/fusion/bin/stack06";
  args[1] = "/opt/fusion/run";
  args[2] = NULL;

  build_decoding_table();

  webserver(2, args);
}

// random decoder from stackoverflow
// modified to make more vulnerable

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void base64_decode(const char *data,
                    size_t input_length,
                    unsigned char *output,
                    size_t *output_length) {

    if (decoding_table == NULL) build_decoding_table();

    // printf("data: %p, input_length: %d, output: %p, output_length: %p\n",
    // data, input_length, output, output_length);

    if ((input_length % 4) != 0) {
  // printf("len % 4 = fail\n");
  return;
    }

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
                        + (sextet_b << 2 * 6)
                        + (sextet_c << 1 * 6)
                        + (sextet_d << 0 * 6);

        if (j < *output_length) output[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) output[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) output[j++] = (triple >> 0 * 8) & 0xFF;
    }
}


void build_decoding_table() {

    decoding_table = malloc(256);

    for (int i = 0; i < 0x40; i++)
        decoding_table[encoding_table[i]] = i;
}


void base64_cleanup() {
    free(decoding_table);
}


// end random decoder

int validate_credentials(char *line)
{
  char *p, *pw;
  unsigned char details[2048];
  int bytes_wrong;
  int l;
  struct timeval tv;
  int output_len;


  memset(details, 0, sizeof(details));

  output_len = sizeof(details);

  p = strchr(line, '\n');
  if(p) *p = 0;
  p = strchr(line, '\r');
  if(p) *p = 0;

  // printf("%d\n", strlen(line));
  base64_decode(line, strlen(line), details, &output_len);
  // printf("%s -> %s\n", line, details);
  // fflush(stdout);
  
  p = strchr(details, ':');
  pw = (p == NULL) ? (char *)details : p + 1;

  for(bytes_wrong = 0, l = 0; pw[l] && l < password_size; l++) {
      if(pw[l] != password[l]) {
          
#if 0
          char *buf;
          asprintf(&buf, "[%d] wrong byte (%02x vs %02x)\n", l,
                          password[l], pw[l]);
          write(58, buf, strlen(buf));
#endif
          
          bytes_wrong++;
      }
  }

  // anti bruteforce mechanism. good luck ;>
  
  tv.tv_sec = 0;
  tv.tv_usec = 2500 * bytes_wrong;

  select(0, NULL, NULL, NULL, &tv);

  // printf("%d bytes wrong!\n", bytes_wrong);

  if(l < password_size || bytes_wrong)
      send_error(401, "Unauthorized",
      "WWW-Authenticate: Basic realm=\"stack06\"",
      "Unauthorized");

  return 1;
}

int
webserver( int argc, char** argv )
    {
    char line[10000], method[10000], path[10000], protocol[10000], idx[20000];
    char location[20000], command[20000];
    char* file;
    size_t len;
    int ich;
    struct stat sb;
    FILE* fp;
    struct dirent **dl;
    int i, n;
    int authed = 0;

    if ( argc != 2 )
  send_error( 500, "Internal Error", (char*) 0,
  "Config error - no dir specified." );
    if ( chdir( argv[1] ) < 0 )
  send_error( 500, "Internal Error", (char*) 0,
  "Config error - couldn't chdir()." );
    if ( fgets( line, sizeof(line), stdin ) == (char*) 0 )
  send_error( 400, "Bad Request", (char*) 0,
  "No request found." );
    if ( sscanf( line, "%[^ ] %[^ ] %[^ ]", method, path, protocol ) != 3 )
  send_error( 400, "Bad Request", (char*) 0, "Can't parse request." );
    while ( fgets( line, sizeof(line), stdin ) != (char*) 0 )
  {
        if ( strncmp ( line, "Authorization: Basic ", 21) == 0)
      authed = validate_credentials(line + 21);
      
  if ( strcmp( line, "\n" ) == 0 || strcmp( line, "\r\n" ) == 0 )
      break;
  }
    if ( ! authed) send_error(401, "Unauthorized",
      "WWW-Authenticate: Basic realm=\"stack06\"",
      "Unauthorized");

    if ( strcasecmp( method, "get" ) != 0 )
  send_error( 501, "Not Implemented", (char*) 0,
  "That method is not implemented." );
    if ( path[0] != '/' )
  send_error( 400, "Bad Request", (char*) 0, "Bad filename." );
    file = &(path[1]);
    strdecode( file, file );
    if ( file[0] == '\0' )
  file = "./";
    len = strlen( file );
    if ( file[0] == '/' || strcmp( file, ".." ) == 0 ||
      strncmp( file, "../", 3 ) == 0 || strstr( file, "/../" ) != (char*) 0 ||
      strcmp( &(file[len-3]), "/.." ) == 0 )
  send_error( 400, "Bad Request", (char*) 0, "Illegal filename." );
    if ( stat( file, &sb ) < 0 )
  send_error( 404, "Not Found", (char*) 0, "File not found." );
    if ( S_ISDIR( sb.st_mode ) )
  {
  if ( file[len-1] != '/' )
      {
      (void) snprintf(
      location, sizeof(location), "Location: %s/", path );
      send_error( 302, "Found", location, "Directories must end with a slash." );
      }
  (void) snprintf( idx, sizeof(idx), "%sindex.html", file );
  if ( stat( idx, &sb ) >= 0 )
      {
      file = idx;
      goto do_file;
      }
  send_headers( 200, "Ok", (char*) 0, "text/html", -1, sb.st_mtime );
  (void) printf( "<html><head><title>Index of %s</title></head>\n"
      "<body bgcolor=\"#99cc99\"><h4>Index of %s</h4>\n<pre>\n",
      file, file );
  n = scandir( file, &dl, NULL, alphasort );
  if ( n < 0 )
      perror( "scandir" );
  else
      for ( i = 0; i < n; ++i )
      file_details( file, dl[i]->d_name );
  (void) printf( "</pre>\n<hr>\n<address><a href=\"%s\">%s</a></address>"
      "\n</body></html>\n", SERVER_URL, SERVER_NAME );
  }
    else
  {
  do_file:
  fp = fopen( file, "r" );
  if ( fp == (FILE*) 0 )
      send_error( 403, "Forbidden", (char*) 0, "File is protected." );
  send_headers( 200, "Ok", (char*) 0, get_mime_type( file ), sb.st_size,
      sb.st_mtime );
  while ( ( ich = getc( fp ) ) != EOF )
      putchar( ich );
  }

    (void) fflush( stdout );
    exit( 0 );
    }


static void
file_details( char* dir, char* name )
    {
    static char encoded_name[1000];
    static char path[2000];
    struct stat sb;
    char timestr[16];

    strencode( encoded_name, sizeof(encoded_name), name );
    (void) snprintf( path, sizeof(path), "%s/%s", dir, name );
    if ( lstat( path, &sb ) < 0 )
  (void) printf( "<a href=\"%s\">%-32.32s</a>    ???\n",
      encoded_name, name );
    else
  {
  (void) strftime( timestr, sizeof(timestr), "%d%b%Y %H:%M",
      localtime( &sb.st_mtime ) );
  (void) printf( "<a href=\"%s\">%-32.32s</a>    %15s %14lld\n",
      encoded_name, name, timestr, (int64_t) sb.st_size );
  }
    }


static void
send_error( int status, char* title, char* extra_header, char* text )
    {
    send_headers( status, title, extra_header, "text/html", -1, -1 );
    (void) printf( "<html><head><title>%d %s</title></head>\n<body "
      "bgcolor=\"#cc9999\"><h4>%d %s</h4>\n", status,
      title, status, title );
    (void) printf( "%s\n", text );
    (void) printf( "<hr>\n<address><a href=\"%s\">%s</a></address>"
      "\n</body></html>\n", SERVER_URL, SERVER_NAME );
    (void) fflush( stdout );
    exit( 1 );
    }


static void
send_headers( int status, char* title, char* extra_header,
  char* mime_type, off_t length, time_t mod )
    {
    time_t now;
    char timebuf[100];

    (void) printf( "%s %d %s\015\012", PROTOCOL, status, title );
    (void) printf( "Server: %s\015\012", SERVER_NAME );
    now = time( (time_t*) 0 );
    (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &now ) );
    (void) printf( "Date: %s\015\012", timebuf );
    if ( extra_header != (char*) 0 )
  (void) printf( "%s\015\012", extra_header );
    if ( mime_type != (char*) 0 )
  (void) printf( "Content-Type: %s\015\012", mime_type );
    if ( length >= 0 )
  (void) printf( "Content-Length: %lld\015\012", (int64_t) length );
    if ( mod != (time_t) -1 )
  {
  (void) strftime( timebuf, sizeof(timebuf), RFC1123FMT, gmtime( &mod ) );
  (void) printf( "Last-Modified: %s\015\012", timebuf );
  }
    (void) printf( "Connection: close\015\012" );
    (void) printf( "\015\012" );
    }


static char*
get_mime_type( char* name )
    {
    char* dot;

    dot = strrchr( name, '.' );
    if ( dot == (char*) 0 )
  return "text/plain; charset=iso-8859-1";
    if ( strcmp( dot, ".html" ) == 0 || strcmp( dot, ".htm" ) == 0 )
  return "text/html; charset=iso-8859-1";
    if ( strcmp( dot, ".jpg" ) == 0 || strcmp( dot, ".jpeg" ) == 0 )
  return "image/jpeg";
    if ( strcmp( dot, ".gif" ) == 0 )
  return "image/gif";
    if ( strcmp( dot, ".png" ) == 0 )
  return "image/png";
    if ( strcmp( dot, ".css" ) == 0 )
  return "text/css";
    if ( strcmp( dot, ".au" ) == 0 )
  return "audio/basic";
    if ( strcmp( dot, ".wav" ) == 0 )
  return "audio/wav";
    if ( strcmp( dot, ".avi" ) == 0 )
  return "video/x-msvideo";
    if ( strcmp( dot, ".mov" ) == 0 || strcmp( dot, ".qt" ) == 0 )
  return "video/quicktime";
    if ( strcmp( dot, ".mpeg" ) == 0 || strcmp( dot, ".mpe" ) == 0 )
  return "video/mpeg";
    if ( strcmp( dot, ".vrml" ) == 0 || strcmp( dot, ".wrl" ) == 0 )
  return "model/vrml";
    if ( strcmp( dot, ".midi" ) == 0 || strcmp( dot, ".mid" ) == 0 )
  return "audio/midi";
    if ( strcmp( dot, ".mp3" ) == 0 )
  return "audio/mpeg";
    if ( strcmp( dot, ".ogg" ) == 0 )
  return "application/ogg";
    if ( strcmp( dot, ".pac" ) == 0 )
  return "application/x-ns-proxy-autoconfig";
    return "text/plain; charset=iso-8859-1";
    }


static void
strdecode( char* to, char* from )
    {
    for ( ; *from != '\0'; ++to, ++from )
  {
  if ( from[0] == '%' && isxdigit( from[1] ) && isxdigit( from[2] ) )
      {
      *to = hexit( from[1] ) * 16 + hexit( from[2] );
      from += 2;
      }
  else
      *to = *from;
  }
    *to = '\0';
    }


static int
hexit( char c )
    {
    if ( c >= '0' && c <= '9' )
  return c - '0';
    if ( c >= 'a' && c <= 'f' )
  return c - 'a' + 10;
    if ( c >= 'A' && c <= 'F' )
  return c - 'A' + 10;
    return 0;       /* shouldn't happen, we're guarded by isxdigit() */
    }


static void
strencode( char* to, size_t tosize, const char* from )
    {
    int tolen;

    for ( tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from )
  {
  if ( isalnum(*from) || strchr( "/_.-~", *from ) != (char*) 0 )
      {
      *to = *from;
      ++to;
      ++tolen;
      }
  else
      {
      (void) sprintf( to, "%%%02x", (int) *from & 0xff );
      to += 3;
      tolen += 3;
      }
  }
    *to = '\0';
    }

