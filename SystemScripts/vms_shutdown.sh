#!/bin/bash

echo Write system time to RTC
clock_result=$(hwclock --systohc --rtc /dev/rtc1)
if [ -z "$clock_result" ]
then
        echo "Clock set successfully"
else
        echo "RTC unable to be set - $clock_result"
fi
