#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define STACKSIZE 1024
#define PRIORITY 7
#define SLEEP_TIME_MS 1000

#define SW0_NODE DT_ALIAS(sw0)

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

K_MUTEX_DEFINE(my_mutex1);
K_MUTEX_DEFINE(my_mutex2);

/* pisca o led0, chamada pelo timer em blink0 */
void blink_led(struct k_timer *timer_id) {
    static bool led_state = false;

    if (gpio_is_ready_dt(&led0)) {
        gpio_pin_toggle_dt(&led0); //inverte o estado atual do led, fazendo-o piscar
        led_state = !led_state; //acompanha o estado do led, como uma variavel booleana
        if (led_state == true) { //se o led estiver ligado
            printk("LED0 ligado e mutex travado\n");
            k_mutex_lock(&my_mutex1, K_FOREVER); 
        } else { //se o led estiver desligado
            printk("LED0 desligado e mutex destravado\n");
            k_mutex_unlock(&my_mutex1); 
        }
        printk("LED0 alternado; estado=%s\n", led_state ? "ON" : "OFF");
    }
}

K_TIMER_DEFINE(blink_timer, blink_led, NULL);

/* funcao chamada pela thread1, verifica erros e inicializa a funcao periodica*/
void blink0(void) {
    int ret;

    if (!gpio_is_ready_dt(&led0)) {
        printk("Erro: dispositivo %s não está pronto\n", led0.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Erro: Falha ao configurar o pino do LED\n");
        return;
    }

    k_timer_start(&blink_timer, K_NO_WAIT, K_MSEC(SLEEP_TIME_MS));
}

volatile bool led1_state = false;  // Variável para armazenar o estado do LED1

/* Liga o led1 quando o botao é ativado, pegando o mutex2 */
void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Botão pressionado em %" PRIu32 "\n", k_cycle_get_32());

    led1_state = !led1_state; // Alternar o estado do LED1
    if (led1_state == true) { // Se o LED1 estiver ligado
        printk("LED1 ligado e mutex travado\n");
        k_mutex_lock(&my_mutex2, K_FOREVER); 
    } else { // Se o LED1 estiver desligado
        printk("LED1 desligado e mutex destravado\n");
        k_mutex_unlock(&my_mutex2); 
    }
    gpio_pin_set_dt(&led1, led1_state); // Atualiza o estado do LED1, de acordo com a variável led1_state
}

/* verifica todos os possiveis erros e inicializa/registra o callback */
void button(void) {
    int ret1;

    if (!gpio_is_ready_dt(&button)) {
        printk("Erro: dispositivo do botão %s não está pronto\n", button.port->name);
        return;
    }

    ret1 = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret1 != 0) {
        printk("Erro %d: falha ao configurar o pino %d do dispositivo %s\n", ret1, button.pin, button.port->name);
        return;
    }

    ret1 = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret1 != 0) {
        printk("Erro %d: falha ao configurar interrupção no pino %d do dispositivo %s\n", ret1, button.pin, button.port->name);
        return;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);
    printk("Configuração do botão em %s pino %d\n", button.port->name, button.pin);

    if (!gpio_is_ready_dt(&led1)) {
        printk("Erro: dispositivo LED1 %s não está pronto\n", led1.port->name);
        return;
    }

    ret1 = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret1 < 0) {
        printk("Erro: Falha ao configurar o pino do LED1\n");
        return;
    }
}

/* quando ambos os mutex estsao travados. Nao pensei em uma funcao especifica, usei apenas para mostrar
o estado dos mutex */
void terceira(){
    while(1){
        if (k_mutex_lock(&my_mutex1, K_MSEC(100)) == 0 && k_mutex_lock(&my_mutex2, K_MSEC(100)) == 0){
            printk("Ambos Travados\n");
        }
        else{
            printk("NAO ESTAO ambos travados\n");
        }
        k_msleep(SLEEP_TIME_MS);
    }
}

K_THREAD_DEFINE(blink0_id, STACKSIZE, blink0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(button_id, STACKSIZE, button, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(terceira_id, STACKSIZE, terceira, NULL, NULL, NULL, PRIORITY, 0, 0);