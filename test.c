#include <linux/module.h> /* Содержит функции и определения для динамической загрузки модулей ядра */
#include <linux/init.h>  /* Указывает на функции инициализации и очистки */
#include <linux/fs.h>    /* Содержит функции регистрации и удаления драйвера */
#include <linux/cdev.h>  /* Содержит необходимые функции для символьного драйвера */
#include <linux/slab.h>  /* Содержит функцию ядра для управления памятью */
#include <asm/uaccess.h> /* Предоставляет доступ к пространству пользователя */

// Ниже мы задаём информацию о модуле, которую можно будет увидеть с помощью Modinfo
MODULE_LICENSE( "GPL" );
MODULE_SUPPORTED_DEVICE( "test" ); /* /dev/testdevice */

#define SUCCESS 0
#define DEVICE_NAME "test" /* Имя нашего устройства */

// Поддерживаемые нашим устройством операции
static int device_open( struct inode *, struct file * );
static int device_release( struct inode *, struct file * );
static ssize_t device_read( struct file *, char *, size_t, loff_t * );

// Глобальные переменные, объявлены как static, воизбежание конфликтов имен.
static int major_number; /* Старший номер устройства нашего драйвера */
static int is_device_open = 0; /* Используется ли девайс ? */
static char text[ 5 ] = "test\n"; /* Из этого внутреннего буфера мы будем читать в функции read */
static char* text_ptr = text; /* Указатель на текущую позицию в тексте */

// Прописываем обработчики операций на устройством
static struct file_operations fops =
 {
 	.open = device_open,
 	.release = device_release,
 	.read = device_read
 };



// Функция загрузки модуля. Входная точка. Можем считать что это наш main()
static int __init test_init( void )
{
	printk( KERN_ALERT "TEST driver loaded!\n" );

	// Регистрируем устройсво и получаем старший номер устройства
	major_number = register_chrdev( 0, DEVICE_NAME, &fops );

	if ( major_number < 0 )
	{
		printk( "Registering the character device failed with %d\n", major_number );
		return major_number;
	}

	// Сообщаем присвоенный нам старший номер устройства
	printk( "Test module is loaded!\n" );

	printk( "Please, create a dev file with 'mknod /dev/test c %d 0'.\n", major_number );

	return SUCCESS;
}

// Функция выгрузки модуля
static void __exit test_exit( void )
{
	// Освобождаем устройство
	unregister_chrdev( major_number, DEVICE_NAME );

	printk( KERN_ALERT "Test module is unloaded!\n" );
}

// Указываем наши функции загрузки и выгрузки
module_init( test_init );
module_exit( test_exit );



static int device_open( struct inode *inode, struct file *file )
{
	text_ptr = text;

	if ( is_device_open )
		return -EBUSY;

	is_device_open++;

	return SUCCESS;
}

static int device_release( struct inode *inode, struct file *file )
{
	is_device_open--;
	return SUCCESS;
}

static ssize_t device_read( struct file *filp, char *buffer, size_t length, loff_t * offset ){
	int byte_read = 0;

	if ( *text_ptr == 0 )//text_ptr = text[], тот массив. из которого мы читаем
		return 0;

	while ( length && *text_ptr )
	{
		put_user( *( text_ptr++ ), buffer++ );
		length--;
		byte_read++;
	}

	return byte_read;
}