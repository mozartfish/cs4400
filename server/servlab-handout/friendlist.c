/*
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
// static void serve_request(int fd, dictionary_t *query);
/** additional functions tu support the client requests */
static void serve_friends(int fd, dictionary_t *query);
static void serve_befriend(int fd, dictionary_t *query);
static void serve_unfriend(int fd, dictionary_t *query);
static void serve_introduce(int fd, dictionary_t *query);
static void add_friends(char *friend_one, char *friend_two);
static void remove_friends(char *friend_one, char *friend_two);

/**Global variables */
dictionary_t *user_dict; // a global dictionary for keeping track of clients and their friends

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  // initialize a new dictionary for keeping track of friends
  user_dict = make_dictionary(COMPARE_CASE_INSENS, free);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1)
  {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0)
    {
      Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      doit(connfd);
      Close(connfd);
    }
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);

  if (!parse_request_line(buf, &method, &uri, &version))
  {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  }
  else
  {
    printf("URI = %s received\n", uri);
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1"))
    {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    }
    else if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    }
    else
    {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      print_stringdictionary(query);

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */
      printf("call a handler\n");

      if (starts_with("/friends", uri))
      {
        printf("call the friend handler\n");
        serve_friends(fd, query);
      }
      else if (starts_with("/befriend", uri))
      {
        printf("call the befriend handler\n");
        serve_befriend(fd, query);
      }
      else if (starts_with("/unfriend", uri))
      {
        printf("call the friend handler\n");
        serve_unfriend(fd, query);
      }
      else if (starts_with("/introduce", uri))
      {
        printf("call introduce handler\n");
        serve_introduce(fd, query);
      }

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while (strcmp(buf, "\r\n"))
  {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }

  return d;
}

void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest)
{
  char *len_str, *type, *buffer;
  int len;

  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");

  buffer = malloc(len + 1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded"))
  {
    parse_query(buffer, dest);
  }

  free(buffer);
}

static char *ok_header(size_t len, const char *content_type)
{
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_friends - print out all the friends of a particular user
 * if they exist
 */
static void serve_friends(int fd, dictionary_t *query)
{
  size_t len;
  char *body, *header;
  const char *user = dictionary_get(query, "user");

  // const size_t max_length = 1753;

  // create an empty string for the body
  body = "";

  // get the friends of the user
  dictionary_t *user_friends_dict = dictionary_get(user_dict, user);

  // the friends are stored as a dictionary pair
  // where the mapping is <user, null> to represent
  // a set of friends
  // if the user has no friends, the dictionary value will be null
  if (user_friends_dict != NULL)
  {
    const char **friends = dictionary_keys(user_friends_dict);
    body = join_strings(friends, '\n');
  }

  // debugging statement to make sure something is happening
  // printf("HELLO WORLD\n");
  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);
}

/*
 * serve-befriend handler - stub for the befriend handler
 */
static void serve_befriend(int fd, dictionary_t *query)
{
  size_t len;
  char *body, *header;
  char *user = dictionary_get(query, "user");
  char *friends = (char *)dictionary_get(query, "friends");
  char **friend_list = split_string(friends, '\n');
  int i;

  // print information about the friends
  for (i = 0; friend_list[i] != NULL; ++i)
  {
    // get the friend and see if its in the dictionary
    char *friend = friend_list[i];
    add_friends(user, friend);
  }

  // get the user friends
  dictionary_t *user_friends = dictionary_get(user_dict, user);
  const char **friend_names_list = dictionary_keys(user_friends);
  body = join_strings(friend_names_list, '\n');

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/*
 * serve_unfriend - stub for the unfriend handler
 */
static void serve_unfriend(int fd, dictionary_t *query)
{
  size_t len;
  char *user = dictionary_get(query, "user");
  char *body, *header;
  char *friends = dictionary_get(query, "friends");
  char **friend_list = split_string((char *)(friends), '\n');
  int i;

  // print information about the friends
  for (i = 0; friend_list[i] != NULL; ++i)
  {
    // get the friend and see if its in the dictionary
    char *friend = friend_list[i];
    remove_friends(user, friend);
  }

  // get the user friends
  dictionary_t *user_friends = dictionary_get(user_dict, user);
  const char **friend_names_list = dictionary_keys(user_friends);
  body = join_strings(friend_names_list, '\n');

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/*
 * serve-introduce handler - stub for the introduce handler
 */
static void serve_introduce(int fd, dictionary_t *query)
{
  size_t len;
  char *body, *header;

  body = strdup("alice\nbob");

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  free(body);
}

/** Function that adds friends to the global dictionary if they do not exist*/
static void add_friends(char *user_one, char *user_two)
{
  printf("Enter the add function\n");

  // check for user one
  if ((dictionary_t *)(dictionary_get(user_dict, user_one)) == NULL)
  {
    dictionary_t *new_user_one = (dictionary_t *)(make_dictionary(COMPARE_CASE_SENS, free));
    dictionary_set(user_dict, user_one, new_user_one);
  }

  // check for user two
  if ((dictionary_t *)(dictionary_get(user_dict, user_two)) == NULL)
  {
    dictionary_t *new_user_two = (dictionary_t *)(make_dictionary(COMPARE_CASE_SENS, free));
    dictionary_set(user_dict, user_two, new_user_two);
  }

  // check if the names are duplicates
  // the most recent user gets added 
  if (strcmp(user_one, user_two) == 0)
  {
    return;
  }

  printf("print the new people\n");
  print_stringdictionary(user_dict);
  printf("end the new people print\n");

  printf("add new people as friends\n");
  // add user two to user one dictionary
  dictionary_set((dictionary_t *)(dictionary_get(user_dict, user_one)), user_two, NULL);

  // add user one to user two dictionary
  dictionary_set((dictionary_t *)(dictionary_get(user_dict, user_two)), user_one, NULL);

  print_stringdictionary(user_dict);
  printf("end the new people as friends\n");
  return;
}

/** Function that removes friends from the global dictionary */
static void remove_friends(char *user_one, char *user_two)
{
  printf("enter the remove function\n");
  // check if the user one dictionary is null
  if (dictionary_get(user_dict, user_one) == NULL)
  {
    return;
  }

  // check if the user two dictionary is null
  if (dictionary_get(user_dict, user_two) == NULL)
  {
    return;
  }

  // CHECK 1 :  user names are the same
  // the most recent gets removed 
  if (strcmp(user_one, user_two) == 0)
  {
    return;
  }

  // get the friends of user_one and user_two
  // add user two to user one dictionary
  dictionary_remove((dictionary_t *)(dictionary_get(user_dict, user_one)), user_two);

  // add user one to user two dictionary
  dictionary_remove((dictionary_t *)(dictionary_get(user_dict, user_two)), user_one);
  printf("print the users\n");

  print_stringdictionary(user_dict);
  return;
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
                 char *shortmsg, char *longmsg)
{
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor="
                        "ffffff"
                        ">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);

  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d)
{
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++)
  {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
/*
 * serve_request - example request handler
 */
// static void serve_request(int fd, dictionary_t *query)
// {
//   size_t len;
//   char *body, *header;

//   body = strdup("alice\nbob");

//   len = strlen(body);

//   /* Send response headers to client */
//   header = ok_header(len, "text/html; charset=utf-8");
//   Rio_writen(fd, header, strlen(header));
//   printf("Response headers:\n");
//   printf("%s", header);

//   free(header);

//   /* Send response body to client */
//   Rio_writen(fd, body, len);

//   free(body);
// }