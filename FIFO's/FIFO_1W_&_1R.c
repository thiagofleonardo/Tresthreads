#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <string.h>

#define STACKSIZE 1024
#define PRIORITY 7

K_FIFO_DEFINE(my_fifo);

void escrever1(void) {
    while (1) {
        if (k_fifo_num_used_get(&my_fifo) > 10) {
            k_sleep(K_SECONDS(1));
            continue;
        }

        char *data1 = k_malloc(7);
        __ASSERT(data1 != NULL, "k_malloc failed");

        strcpy(data1, "Tarde!");
        k_fifo_put(&my_fifo, data1);

        k_sleep(K_SECONDS(1));
    }
}

void ler(void) {
    while (1) {
        char *data2 = k_fifo_get(&my_fifo, K_FOREVER);
        if (data2) {
            printk("Recebido: %s\n", data2);
            k_free(data2);
        }
        k_sleep(K_SECONDS(1));
    }
}

K_THREAD_DEFINE(thread1_id, STACKSIZE, escrever1, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
K_THREAD_DEFINE(thread2_id, STACKSIZE, ler, NULL, NULL, NULL, PRIORITY, 0, K_NO_WAIT);
