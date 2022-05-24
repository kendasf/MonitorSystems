#!/bin/bash
clockRead="0"

echo Enable I2C-2 for RTC Clock
echo BB-I2C2 > /sys/devices/platform/bone_capemgr/slots

echo Map RTC to address 0x68 on i2c-2
if [ -d "/sys/bus/i2c/devices/2-0068" ]
then
        echo Device registered on I2C-2 address 0x68
        check=$(ln -sf /dev/rtc1 /dev/rtc)  
        clockReady="1"
else
        echo Trying DS1307 driver
        register=$(echo ds1307 0x68 > /sys/bus/i2c/devices/i2c-2/new_device)
        
        check=$(ls /sys/bus/i2c/devices | grep 2-0068)
        if [ ! -z "$check" ]
        then
	        echo DS1307 found on I2C-2 bus @ 0x68
                check=$(ln -sf /dev/rtc1 /dev/rtc)
                clockReady="1"
        else
                echo Trying PCF8523 driver
                register=$(echo pcf8523 0x68 > /sys/bus/i2c/devices/i2c-2/new_device | grep write)
                
                check=$(ls /sys/bus/i2c/devices | grep 2-0068)
                if [ ! -z "$check" ] 
                then
	                echo PCF8523 found on I2C-2 bus @ 0x68
                        check=$(ln -sf /dev/rtc1 /dev/rtc)
                        clockReady="1"
                else
                        echo No supported RTC devices found at 0x68
                fi
        fi
fi

if [ "$clockReady" == "1" ]
then
        utc=$(timedatectl | grep "Universal time" | cut -d " " -f 6-7)
        rtc=$(timedatectl | grep "RTC time" | cut -d " " -f 12-13)
        if [ "$rtc" != "$utc" ]
        then
                setTime=$(timedatectl --adjust-system-clock set-local-rtc 1)
                setTime=$(timedatectl --adjust-system-clock set-local-rtc 0)
        fi

        utc=$(timedatectl | grep "Universal time" | cut -d " " -f 6-7)
        rtc=$(timedatectl | grep "RTC time" | cut -d " " -f 12-13)
        if [ "$rtc" == "$utc" ]
        then
                echo "Clock set successfully"
                echo "UTC: $utc"
                echo "RTC: $rtc"
        else
                echo "Clock does not match, check RTC"
                echo "UTC: $utc"
                echo "RTC: $rtc "
        fi
fi

# # GPIO1_19 - Display_OE :
# echo Configure GPIO for Display_OE
# echo 35 > /sys/class/gpio/export              # export pin (gpio_bank * 32) + gpio_pin = 1*19 + 16 = 35
# echo out > /sys/class/gpio/gpio35/direction   # set direction
# echo 0   > /sys/class/gpio/gpio35/value       # set value
# echo 1   > /sys/class/gpio/gpio35/value       # set value
# echo 0   > /sys/class/gpio/gpio35/value       # set value

# # GPIO3_16 - Display_LE :
# echo Configure GPIO for Display_LE
# echo 112 > /sys/class/gpio/export              # export pin (gpio_bank * 32) + gpio_pin = 3*32 + 16 = 112
# echo out > /sys/class/gpio/gpio112/direction   # set direction
# echo 0   > /sys/class/gpio/gpio112/value       # set value

# # GPIO1_29 - CAM_PWR
# echo COnfigure GPIO for CAM_PWR
# echo 61  > /sys/class/gpio/export              # export pin (gpio_bank * 32) + gpio_pin = 1*32 + 28 = 61
# echo out > /sys/class/gpio/gpio61/direction    # set direction [in, out]
# echo 0   > /sys/class/gpio/gpio61/value        # set value     [0, 1]

echo Enable UART1
echo BB-UART1 > /sys/devices/platform/bone_capemgr/slots

echo Enable UART4
echo BB-UART4 > /sys/devices/platform/bone_capemgr/slots

echo Enable SPI
echo BB-SPIDEV0 > /sys/devices/platform/bone_capemgr/slots

# echo Enable PWM1
# echo BB-PWM1 > /sys/devices/platform/bone_capemgr/slots

# echo Configure Pins for PWM
# echo 0 > /sys/class/pwm/pwmchip0/export &   # P9.14 (EHRPWM1a)   # Enable PWM 0
# wait
# echo 200000 > /sys/class/pwm/pwmchip0/pwm0/period &
# wait
# echo 0 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle &
# wait
# echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable &
# wait

#echo 1 > /sys/class/pwm/pwmchip0/export   # P9.16 (EHRPWM1b)

echo adc
echo BB-ADC > /sys/devices/platform/bone_capemgr/slots