#include "kernel_service.h"
#include "k_file.h"
#include "kernel_udp.h"
#include <linux/kthread.h>

// Handles kthread initialization

struct file** f;

struct udp_service
{
  char                name[15]; // same as p_name
  struct task_struct* k_thread;
  struct socket*      sock;
};

void
k_thread_stop(struct udp_service* k)
{
  if (k && k->k_thread)
    k->k_thread = NULL;
}

static void
create_thread(struct udp_service* k, tfunc f, void* data)
{
  k->k_thread = kthread_run(f, data, k->name);
  if (k->k_thread >= 0) {
    printk(KERN_INFO "%s Thread running\n", k->name);
  } else {
    printk(KERN_ERR "%s Cannot run thread\n", k->name);
    k->k_thread = NULL;
  }
}

void
init_service(udp_service** k, char* name, unsigned char* myip, int myport,
             tfunc f, void* data)
{
  udp_service* service = kmalloc(sizeof(struct udp_service), GFP_KERNEL);
  *k = service;
  if (service) {
    strcpy(service->name, name);
    service->k_thread = NULL;
    service->sock = NULL;
    if (udp_init(&(service->sock), myip, myport) >= 0) {
      create_thread(service, f, data);
    }
  } else {
    printk(KERN_ERR "Failed to initialize %s\n", name);
  }
}

void
quit_service(udp_service* k)
{
  int ret;
  if (k) {
    if (k->k_thread) {
      if ((ret = kthread_stop(k->k_thread)) == 0) {
        printk(KERN_INFO "%s Terminated thread\n", k->name);
      } else {
        printk(KERN_INFO "%s Error %d in terminating thread\n", k->name, ret);
      }
    } else {
      printk(KERN_INFO "%s Thread was already stopped\n", k->name);
    }

    release_socket(k->sock);
    printk(KERN_INFO "%s Released socket\n", k->name);
    printk(KERN_INFO "%s Module unloaded\n", k->name);
    kfree(k);
  } else {
    printk(KERN_INFO "Module was NULL, terminated\n");
  }
}

char*
get_service_name(udp_service* k)
{
  return k->name;
}

struct socket*
get_service_sock(udp_service* k)
{
  return k->sock;
}

void
check_params(unsigned char* dest, char* src)
{
  if (src != NULL)
    sscanf(src, "%hhu.%hhu.%hhu.%hhu", &dest[0], &dest[1], &dest[2], &dest[3]);
}

void
adjust_name(char* print, char* src, int size_name)
{
  size_t len =
    strlen(src) + 1 > size_name - 3 ? size_name - 3 : strlen(src) + 1;
  memcpy(print, src, len);
  memcpy(print + len - 1, ":", 2);
}
