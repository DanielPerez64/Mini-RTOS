/*
 * 29/07/2024
 *
 * Este es un codigo de un mini RTOS como tarea escolar. Realiza cambio de contexto, agenda las tareas y
 * ejecuta un pequeño kernel para seleccionar las tareas con las prioridades asignadas.
 *
 * El codigo fue desarrollado en MCUXPresso, de  NXP. En la tarjeta de desarrollo FRDM K64F
 *
 * Autor: Daniel Perez Montes
 *
*/
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

#include "RGB_ledcolors.h"

#define RTOS_STACK_MAX 200
#define RTOS_TASK_MAX 7
#define RTOS_STACK_FRAME_SIZE 8
#define RTOS_STACK_PC_OFFSET 2
#define RTOS_STACK_PSR_OFFSET 1
#define RTOS_STACK_PSR_DEFAULT 0x01000000

// define si la tarea comienza lista para su ejecucion
#define AUTOSTART true

/* Origen del cambio de contexto de una tarea */
typedef enum
{
	excepcion,
	tarea
}origenes;

/* Seleccion de colores en el driver del led RGB */
typedef enum
{
	RED,
	BLUE,
	GREEN,
	CYAN,
	YELLOW,
	VIOLET,
	WHITE,
    OFF
} color_t;

/* Estados para la tarea */
typedef enum
{
	READY,
	RUNNING,
	WAITING
} taskStatus_t;

/* Esta es la estructura de una tarea, almacenamos:
 *
 * 	spTask: 		Stack Pointer de la tarea en ejecucion
 * 	stackTask[]: 	Creamos un arreglo para almacenar los bytes empleados en la tarea
 * 	status:			Definimos en que estado se encuentra la tarea
 * 	delay:			El tiempo en el cual la tarea se encuentra en estado Waiting antes de pasar a estado Ready
 * 	priority:		La prioridad para que el kernel ejecute la tarea aun estado en estado Ready.
 * 	dummy:			Por cuestiones del alineamiento de memoria que realiza el archivo linker del compilador de MCUXpresso, la cantidad de registros
 * 					que se necesitaban para guardar las tareas en este ejemplo no cuadraban en todos los casos, dando como resultado un desfase en el
 * 					stack pointer con cada cambio de contexto. Agregando esta variable dummy a la estructura, el cambio de contexto ocurre de manera
 * 					cuadrada sin desfases, sin embargo es un detalle que no consigo comprender al 100 por ciento.
 * */
typedef struct
{
	void (*func)(void *);
	uint32_t * spTask;
	uint32_t stackTask[RTOS_STACK_MAX];
    taskStatus_t status;
    uint32_t delay;
    uint32_t priority;
    uint32_t dummy;	/* Sin uso */
} tasks_t;

/* Estructura empleada para la calendarizacion de las tareas */
struct {
	uint32_t global_clock;
	int8_t currentTask;
	int8_t nextTask;
	uint8_t counterTask;
	tasks_t task[RTOS_TASK_MAX+1];
} task_list = {0};

/* Prototipos de funcion de minirtos */
void minirtos_contextSwitch (origenes origen);
void minirtos_kernel (origenes origen);
void minirtos_activateWaitingTasks ( void );
void PendSV ( void );
void minirtos_startScheduler (void);
void minirtos_delay (uint8_t ticks);
uint32_t minirtos_createTask(void (*task_body)(void *), uint32_t priority, uint32_t autostart_flag);

//volatile uint32_t g_systickCounter = 10;

/* tareas a generar */
void blinking_led(void *args);
void idle_task(void *args);

/* comienzo de la aplicacion */
int main(void)
{
    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
    BOARD_InitDebugConsole();

    /*********************************************/
    // CONFIGURACION DE LOS LEDS DE LA TARJETA

    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortE);

    PORT_SetPinMux(PORTB, 21U, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTB, 22U, kPORT_MuxAsGpio);
	PORT_SetPinMux(PORTE, 26U, kPORT_MuxAsGpio);

    gpio_pin_config_t LED_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U
    };

    GPIO_PinInit(GPIOB, 22, &LED_config);
    GPIO_PinInit(GPIOB, 21, &LED_config);
    GPIO_PinInit(GPIOE, 26, &LED_config);

    /*********************************************/

    NVIC_SetPriority(PendSV_IRQn, 0xFF); // Asigna prioridad minima a la interrupcion del PendSV

    PRINTF("Hello World\r\n");

    /*********************************************/
    // INICIALIZACION DE LAS TAREAS Y EL SCHEDULER

    task_list.currentTask = -1;
    task_list.nextTask = -1;

    minirtos_createTask (blinking_led, 3, AUTOSTART);
    minirtos_createTask (blinking_led, 2, AUTOSTART);
    minirtos_createTask (blinking_led, 4, AUTOSTART);
    minirtos_createTask (blinking_led, 5, AUTOSTART);
    minirtos_createTask (blinking_led, 2, AUTOSTART);
    minirtos_createTask (blinking_led, 1, AUTOSTART);
    minirtos_createTask (blinking_led, 4, AUTOSTART);

    minirtos_startScheduler();

    /*********************************************/

    while(1) {
        __asm volatile ("nop");
    }
    return 0 ;
}

void idle_task(void *args)
{
    while (1)
    {
        Off_led();
        task_list.task[task_list.nextTask].status = READY;
        PRINTF("IDLE 3 $$$$$$$$$$$$$$$$$$$$\r\n");
    }
}

void blinking_led(void *args)
{
    color_t color = OFF;

    if(task_list.currentTask != (task_list.counterTask-1))
    {
	    color = task_list.currentTask;
    }

	while(1)
	{
		switch(color)
		{
			case RED:
				Rojo ();
				PRINTF("rojo 0 ++++++++++++++++++++\n\r");
				break;
			case BLUE:
				Azul();
				PRINTF("azul 1 *********************\n\r");
				break;
			case GREEN:
				Verde ();
				PRINTF("verde 2 ????????????????????\n\r");
				break;
			case YELLOW:
				Amarillo ();
				PRINTF("amarillo 4 ++++++++++++++++++++\n\r");
				break;
			case  VIOLET:
				Morado ();
				PRINTF("morado 5 ++++++++++++++++++++\n\r");
				break;
			case  CYAN:
				Cyan ();
				PRINTF("cyan 3 ++++++++++++++++++++\n\r");
				break;
			case WHITE:
				Blanco();
				PRINTF("blanco 6 ++++++++++++++++++++\n\r");
				break;
			case OFF:
				Off_led ();
				//PRINTF("Tarea IDLE $$$$$$$$$$$$$$$$$$$$\r\n");
				break;
			default:
				Off_led ();
				break;
		}

		minirtos_delay((color+1)*5);

	}
}

void PendSV_Handler(void)
{
	register uint32_t r0 asm("r0");

	SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk; // limpia la interrupcion del registro
	r0 = (uint32_t)task_list.task[task_list.currentTask].spTask; // recupera el stack de la tarea

	asm("mov r7, r0");	// mover el registro r0 al registro r7
}

void SysTick_Handler(void)
{
	++task_list.global_clock;	// cuenta los ticks que lleva la ejecucion del programa
	minirtos_activateWaitingTasks(); //reduce el delay de las tareas
	minirtos_kernel(excepcion);	// ejecuta el kernel

}

void minirtos_contextSwitch (origenes origen)
{
	static uint8_t firstTime = 0;
	register uint32_t r0 asm("r0");

	/* La primera vez que se ejecuta el mini rtos no se realiza ningun cambio de contexto */
	if(firstTime)
	{
		asm("mov r0, r7");
		// si el origen del cambio de contexto ocurre desde una tarea, ejecuta este desfase
		if(origen==tarea){
			task_list.task[task_list.currentTask].spTask = (uint32_t*)r0-10;
		}
		// si el cambio de contexto se invoca desde una excepcion, realiza este desfase
		else
		{
			task_list.task[task_list.currentTask].spTask = (uint32_t*)r0+11;
		}


	}
	else
	{
		firstTime = 1;
	}

	// cambia el estado de la tarea actual
	if(task_list.task[task_list.currentTask].status == RUNNING)
	{
		task_list.task[task_list.currentTask].status = READY;
	}
	// la sieguiente tarea en ejecutarse se convierte en la tarea actual
	task_list.currentTask = task_list.nextTask;
	// la tarea proxima a ejecutarse pasa a estado Running
	task_list.task[task_list.nextTask].status = RUNNING;
	// invoca el cambio de contexto
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void minirtos_kernel (origenes origen)
{
	// SCHEDULER
	uint8_t highestPriority = 0;
	for (uint8_t var = 0; var < task_list.counterTask; var++)
	{
		if((task_list.task[var].status == READY || task_list.task[var].status == RUNNING) && task_list.task[var].priority >= highestPriority)
		{
			highestPriority = task_list.task[var].priority;
			task_list.nextTask = var;
		}
	}

	// comprueba si se ejecuta un cambio de contexto
    if (task_list.currentTask != task_list.nextTask)
    {
    	minirtos_contextSwitch (origen);
    }
}


/* Esta funcion actualiza el delay de cada tarea */
void minirtos_activateWaitingTasks ( void )
{
  for (uint8_t i = 0; i < task_list.counterTask-1; i++) //-1??
  {
    if (task_list.task[i].status == WAITING)
    {
    	if (!(--task_list.task[i].delay))
      {
    		task_list.task[i].status = READY;
      }
    }
  }
}

// esta tarea asigna un delay a las tareas, se ejecuta despues de la tarea
void minirtos_delay (uint8_t ticks)
{
	    task_list.task[task_list.currentTask].status=WAITING;
	    task_list.task[task_list.currentTask].delay = ticks;
		minirtos_kernel (tarea);

}

// inicializa el calendariazador y genera la tarea idle
void minirtos_startScheduler (void)
{

	task_list.global_clock = 0;
	minirtos_createTask (idle_task, 0, AUTOSTART); // tarea idle tiene la menor prioridad

	if (SysTick_Config(SystemCoreClock / 1000))
	{
		while (1)
		{
		}
	}
}

/* Crea una tarea */
uint32_t minirtos_createTask (void (*task_body)(void *), uint32_t priority, uint32_t autostart_flag)
{
	// verifica que no se haya llegado al limite de las tareas
	if (task_list.counterTask <= RTOS_TASK_MAX)
	{
		// verifica si la tarea se debe estar lista al momento de crearse
		if (autostart_flag == AUTOSTART)
		{
			task_list.task[task_list.counterTask].status = READY;
		}
		else
		{
			task_list.task[task_list.counterTask].status = WAITING;
		}

		// genera los atributos que debe tener la tarea
		task_list.task[task_list.counterTask].priority = priority;
		task_list.task[task_list.counterTask].func = task_body;
		// acomoda el puntero del stack para almacenar los registros requeridos para el funcionamiento del minirtos
		task_list.task[task_list.counterTask].spTask = &task_list.task[task_list.counterTask].stackTask[RTOS_STACK_MAX-1] - RTOS_STACK_FRAME_SIZE;
		// inicializa el program counter con la direccion de la funcion
		task_list.task[task_list.counterTask].stackTask[RTOS_STACK_MAX-RTOS_STACK_PC_OFFSET] = (uint32_t)task_body;
		// inicializa el pcr con la direccion por default
		task_list.task[task_list.counterTask].stackTask[RTOS_STACK_MAX-RTOS_STACK_PSR_OFFSET] = RTOS_STACK_PSR_DEFAULT;
		// sin delay
		task_list.task[task_list.counterTask].delay = 0;
		// recorre al siguiente espacio para las tareas
		task_list.counterTask++;

		return task_list.counterTask;
	}

	return 1;
}
