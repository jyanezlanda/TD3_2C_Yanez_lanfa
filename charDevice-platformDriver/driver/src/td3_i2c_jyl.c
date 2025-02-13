#include "../inc/devDriver.h"
#include "../inc/BBB_i2c_Reg.h"
#include "../inc/mpu_6050_reg.h"

MODULE_DEVICE_TABLE( of, i2c_of_device_ids );

static DECLARE_WAIT_QUEUE_HEAD(queue);
static unsigned int wake_up_rx = 0;
static unsigned int wake_up_tx = 0;

void __iomem * cmper_baseAddr;
void __iomem * i2c2_baseAddr;
void __iomem * ctlmod_baseAddr;
volatile int virq;

static int __init i2c_init(void)
{
    int status = 0;
    pr_alert("[Init]-$ Set CharDriver \n");
    if (( state.myi2c_cdev = cdev_alloc() )== NULL) //Alloc ChrDev 
    {
        pr_alert("[Init]-$ No es posible realizar el alloc\n");
        return -1;
    }

    if( (status = alloc_chrdev_region(&state.myi2c, MENOR, CANT_DISP, COMPATIBLE)) < 0) //Usa dev_t para buscar el numero mayor, el compatible es el del device tree NO TRABAJA CON EL DEV TREE
    {
        pr_alert("[Init]-$ No es posible asignar el numero mayor \n");
        return status;
    }

    pr_alert("[Init]-$ Numero MAYOR asignado %d, 0x%X \n", MAJOR(state.myi2c), MAJOR(state.myi2c));

    cdev_init(state.myi2c_cdev, &i2c_ops); //No tiene codigo de error

    if( (status = cdev_add(state.myi2c_cdev, state.myi2c, CANT_DISP)) < 0)
    {
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        pr_alert("[Init]-$ No fue posible añadir el dispositivo\n");
        return status;
    }

    if((state.myi2c_class = class_create(THIS_MODULE,CLASS_NAME)) == NULL) // Aca se agrega al /dev o es necesario para esto
    {
        pr_alert("[Init]-$ No se pudo crear la clase para el device\n");
        cdev_del(state.myi2c_cdev);
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        //cdev_free(state.myi2c_cdev);
        return EFAULT;
    } 

    state.myi2c_class->dev_uevent = change_permission_cdev; //Se ejecuta cuando device_create salga bien

    //Creo el device
    if((device_create(state.myi2c_class, NULL, state.myi2c, NULL, COMPATIBLE)) == NULL)
    {
        pr_alert("[Init]-$ No se pudo crear el device\n");
        class_destroy(state.myi2c_class);
        cdev_del(state.myi2c_cdev);
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        return EFAULT;
    }
    pr_alert("[Init]-$ Inicialización del modulo terminada exitosamente que capo sos \n");

    /* A partir de aca me meto con Platform Driver para manejar el i2c*/

    if((status = platform_driver_register(&i2c_pd)) < 0)
    {
        class_destroy(state.myi2c_class);
        device_destroy(state.myi2c_class, state.myi2c);
        cdev_del(state.myi2c_cdev);
        unregister_chrdev_region(state.myi2c, CANT_DISP);
        return status;
    }

    return 0;
}

static void __exit i2c_exit(void)
{
    printk(KERN_ALERT "[Exit]-$ Cerrando el CharDriver\n");

    platform_driver_unregister(&i2c_pd);
    device_destroy(state.myi2c_class, state.myi2c);
    class_destroy(state.myi2c_class);
    cdev_del(state.myi2c_cdev);
    unregister_chrdev_region(state.myi2c, CANT_DISP);

    return;
}

static int change_permission_cdev(struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0666); // el device se le da permisos
    return 0;
}

static int i2c_probe(struct platform_device * i2c_pd)
{
    int status = 0;
    int aux;

    pr_alert("[Probe]-$ Entre al PROBE\n");

    /* Mapeo el registro CM_PER*/

    if((cmper_baseAddr = ioremap(CM_PER, CM_PER_LEN)) == NULL)  //  Modifica la paginación para poder acceder a los registros y configurar el i2c
    {
        pr_alert("[Probe]-$ No pudo mapear CM_PER\n");
        return 1;
    }

    pr_info("[Probe]-$ cmper_baseAddr: 0x%X\n", (unsigned int)cmper_baseAddr);

    /* Mapeo el registro CTLMOD*/
    if((ctlmod_baseAddr = ioremap(CTRL_MODULE_BASE, CTRL_MODULE_LEN)) == NULL)  //  Modifica la paginación para poder acceder a los registros y configurar el i2c
    {
        pr_alert("[Probe]-$ No pudo mapear CTRL_MODULE\n");
        iounmap(cmper_baseAddr);
        return 1;
    }

    pr_info("[Probe]-$ ctlmod_baseAddr: 0x%X\n", (unsigned int)ctlmod_baseAddr);

    /* Mapre el registro I2C2*/
    if((i2c2_baseAddr = ioremap(I2C2, I2C2_LEN)) == NULL)  //  Modifica la paginación para poder acceder a los registros y configurar el i2c
    {
        pr_alert("[Probe]-$ No pudo mapear I2C2\n");
        iounmap(ctlmod_baseAddr);
        iounmap(cmper_baseAddr);
        return 1;
    }

    pr_info("[Probe]-$ i2c2_baseAddr: 0x%X\n", (unsigned int)i2c2_baseAddr);

    /* ACA SOLICITO AL SO UNA VIRTUAL IRQ */

    /* Solicito un número de IRQ*/

    if((virq = platform_get_irq(i2c_pd, 0)) < 0)
    {
        pr_alert("[Probe]-$ No se pudo obtener una VIRQ\n");
        iounmap(i2c2_baseAddr);
        iounmap(ctlmod_baseAddr);
        iounmap(cmper_baseAddr);
        return 1;
    }

    /* Asocio la virq a mi handler*/
    if(request_irq(virq, (irq_handler_t) jyl_i2c_irq_handler, IRQF_TRIGGER_RISING, COMPATIBLE, NULL))
    {
        pr_alert("[Probe]-$ No se pudo obtener asociar la VIRQ al handler\n");
        iounmap(i2c2_baseAddr);
        iounmap(ctlmod_baseAddr);
        iounmap(cmper_baseAddr);
        return 1;
    }

    pr_info("[Probe]-$ Numero de interrupción: %d\n", virq);

    /* Se comienza con la lógica del I2C*/

    aux = ioread32( cmper_baseAddr + IDCM_PER_I2C_CLKCTRL );
    aux |= 0x02;
    iowrite32(aux, cmper_baseAddr + IDCM_PER_I2C_CLKCTRL);

    pr_alert("[Probe]-$ Clock seteado crack.\n");
    do{
        msleep(1);
        pr_info("[Probe]-$ Configurando registro CM_PER... El clock no malviajes \n");
    }while(ioread32(cmper_baseAddr + IDCM_PER_I2C_CLKCTRL) != 2);
    pr_alert("[Probe]-$ Ya se estabilizo el clock.\n");
    msleep(10);

    pr_info("[Probe]-$ Termina el PROBE maquina\n");
    return status;
}

static int i2c_remove(struct platform_device * i2c_pd)
{
    int status = 0;

    pr_alert("[Remove]-$ Lo remuevo todo en REMOVE\n");

    iounmap(i2c2_baseAddr);
    iounmap(ctlmod_baseAddr);
    iounmap(cmper_baseAddr);
    
    free_irq(virq, NULL);

    pr_alert("[Remove]-$ Salí del REMOVE\n");
    return status;
}

uint32_t init_mpu6050 (void)
{
    uint8_t *dataWrite;
    uint8_t aux;
    uint32_t cant;
    uint8_t saMpu6050;

    pr_info("[Inicio MPU]-$ Inicializando MPU6050.\n");    

    cant = readSensor(WHO_AM_I, &saMpu6050, 1);   // Checks device ID
    pr_info("[Inicio MPU]-$ Device ID -> WHO_AM_I = 0x%x\n", saMpu6050);

    if(saMpu6050 != I2C_SA_ADDR )
    {
        pr_info("[Inicio MPU]-$ UNKOWN DEVICE ID.\n");
		return -1;
    }

    dataWrite = (uint8_t *)kmalloc( 2 * sizeof(uint8_t), GFP_KERNEL);
	if(dataWrite == NULL) {
		pr_alert("[Inicio MPU]-$ Error en kmalloc para buff data.\n");
		return -1;
	}
    
    dataWrite[0] = PWR_MGMT_1;
    dataWrite[1] = 0x0;
    writeSensor(dataWrite, 2);  // Alimento el dispositivo
    cant = readSensor(PWR_MGMT_1, &aux, 1);

    pr_info("[Inicio MPU][DEBUG]-$ PWR_MGMT_1 = 0x%x",aux);
    if(aux == 0)
        pr_info("[Inicio MPU][DEBUG]-$ Dandole energía al dispositivo.\n");
    else{
        pr_info("[Inicio MPU][DEBUG]-$ Error alimentando el dispositivo.\n");
        return -1;
    }

    dataWrite[0] = GYRO_CONFIG;
    dataWrite[1] = 0x10;  //Bit [3:4] - FS_SEL - This sets the scale range of the gyroscope. 0= -+250°/s; 1= -+500°/S; 2= -+1000°/S; 3= -+00°/S
    writeSensor(dataWrite, 2);   // Configs 1000°/s
    cant = readSensor(GYRO_CONFIG, &aux, 1);
    pr_info("[Inicio MPU][DEBUG]-$ GYRO_CONFIG = 0x%x",aux);


    dataWrite[0] = ACCEL_CONFIG;
    dataWrite[1] = 0x00;      //Bit [3:4] - AFS_SEL - This sets the scale range of the accelerate. 0= -+2g; 1= -+4g; 2= -+8g; 3= -+16g
    writeSensor(dataWrite, 2); // Configuro que el sensor vaya entre +-2G
    aux = readSensor(ACCEL_CONFIG, &aux, 1);
    pr_info("[Inicio MPU][DEBUG]-$ ACCEL_CONFIG = 0x%x", aux);

    pr_info("[Inicio MPU]-$ Se inicializo el sensor ya esta.\n");
    
    kfree(dataWrite);
    return 0;
}

uint32_t readSensor ( uint32_t registro, uint8_t * dataRx, ssize_t lenRx )
{
    uint32_t i = 0;
    uint32_t aux, ret;
    
    aux = ioread32(i2c2_baseAddr + I2C_IRQSTATUS_RAW);
    while (aux & I2C_BUS_BUSY_MASK)
    {
        msleep(10);
        pr_err("[ERROR] Read_I2C: BUS I2C Ocupado\n");
        if (i == 4)
        {
            pr_err("[ERROR] Read_I2C: Me canse de esperar abortando escritura\n");
            return -1;
        }
        else
        {
            i++;
        }
        aux = ioread32(i2c2_baseAddr + I2C_IRQSTATUS_RAW);
   }

    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQENABLE_CLR);  //Desactivo todas las interrupciones
    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQSTATUS);      //Por si alguna quedó sin atender las limpio tambien

    //Inicializo estructura
    data_i2c.tx_len   = 1;
    data_i2c.rx_len   = lenRx;
    data_i2c.tx_index = 0;
    data_i2c.rx_index = 0;
    wake_up_rx = 0;
    wake_up_tx = 0;

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_TRX | I2C_CON_MST;   // Pongo modo master transmitter
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    aux = ioread32(i2c2_baseAddr + I2C_IRQENABLE_SET);
    aux |= IRQ_XRDY_MASK | IRQ_ARDY_MASK;   //Activo interrupciones de XRDY y ARDY
    iowrite32(aux, i2c2_baseAddr + I2C_IRQENABLE_SET);

    iowrite32(I2C_OA_VALUE, i2c2_baseAddr + I2C_OA);              // Own Address
    iowrite32(I2C_SA_ADDR , i2c2_baseAddr + I2C_SA);              // Slave Address
    iowrite32(data_i2c.tx_len, i2c2_baseAddr + I2C_CNT);          // Contador de dataTx a transmitir - 1 Byte
	iowrite32(registro, i2c2_baseAddr + I2C_DATA);                // Dato a escribir en este caso el registro que voy a leer

    data_i2c.tx_buff = (uint8_t *)kmalloc(data_i2c.tx_len * sizeof(uint8_t), GFP_KERNEL);
	if(data_i2c.tx_buff == NULL) {
		pr_alert("[Lectura Sensor]-$ Error en kmalloc para txbuff.\n");
		return -1;
	}

    data_i2c.tx_buff[0] = registro;

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STT | I2C_CON_STP;  // Genero condiciones de start y stop
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    pr_alert("[Lectura Sensor]-$ Mandando al proceso a la cola de interruptible.\n");

    wait_event_interruptible(queue, wake_up_tx > 0); //Cambio la cola de ejecución del proceso a interruptable

    wake_up_rx = 0;
    wake_up_tx = 0;
    data_i2c.tx_index = 0;

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STP;  //Genero condición de stop
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_MST;                     // Vuelvo a poner al i2c como master ya que se limpia despues de un stop
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    aux = ioread32(i2c2_baseAddr + I2C_CON);
	aux &= (~I2C_CON_TRX);                  // Lo pongo en modo recepción
	iowrite32(aux, i2c2_baseAddr + I2C_CON);

    iowrite32(IRQ_XRDY_MASK, i2c2_baseAddr + I2C_IRQENABLE_CLR);  // Apago XRDY IRQ
    iowrite32(IRQ_RRDY_MASK, i2c2_baseAddr + I2C_IRQENABLE_SET);  // Prendo RRDY IRQ

	iowrite32(data_i2c.rx_len, i2c2_baseAddr + I2C_CNT); // Contador de bytes a recibir

    data_i2c.rx_buff = (uint8_t *)kmalloc(data_i2c.rx_len, GFP_KERNEL);
	if(data_i2c.rx_buff == NULL) {
		pr_alert("[readByteMPU6050]-$ Error requesting kmallog.\n");
		return -1;
	}
	
    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STT | I2C_CON_STP; 
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    wait_event_interruptible(queue, wake_up_rx > 0);

    wake_up_rx = 0;
    wake_up_tx = 0;
    data_i2c.rx_index = 0;

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STP;
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

	ret = data_i2c.rx_len;

	iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQENABLE_CLR);    // Limpio todas las interrupciones
    
    for(i = 0; i<data_i2c.rx_len; i++)
    {
      dataRx[i] = data_i2c.rx_buff[i];
    }

    kfree(data_i2c.rx_buff);
    kfree(data_i2c.tx_buff);
	return ret;
}

uint32_t writeSensor(uint8_t * dataTx, uint32_t lenTx)
{
    int aux = 0;
    int i = 0;
    i2c_softreset();

    aux = ioread32(i2c2_baseAddr + I2C_IRQSTATUS_RAW);
	
    while (aux & I2C_BUS_BUSY_MASK)
    {
        msleep(10);
        pr_err("[ERROR] Read_I2C: BUS I2C Ocupado\n");
        if (i == 4)
        {
            pr_err("[ERROR] Read_I2C: Me canse de esperar abortando escritura\n");
            return -1;
        }
        else
        {
            i++;
        }
         
        aux = ioread32(i2c2_baseAddr + I2C_IRQSTATUS_RAW);
   }
    
    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQSTATUS);   // Limpio el registro de interrupciones por siquedó algun bit

    /* Seteo Master y Trx para poner en modo master transmitter */
    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_MST | I2C_CON_TRX;       
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    data_i2c.tx_len   = lenTx;
    data_i2c.rx_len   = 0;
    data_i2c.tx_index = 0;
    data_i2c.rx_index = 0;
    wake_up_rx = 0;
    wake_up_tx = 0;
    
    data_i2c.tx_buff = dataTx;

    iowrite32(data_i2c.tx_len, i2c2_baseAddr + I2C_CNT);         // Cargo el contador que usa el i2c para transmitir

    aux = ioread32(i2c2_baseAddr + I2C_IRQENABLE_SET);
    aux |= IRQ_XRDY_MASK | IRQ_ARDY_MASK;                       // Habilito interrupciones de XRDY y ARDY
    iowrite32(aux, i2c2_baseAddr + I2C_IRQENABLE_SET);

 
    
    /* Agrego condiciones de start y stop al registro de control */
    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STT | I2C_CON_STP;
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    wait_event_interruptible(queue, wake_up_tx > 0);

    wake_up_rx = 0;
    wake_up_tx = 0;
    data_i2c.tx_index = 0;

    aux = ioread32(i2c2_baseAddr + I2C_CON);
    aux |= I2C_CON_STP;
    iowrite32(aux, i2c2_baseAddr + I2C_CON);

    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQENABLE_CLR);    // Limpio todas las interrupciones


    return 0;
}


irqreturn_t jyl_i2c_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    uint32_t irq_i2c_status = ioread32(i2c2_baseAddr + I2C_IRQSTATUS);   // Cargo el registro de IRQ Status para saber por que me interrumpio
    //pr_info("[irq_handler]-$ Status = 0x%X\n", irq_i2c_status);
    
    if( irq_i2c_status & IRQ_RRDY_MASK )    // Si es por recepción 
    {
        //pr_info("[irq_handler_RX]-$ Tenemos dato listo, rx_index = %d, rx_len = %d.\n", data_i2c.rx_index, data_i2c.rx_len);
        data_i2c.rx_buff[data_i2c.rx_index] = ioread32(i2c2_baseAddr + I2C_DATA);
        pr_info("[irq_handler_RX]-$ Tenemos dato listo, rx_index = %d, dato= 0x%X.\n", data_i2c.rx_index, data_i2c.rx_buff[data_i2c.rx_index]);
        data_i2c.rx_index++;

        if(data_i2c.rx_len == data_i2c.rx_index)
        {
            iowrite32(IRQ_RRDY_MASK, i2c2_baseAddr + I2C_IRQENABLE_CLR);     // Clear RX IRQ
            wake_up_rx = 1;
            wake_up_interruptible(&queue);
        }
        iowrite32(IRQ_RRDY_MASK, i2c2_baseAddr + I2C_IRQSTATUS);          // Clear status
    }

    if( irq_i2c_status & IRQ_XRDY_MASK )    // TX handler
    {
        //pr_info("[irq_handler_TX]-$ Marche el siguiente dato maestro.\n");
        iowrite32(data_i2c.tx_buff[data_i2c.tx_index], i2c2_baseAddr + I2C_DATA);   // Data to transmit
        data_i2c.tx_index++;

        if(data_i2c.tx_len == data_i2c.tx_index)
        {
            iowrite32(IRQ_XRDY_MASK, i2c2_baseAddr + I2C_IRQENABLE_CLR);
        }

        iowrite32(IRQ_XRDY_MASK, i2c2_baseAddr + I2C_IRQSTATUS);
    }

    if( irq_i2c_status & IRQ_ARDY_MASK )    // Access Ready handler
    {
        //pr_info("[irq_handler_ARDY]-$ El registro se accedió (ARDY) y esta disponible de nuevo.\n");
        iowrite32(IRQ_ARDY_MASK, i2c2_baseAddr + I2C_IRQSTATUS);
        wake_up_tx = 1;
        wake_up_interruptible(&queue);
        iowrite32(IRQ_ARDY_MASK, i2c2_baseAddr + I2C_IRQENABLE_CLR);
    }

    return IRQ_HANDLED;
}

static void i2c_softreset(void) 
{
	iowrite32(0, i2c2_baseAddr + I2C_CON);
	iowrite32(I2C_EN, i2c2_baseAddr + I2C_CON);

	return;
}

static int open_jyl(struct inode *inode, struct file *file)
{
    int aux = 0;
    pr_alert("[open_jyl]-$ Inicializando BBB i2c2 ... (reza malena)\n");


    /*    El I2C esta multiplexado con la uart1, a continuación le digo que voy a usa el I2C2   */
    iowrite32(0x3B, ctlmod_baseAddr + CTRL_MODULE_UART1_CTSN); //0x3B Seteo modo 3 (pin i2c), Bit 3 Pullup habilitacda y Bit 6 RXActive en 1 
    iowrite32(0x3B, ctlmod_baseAddr + CTRL_MODULE_UART1_RTSN);

    /* Vuelvo a setear el clk again, si no lo hago no me deja leer */
    iowrite32(0x02, cmper_baseAddr + IDCM_PER_I2C_CLKCTRL);
    do{
	    msleep(1);
    }while( ioread32(cmper_baseAddr + IDCM_PER_I2C_CLKCTRL) != 0x2 );
    msleep(10);

	i2c_softreset();    //El manual pide un softreset antes de utilizar le modulo
    
    /*  SET de registros de los pines de I2C   */
    iowrite32(0x00, i2c2_baseAddr + I2C_CON);           //Escribo el valor por defecto
    iowrite32(0x03, i2c2_baseAddr + I2C_PSC);           //Divisor de freq, divido por 4
    iowrite32(I2C_SCLL_400K, i2c2_baseAddr + I2C_SCLL); //Seteo las frcuencias de low y high en fast speed (400k)
    iowrite32(I2C_SCLH_400K, i2c2_baseAddr + I2C_SCLH);
    iowrite32(0x36, i2c2_baseAddr + I2C_OA);            //Configuro Own Addr
    iowrite32(0x00, i2c2_baseAddr + I2C_SYSC);          //Configuro el registro por defecto
    iowrite32(I2C_SA_ADDR, i2c2_baseAddr + I2C_SA);     //Slave Addr
    iowrite32(I2C_EN, i2c2_baseAddr +I2C_CON);          //Habilito el modulo

    aux = ioread32(i2c2_baseAddr  + I2C_IRQSTATUS);
	iowrite32(0x7FFF, i2c2_baseAddr  + I2C_IRQSTATUS);  //Habilito todas las interrupciones


    init_mpu6050();

    return 0;
}

static int release_jyl(struct inode *inode, struct file *file)
{
    uint32_t aux;
    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQENABLE_CLR);  //Desactivo todas las interrupciones
    iowrite32(0xFFFF, i2c2_baseAddr + I2C_IRQSTATUS);      //Por si alguna quedó sin atender las limpio tambien

    aux = ioread32(i2c2_baseAddr +I2C_CON);          // Deshabilito el modulo
    aux &= ~I2C_EN;
    iowrite32(aux, i2c2_baseAddr +I2C_CON);
    
    return 0;
}

static ssize_t read_jyl(struct file * device_descriptor, char __user *user_buffer, size_t read_len, loff_t * my_loff_t)
{
  
    int aux = 0;
    int ret = 0;
    int i = 0;
    uint8_t cant = 0;
    dataframe **k_data;
    uint8_t *k_buff;

    pr_alert("[Read]-$ Vamo a leer.\n");

    if( read_len * DATAFRAME_LEN > FIFO_LEN)
    {
        pr_alert("[Read]-$ ERROR: La memoria a leer sobrepasa la FIFO\n");
		return -1;
    }

    k_data = (dataframe **) kmalloc(read_len * sizeof(dataframe *), GFP_KERNEL);
	if(k_data == NULL) {
		pr_alert("[Read]-$ ERROR: Error en kmalloc para k_data\n");
		return -1;
	}

    for(i = 0; i < read_len; i++) 
    {
        k_data[i] = (dataframe *) kmalloc(sizeof(dataframe), GFP_KERNEL);
    	if(k_data == NULL) {
	    	pr_alert("[Read]-$ ERROR: Error en kmalloc para k_data\n");
		    return -1;
	    }

        cant = (uint8_t) readSensor(ACCEL_XOUT_H, (uint8_t *) k_data [i], DATAFRAME_LEN);
        if(cant != 14) {
            pr_alert("[Read]-$ ERROR: Canridad leida incorrecta\n");
        }
        else
            ret++;
    }

    k_buff = (uint8_t*) kmalloc(read_len * DATAFRAME_LEN, GFP_KERNEL);
	if(k_buff == NULL) {
		pr_alert("[Read]-$ ERROR: Error en kmalloc para k_data\n");
		return -1;
	}

    for(i = 0; i<read_len; i++)
    {
        k_buff[i*DATAFRAME_LEN + 0] = k_data[i]->ACCELX_H;
        k_buff[i*DATAFRAME_LEN + 1] = k_data[i]->ACCELX_L;
        k_buff[i*DATAFRAME_LEN + 2] = k_data[i]->ACCELY_H;
        k_buff[i*DATAFRAME_LEN + 3] = k_data[i]->ACCELY_L;
        k_buff[i*DATAFRAME_LEN + 4] = k_data[i]->ACCELZ_H;
        k_buff[i*DATAFRAME_LEN + 5] = k_data[i]->ACCELZ_L;
        k_buff[i*DATAFRAME_LEN + 6] = k_data[i]->TEMP_H;
        k_buff[i*DATAFRAME_LEN + 7] = k_data[i]->TEMP_L;
        k_buff[i*DATAFRAME_LEN + 8] = k_data[i]->GYROX_H;
        k_buff[i*DATAFRAME_LEN + 9] = k_data[i]->GYROX_L;
        k_buff[i*DATAFRAME_LEN +10] = k_data[i]->GYROY_H;
        k_buff[i*DATAFRAME_LEN +11] = k_data[i]->GYROY_L;
        k_buff[i*DATAFRAME_LEN +12] = k_data[i]->GYROZ_H;
        k_buff[i*DATAFRAME_LEN +13] = k_data[i]->GYROZ_L;
    }
	aux = copy_to_user(user_buffer, k_buff, read_len * DATAFRAME_LEN * sizeof(uint8_t));
	if(aux < 0) {
		pr_alert("[Read]-$ ERROR: copying to user.\n");
		return -1;
	}
    pr_alert("[Read]-$ Se lo mandamo al usuario gracia.\n");

    for( i = 0; i < read_len; i++)
        kfree(k_data[i]);
    kfree(k_data);
    kfree(k_buff);
    pr_alert("[Read]-$ Liberamo la memoria y salio todo bien creemo\n");


    return ret;
}

static ssize_t write_jyl(struct file * device_descriptor, const char __user *user_buffer, size_t read_len, loff_t * my_loff_t)
{
    ssize_t a = 0;

    i2c_softreset();

    return a;
}

static long ioctl_jyl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long a = 0;

    return a;
}

module_init(i2c_init);
module_exit(i2c_exit);