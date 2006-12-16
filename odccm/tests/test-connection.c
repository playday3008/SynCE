#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char *argv[])
{
  int fd, dev_fd, ret;
  struct sockaddr_un sa;
  struct msghdr msg = { 0, };
  struct cmsghdr *cmsg;
  struct iovec iov;
  char cmsg_buf[512];
  char data_buf[512];
  const char request_buf[] = "\x0c\x00\x00\x00\x4b\x00\x00\x00"
                             "\x05\x00\x00\x00\x08\x02\x00\x00";
  FILE *f;

  if (argc != 2)
    {
      fprintf (stderr, "usage: %s [path]\n", argv[0]);
      return EXIT_FAILURE;
    }

  fd = socket (AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return EXIT_FAILURE;

  sa.sun_family = AF_UNIX;
  strcpy (sa.sun_path, argv[1]);

  printf ("connecting to '%s': ", sa.sun_path);
  fflush (stdout);

  if (connect (fd, (struct sockaddr *) &sa, sizeof (sa)) < 0)
    {
      printf ("failed\n");
      return EXIT_FAILURE;
    }

  printf ("success\n");

  msg.msg_control = cmsg_buf;
  msg.msg_controllen = sizeof (cmsg_buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_flags = MSG_WAITALL;

  iov.iov_base = data_buf;
  iov.iov_len = sizeof (data_buf);

  printf ("calling recvmsg: ");
  fflush (stdout);

  ret = recvmsg (fd, &msg, 0);
  if (ret < 0)
    {
      printf ("failed\n");
      return EXIT_FAILURE;
    }

  printf ("success (ret=%d)\n", ret);

  cmsg = CMSG_FIRSTHDR (&msg);
  if (cmsg == NULL)
    {
      fprintf (stderr, "NULL?!\n");
      return EXIT_FAILURE;
    }
  else if (cmsg->cmsg_type != SCM_RIGHTS)
    {
      fprintf (stderr, "unexpected cmsg_type=%d\n", cmsg->cmsg_type);
      return EXIT_FAILURE;
    }

  dev_fd = *((int *) CMSG_DATA (cmsg));

  printf ("got dev_fd=%d\n", dev_fd);

  /* now let's try doing a RAPI call manually */
  printf ("calling send with RAPI call: ");
  fflush (stdout);

  ret = send (dev_fd, request_buf, sizeof (request_buf), 0);
  if (ret != sizeof (request_buf))
    {
      printf ("short write, ret=%d\n", ret);
      return EXIT_FAILURE;
    }

  printf ("success\n");

  printf ("calling recv to get RAPI call response: ");
  fflush (stdout);

  ret = recv (dev_fd, data_buf, sizeof (data_buf), 0);
  if (ret <= 0)
    {
      printf ("argh, ret=%d\n", ret);
      return EXIT_FAILURE;
    }

  printf ("got %d bytes\n", ret);

  f = fopen ("response.bin", "wb");
  if (f == NULL)
    {
      printf ("error opening response.bin for writing\n");
      return EXIT_FAILURE;
    }

  if (fwrite (data_buf, ret, 1, f) != 1)
    {
      printf ("short write\n");
      return EXIT_FAILURE;
    }

  printf ("response saved to response.bin\n");

  fclose (f);

  return EXIT_SUCCESS;
}

