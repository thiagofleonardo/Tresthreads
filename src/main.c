#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/mutex.h>
// bibliotecas 

#define SLEEP_TIME_MS 1000 //tempo de dormir

#define LED0_NODE DT_ALIAS(led0) //identifica no DT o LED
#define SW0_NODE DT_ALIAS(sw0) //identifica no DT o botão

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios); //configura o LED0 na variável led
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0}); //configura o SWO na variável button e se nao definir antes ele é zero
static struct gpio_callback button_dados; //cria a variável button_dados para armazenar os dados do callback do botão

static struct k_mutex mutex_periodico; //mutex para a função 1
static struct k_mutex mutex_button; //mutex para a função 2

/* Função 1*/
void funcao_periodica(struct k_work *work) {
    int ret;
    static bool led_state = true;

    /* Tenta bloquear o mutex_periodico*/
    if (k_mutex_lock(&mutex_periodico, K_MSEC(100)) == 0) { //ele tenta bloquear a cada 100ms
        ret = gpio_pin_toggle_dt(&led); //se bloqueia, alterna o valor de led
        if (ret < 0) { //se ele não consegue bloquear:
            printk("Error LED\n");
            k_mutex_unlock(&mutex_periodico); //e solta o mutex periodico para nao dar um deadlock
            return;
        }

        led_state = !led_state;
        printk("LED estado: %s\n", led_state ? "ON" : "OFF");

        k_mutex_unlock(&mutex_periodico);
    } else {
        printk("Falha no block para função 1\n");
    }

    k_msleep(SLEEP_TIME_MS); //coloca em sleep a thread
}

/* Função 2 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button pressed\n");

    if (k_mutex_lock(&mutex_button, K_MSEC(100)) == 0) {
        printk("Button event processed under mutex\n");

        k_mutex_unlock(&mutex_button);
    } else {
        printk("Failed to lock mutex for button event\n");
    }
}

/* Função ativada quando as duas anteriores forem ativadas */
void combined_function(struct k_work *work) {
    if (k_mutex_lock(&mutex_periodico, K_MSEC(100)) == 0) {
        if (k_mutex_lock(&mutex_button, K_MSEC(100)) == 0) {
            printk("Combined function activated\n");
            k_mutex_unlock(&mutex_button);
        } else {
            printk("Failed to lock mutex for button event\n");
        }
        k_mutex_unlock(&mutex_periodico);
    } else {
        printk("Failed to lock mutex for combined function\n");
    }
}

K_WORK_DEFINE(periodic_work, funcao_periodica); // Definindo o trabalho correto
K_WORK_DEFINE(combined_work, combined_function); // Definindo o trabalho correto

int main(void) {
    int ret;

    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device %s is not ready\n", button.port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return 0;
    }

    gpio_init_callback(&button_dados, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_dados);

    printk("Press the button to trigger the event\n");

    /* Configura o LED */
    if (!gpio_is_ready_dt(&led)) {
        printk("Error: LED device %s is not ready\n", led.port->name);
        return 0;
    }
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret != 0) {
        printk("Error: failed to configure LED device %s pin %d\n", led.port->name, led.pin);
        return 0;
    } else {
        printk("LED is configured\n");
    }

    /* Inicializa o mutex */
    k_mutex_init(&mutex_periodico);
    k_mutex_init(&mutex_button);

    while (1) {
        /* Ativa a função periódica */
        k_work_submit(&periodic_work);

        /* Verifica se a interrupção do botão ocorreu */
        if (gpio_pin_get_dt(&button) == 1) {
            /* Quando o botão for pressionado, ativa a função combinada */
            k_work_submit(&combined_work);
        }

        k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
