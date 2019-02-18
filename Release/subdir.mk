################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../blinky.cpp \
../prioq.cpp 

OBJS += \
./blinky.o \
./prioq.o 

CPP_DEPS += \
./blinky.d \
./prioq.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: AVR C++ Compiler'
	avr-g++ -I"C:\Program Files\Arduino\hardware\arduino\avr\cores\arduino" -I"C:\Users\Sherry\Desktop\nus\CG2271\lab3\freeRTOS\freeRTOS\Source\include" -I"C:\Program Files\Arduino\hardware\arduino\avr\variants\standard" -Wall -Os -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -funsigned-char -funsigned-bitfields -fno-exceptions -mmcu=atmega328p -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


