#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

#define STACKSIZE 1024
#define PRIORITY 7

K_FIFO_DEFINE(my_fifo);

void escrita1(void)
{
    while(1){
        if (k_fifo_num_used_get(&my_fifo) > 10)
        {
            k_sleep(K_SECONDS(1));
            continue;
        }
        char *data1 = k_malloc(7);  // Allocate 7 bytes for "Tarde!" + null terminator
        __ASSERT(data1 != NULL, "k_malloc failed");

        strcpy(data1, "Tarde!");
        k_fifo_put(&my_fifo, data1);

        k_sleep(K_SECONDS(1));
    }
}

void escrita2(void)
{
    while(1){
        if (k_fifo_num_used_get(&my_fifo) > 10)
        {
            k_sleep(K_SECONDS(1));
            continue;
        }
        char *data2 = k_malloc(7);  // Allocate 7 bytes for "Noite!" + null terminator
        __ASSERT(data2 != NULL, "k_malloc failed");

        strcpy(data2, "Noite!");
        k_fifo_put(&my_fifo, data2);

        k_sleep(K_SECONDS(1));
    }
}

void leitura(void)
{
    while (1) {
        char *data3 = k_fifo_get(&my_fifo, K_FOREVER);
        if (data3) {
            printk("Received: %s\n", data3);
            k_free(data3);  // Free the memory after use
        }
        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(thread1_id, STACKSIZE, escrita1, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(thread2_id, STACKSIZE, escrita2, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(thread3_id, STACKSIZE, leitura, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);