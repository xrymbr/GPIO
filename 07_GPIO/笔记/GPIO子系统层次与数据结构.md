## GPIO子系统层次与数据结构

参考资料：

* Linux 5.x内核文档
  * Linux-5.4\Documentation\driver-api
  * Linux-5.4\Documentation\devicetree\bindings\gpio\gpio.txt
  * Linux-5.4\drivers\gpio\gpio-74x164.c
* Linux 4.x内核文档
  * Linux-4.9.88\Documentation\gpio
  * Linux-4.9.88\Documentation\devicetree\bindings\gpio\gpio.txt
  * Linux-4.9.88\drivers\gpio\gpio-74x164.c

### 1. GPIO子系统的层次

#### 1.1 层次

![image-20210527103908743](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/03_gpio_system_level.png)

#### 1.2 GPIOLIB向上提供的接口

| descriptor-based       | legacy                | 说明 |
| ---------------------- | --------------------- | ---- |
| 获得GPIO               |                       |      |
| gpiod_get              | gpio_request          |      |
| gpiod_get_index        |                       |      |
| gpiod_get_array        | gpio_request_array    |      |
| devm_gpiod_get         |                       |      |
| devm_gpiod_get_index   |                       |      |
| devm_gpiod_get_array   |                       |      |
| 设置方向               |                       |      |
| gpiod_direction_input  | gpio_direction_input  |      |
| gpiod_direction_output | gpio_direction_output |      |
| 读值、写值             |                       |      |
| gpiod_get_value        | gpio_get_value        |      |
| gpiod_set_value        | gpio_set_value        |      |
| 释放GPIO               |                       |      |
| gpio_free              | gpio_free             |      |
| gpiod_put              | gpio_free_array       |      |
| gpiod_put_array        |                       |      |
| devm_gpiod_put         |                       |      |
| devm_gpiod_put_array   |                       |      |

#### 1.3 GPIOLIB向下提供的接口

![image-20210527141654915](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/07_gpiochip_add_data.png)

### 2. 重要的3个核心数据结构

记住GPIO Controller的要素，这有助于理解它的驱动程序：

* 一个GPIO Controller里有多少个引脚？有哪些引脚？
* 需要提供函数，设置引脚方向、读取/设置数值
* 需要提供函数，把引脚转换为中断

以Linux面向对象编程的思想，一个GPIO Controller必定会使用一个结构体来表示，这个结构体必定含有这些信息：

* GPIO引脚信息
* 控制引脚的函数
* 中断相关的函数

#### 2.1 gpio_device

每个GPIO Controller用一个gpio_device来表示：

* 里面每一个gpio引脚用一个gpio_desc来表示
* gpio引脚的函数(引脚控制、中断相关)，都放在gpio_chip里

![image-20210527115015701](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/04_gpio_device.png)

#### 2.2 gpio_chip

我们并不需要自己创建gpio_device，编写驱动时要创建的是gpio_chip，里面提供了：

* 控制引脚的函数
* 中断相关的函数
* 引脚信息：支持多少个引脚？各个引脚的名字？

![image-20210527141048567](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/05_gpio_chip.png)



#### 2.3 gpio_desc

我们去使用GPIO子系统时，首先是获得某个引脚对应的gpio_desc。

gpio_device表示一个GPIO Controller，里面支持多个GPIO。

在gpio_device中有一个gpio_desc数组，每一引脚有一项gpio_desc。

![image-20210527142042290](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/08_gpio_desc.png)

### 3. 怎么编写GPIO Controller驱动程序

分配、设置、注册gpioc_chip结构体，示例：`drivers\gpio\gpio-74x164.c`

![image-20210527141554940](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/06_gpio_controller_driver_example.png)

### 4. GPIO调用Pinctrl的过程

GPIO子系统中的request函数，用来申请某个GPIO引脚，

它会导致Pinctrl子系统中的这2个函数之一被调用：`pmxops->gpio_request_enable`或`pmxops->request`

![image-20210529101834908](E:/QRS/doc_and_source_for_drivers/IMX6ULL/doc_pic/07_GPIO/pic/07_GPIO/14_gpio_request.png)

调用关系如下：

```c
gpiod_get
    gpiod_get_index
    	desc = of_find_gpio(dev, con_id, idx, &lookupflags);
		ret = gpiod_request(desc, con_id ? con_id : devname);
					ret = gpiod_request_commit(desc, label);
								if (chip->request) {
                                    ret = chip->request(chip, offset);
                                }
```



我们编写GPIO驱动程序时，所设置`chip->request`函数，一般直接调用`gpiochip_generic_request`，它导致Pinctrl把引脚复用为GPIO功能。

```c
gpiochip_generic_request(struct gpio_chip *chip, unsigned offset)
    pinctrl_request_gpio(chip->gpiodev->base + offset)
		ret = pinctrl_get_device_gpio_range(gpio, &pctldev, &range); // gpio是引脚的全局编号

		/* Convert to the pin controllers number space */
		pin = gpio_to_pin(range, gpio);
    	
		ret = pinmux_request_gpio(pctldev, range, pin, gpio);
					ret = pin_request(pctldev, pin, owner, range);
```



Pinctrl子系统中的pin_request函数就会把引脚配置为GPIO功能：

```c
static int pin_request(struct pinctrl_dev *pctldev,
		       int pin, const char *owner,
		       struct pinctrl_gpio_range *gpio_range)
{
    const struct pinmux_ops *ops = pctldev->desc->pmxops;
    
	/*
	 * If there is no kind of request function for the pin we just assume
	 * we got it by default and proceed.
	 */
	if (gpio_range && ops->gpio_request_enable)
		/* This requests and enables a single GPIO pin */
		status = ops->gpio_request_enable(pctldev, gpio_range, pin);
	else if (ops->request)
		status = ops->request(pctldev, pin);
	else
		status = 0;
}
```



### 5. 我们要做什么

如果不想在使用GPIO引脚时，在设备树中设置Pinctrl信息，

如果想让GPIO和Pinctrl之间建立联系，

我们需要做这些事情：

#### 5.1 表明GPIO和Pinctrl间的联系

在GPIO设备树中使用`gpio-ranges`来描述它们之间的联系：

  * GPIO系统中有引脚号

  * Pinctrl子系统中也有自己的引脚号

  * 2个号码要建立映射关系

  * 在GPIO设备树中使用如下代码建立映射关系

    ```shell
    // 当前GPIO控制器的0号引脚, 对应pinctrlA中的128号引脚, 数量为12
    gpio-ranges = <&pinctrlA 0 128 12>; 
    ```

#### 5.2 解析这些联系

在GPIO驱动程序中，解析跟Pinctrl之间的联系：处理`gpio-ranges`:

  * 这不需要我们自己写代码

  * 注册gpio_chip时会自动调用

    ```c
    int gpiochip_add_data(struct gpio_chip *chip, void *data)
        status = of_gpiochip_add(chip);
    				status = of_gpiochip_add_pin_range(chip);
    
    of_gpiochip_add_pin_range
    	for (;; index++) {
    		ret = of_parse_phandle_with_fixed_args(np, "gpio-ranges", 3,
    				index, &pinspec);
    
        	pctldev = of_pinctrl_get(pinspec.np); // 根据gpio-ranges的第1个参数找到pctldev
    
            // 增加映射关系	
            /* npins != 0: linear range */
            ret = gpiochip_add_pin_range(chip,
                                         pinctrl_dev_get_devname(pctldev),
                                         pinspec.args[0],
                                         pinspec.args[1],
                                         pinspec.args[2]);
    ```


#### 5.3 编程

* 在GPIO驱动程序中，提供`gpio_chip->request`

* 在Pinctrl驱动程序中，提供`pmxops->gpio_request_enable`或`pmxops->request`

