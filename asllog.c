/* asllog - system log utility for (jailbroken) appletv2.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/sysctl.h>

#include <asl.h>
#include <err.h>
#include <sysexits.h>
#include <TargetConditionals.h>

#define TIME_FORMAT "$((Time)(local))"

static const int kMinutesPast = 5;
static const char kMessageFormat[] 
  = TIME_FORMAT " $Host $(Sender)[$(PID)] <$((Level)(str))>: $Message";
static const useconds_t kMsgPollInt = 1000 * 1000;

static int signalled = 0;
static void
on_signal(int n)
{
  signalled = 1;
  signal(n, SIG_DFL);
}

/* the following cobbled together from searches online and digging
   through various versions of apple's opensource tarballs until
   I found definitions that appeared right and didn't segfault on use. */

/* added by hand to minimize changes to definitions and code */
typedef void asl_msg_t;

/* from syslog-148: asl_msg.h */
char *asl_format_message(asl_msg_t *msg, const char *msg_fmt, 
  const char *time_fmt, uint32_t text_encoding, uint32_t *outlen);

/* http://opensource.apple.com//source/Libc/Libc-763.12/gen/asl_private.h */
typedef struct __aslresponse
{
#if TARGET_CPU_X86
  #error u will have bad time - code targets AppleTV2
  /* It's not really wise to try and use this code on anything but an atv2.
     OSX 10.11 will probably work if the missing structure fields are added
     here but I can't see much reason one would want to do so. */
#endif
  uint32_t count;
  uint32_t curr;
  asl_msg_t **msg;
} asl_search_result_t;

/* modified syslog-267 source */
static asl_object_t
asl_msg_list_next(asl_object_t listin)
{
  asl_search_result_t *list = (asl_search_result_t*)listin;
  asl_msg_t *out;

  if (list == NULL) return NULL;
  if (list->curr >= list->count) return NULL;
  if (list->msg == NULL)
  {
    list->curr = 0;
    list->count = 0;
    return NULL;
  }

  out = list->msg[list->curr];
  list->curr++;
  return (asl_object_t)out;
}

/* custom: support function to get message id as number */
static uint64_t
asl_message_id(asl_object_t msg)
{
  const char *s = asl_get(msg, ASL_KEY_MSG_ID);
  if (!s) 
    return 0;
  return atoll(s);
}

/* iterate messages, fetch item count for each and display 
 * returning the last message id seen for further queries */
static int
show_messages(asl_object_t r, uint64_t *maxid)
{
  asl_object_t msg;  
  int count = 0;

  while (!signalled && (msg = asl_msg_list_next(r))) {
    uint32_t slen;
    *maxid = asl_message_id(msg);

    char *s = asl_format_message(msg, kMessageFormat, 
      TIME_FORMAT, ASL_ENCODE_SAFE, &slen);
    assert(s);
    printf("%s", s);
    free(s);
    count++;
  }
  return count;
}

/* fetch system uptime */
static int
uptime(struct timeval *tv)
{
  size_t len = sizeof(struct timeval);
  int mib[2] = { CTL_KERN, KERN_BOOTTIME };
  return sysctl(mib, 2, tv, &len, NULL, 0);
}

/* returns seconds difference between now and a given timeval */
static double
diffnow(struct timeval *then)
{
  struct timeval now;
  double d;

  gettimeofday(&now, NULL);

  d = now.tv_sec - then->tv_sec;
  d += 0.000001 * now.tv_usec;
  d -= 0.000001 * then->tv_usec;

  return d;
}

#define do_asl_free(P) do { asl_free(P); P = NULL; } while (0)
#define NEXT_ARG(C,V) *(C--,V++)
#define SHIFT(C,V) (C > 0 ? NEXT_ARG(C,V) : NULL)

typedef enum {
  StartSinceBoot,
  StartSinceRecent,
} start_t;

int 
main(int argc, char const *argv[])
{
  asl_object_t q, r;
  uint64_t msgid = 0;
  start_t st = StartSinceRecent;
  const char *s;
  char timebuf[32];

  (void)SHIFT(argc, argv);
  while ((s = SHIFT(argc, argv))) {
    if (!strcmp(s, "-B"))
      st = StartSinceBoot;
    else if (!strcmp(s, "-w"))
      continue;
    else {
      fprintf(stderr, "unknown option %s\n", s);
      return EXIT_FAILURE;
    }
  }

  /* use line buffered output */
  (void)setlinebuf(stdout);

  /* do initial query for recent messages */
  q = asl_new(ASL_TYPE_QUERY);
  if (st == StartSinceBoot) {
    struct timeval tvup;
    if (uptime(&tvup)) {
      perror(NULL);
      return -1;
    }
    double d = diffnow(&tvup);        
    snprintf(timebuf, sizeof(timebuf), "-%.4lfs", d);
  } else {
    snprintf(timebuf, sizeof(timebuf), "-%dm", kMinutesPast);
  }
  if ((asl_set_query(q, ASL_KEY_TIME, timebuf, ASL_QUERY_OP_GREATER_EQUAL)))
    err(EX_OSERR, NULL);
  r = asl_search(NULL, q);

  /* check if we failed to find any messages in the given window */
  if (!show_messages(r, &msgid) && st == StartSinceRecent) {
    for (int mins = kMinutesPast * 2; 
              mins < kMinutesPast * 60 * 24 * 7 * 52; 
              mins += 30) {
      snprintf(timebuf, sizeof(timebuf), "-%dm", mins);
      if ((asl_set_query(q, ASL_KEY_TIME, timebuf, ASL_QUERY_OP_GREATER_EQUAL)))
        err(EX_OSERR, NULL);
      r = asl_search(NULL, q);
      if (show_messages(r, &msgid))
        break;
    }
  }
  do_asl_free(r);
  do_asl_free(q);
  
  signal(SIGINT, on_signal);
  signal(SIGQUIT, on_signal);

  /* loop forever - checking for new messages */
  while (!signalled) {
    char buf[32];
    q = asl_new(ASL_TYPE_QUERY);

    snprintf(buf, sizeof(buf), "%llu", msgid);
    if ((asl_set_query(q, ASL_KEY_MSG_ID, buf, ASL_QUERY_OP_GREATER)))
      err(EX_OSERR, NULL);
    r = asl_search(NULL, q);

    show_messages(r, &msgid);
    do_asl_free(r);
    do_asl_free(q);

    if (!signalled)
      usleep(kMsgPollInt);
  }

  return EXIT_SUCCESS;
}
